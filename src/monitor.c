/*
 * src/monitor.c
 *
 * Central monitoring engine. Implements monitor_init/collect/evaluate.
 */

#include "monitor.h"
#include "logger.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

int monitor_init(MonitorContext *ctx, const MonitorConfig *config,
                 volatile int *running)
{
    if (ctx == NULL || config == NULL) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->config = *config;
    ctx->running = running;

    alert_init(&ctx->cpu_alert);
    alert_init(&ctx->memory_alert);
    alert_init(&ctx->disk_alert);
    alert_init(&ctx->load_alert);

    /* Establish CPU baseline (first sample). */
    if (cpu_read_times(&ctx->cpu_prev) == 0) {
        ctx->have_cpu_baseline = 1;
    }

    /* Establish network baseline. */
    if (network_read(&ctx->net_prev) == 0) {
        ctx->have_net_baseline = 1;
    }

    return 0;
}

int monitor_collect(MonitorContext *ctx, SystemMetricsSnapshot *snap)
{
    CpuTimes cpu_curr;
    double elapsed;

    if (ctx == NULL || snap == NULL) {
        return -1;
    }

    memset(snap, 0, sizeof(*snap));

    /* CPU: read a second sample and compute delta vs baseline. */
    if (ctx->have_cpu_baseline && cpu_read_times(&cpu_curr) == 0) {
        unsigned long long total_delta = cpu_total(&cpu_curr) - cpu_total(&ctx->cpu_prev);
        unsigned long long idle_delta = cpu_idle(&cpu_curr) - cpu_idle(&ctx->cpu_prev);
        /* A sub-tick sample window (e.g. --once/--json mode) yields a
         * meaningless CPU%; require at least a couple of ticks to avoid
         * reporting 0% or 100% based on rounding noise. */
        if (total_delta >= 2) {
            if (cpu_calc_usage(&ctx->cpu_prev, &cpu_curr,
                               &snap->cpu.usage_percent) == 0) {
                snap->cpu.total_delta = total_delta;
                snap->cpu.idle_delta = idle_delta;
                ctx->cpu_busy_delta = (total_delta > idle_delta)
                                      ? (total_delta - idle_delta) : 0;
            }
        } else {
            snap->cpu.usage_percent = 0.0;
            ctx->cpu_busy_delta = 0;
        }
        ctx->cpu_prev = cpu_curr; /* advance baseline */
    }

    /* Memory: single sample. */
    if (memory_read(&snap->memory) != 0) {
        logger_warning("memory monitor: /proc/meminfo unreadable");
    }

    /* Disk: single sample. */
    if (disk_read(ctx->config.disk_path, &snap->disk) != 0) {
        logger_warning("disk monitor: statvfs('%s') failed", ctx->config.disk_path);
    }

    /* System: load + uptime. */
    if (system_read(&snap->system) != 0) {
        logger_warning("system monitor: load/uptime unreadable");
    }

    /* Processes: scan. */
    snap->process_count = process_scan(snap->processes, PROCESS_MAX);
    if (snap->process_count < 0) {
        logger_warning("process monitor: /proc scan failed");
        snap->process_count = 0;
    }

    /* Per-process CPU%: compare current scan against the previous scan. */
    if (snap->process_count > 0 && ctx->have_proc_baseline &&
        ctx->cpu_busy_delta > 0) {
        process_calc_cpu_percent(ctx->proc_prev, ctx->proc_prev_count,
                                 snap->processes, snap->process_count,
                                 ctx->cpu_busy_delta);
    }

    /* Advance the process baseline. */
    if (snap->process_count > 0) {
        memcpy(ctx->proc_prev, snap->processes,
               (size_t)snap->process_count * sizeof(ProcessInfo));
        ctx->proc_prev_count = snap->process_count;
        ctx->have_proc_baseline = 1;
    }

    /* Network: second sample + rates. */
    if (ctx->config.network_monitor && ctx->have_net_baseline) {
        NetworkSnapshot net_curr;
        if (network_read(&net_curr) == 0) {
            /* elapsed = config interval (approx), or measure monotonic */
            elapsed = monotonic_seconds();
            (void)elapsed;
            network_calc_rates(&ctx->net_prev, &net_curr,
                               (double)ctx->config.monitor_interval);
            snap->network = net_curr;
            ctx->net_prev = net_curr;
        }
    }

    return 0;
}

/* Emit a single alert to the logger and as a console line. */
static void emit_alert(const char *resource, AlertEvent ev, double value,
                       double warn, double crit)
{
    switch (ev) {
    case ALERT_EVENT_WARNING:
        logger_warning("%s usage %.1f%% exceeded warning threshold %.1f%%",
                       resource, value, warn);
        break;
    case ALERT_EVENT_CRITICAL:
        logger_critical("%s usage %.1f%% exceeded critical threshold %.1f%%",
                        resource, value, crit);
        break;
    case ALERT_EVENT_RECOVERY:
        logger_info("%s usage returned to normal (%.1f%%)", resource, value);
        break;
    case ALERT_EVENT_NONE:
    default:
        break;
    }
}

int monitor_evaluate(MonitorContext *ctx, const SystemMetricsSnapshot *snap)
{
    AlertEvent ev;
    int emitted = 0;

    if (ctx == NULL || snap == NULL) {
        return 0;
    }

    /* CPU */
    ev = alert_evaluate(&ctx->cpu_alert, snap->cpu.usage_percent,
                        ctx->config.cpu_warning, ctx->config.cpu_critical,
                        (double)ctx->config.alert_cooldown);
    if (ev != ALERT_EVENT_NONE) {
        emit_alert("CPU", ev, snap->cpu.usage_percent,
                   ctx->config.cpu_warning, ctx->config.cpu_critical);
        emitted++;
    }

    /* Memory */
    ev = alert_evaluate(&ctx->memory_alert, snap->memory.usage_percent,
                        ctx->config.memory_warning, ctx->config.memory_critical,
                        (double)ctx->config.alert_cooldown);
    if (ev != ALERT_EVENT_NONE) {
        emit_alert("Memory", ev, snap->memory.usage_percent,
                   ctx->config.memory_warning, ctx->config.memory_critical);
        emitted++;
    }

    /* Disk */
    ev = alert_evaluate(&ctx->disk_alert, snap->disk.usage_percent,
                        ctx->config.disk_warning, ctx->config.disk_critical,
                        (double)ctx->config.alert_cooldown);
    if (ev != ALERT_EVENT_NONE) {
        emit_alert("Disk", ev, snap->disk.usage_percent,
                   ctx->config.disk_warning, ctx->config.disk_critical);
        emitted++;
    }

    /* Load (compare against load_warning/critical, using load_1) */
    ev = alert_evaluate(&ctx->load_alert, snap->system.load_1,
                        ctx->config.load_warning, ctx->config.load_critical,
                        (double)ctx->config.alert_cooldown);
    if (ev != ALERT_EVENT_NONE) {
        /* load is not a percentage, so use a dedicated message */
        if (ev == ALERT_EVENT_RECOVERY) {
            logger_info("Load average returned to normal (%.2f)",
                        snap->system.load_1);
        } else if (ev == ALERT_EVENT_WARNING) {
            logger_warning("Load average %.2f exceeded warning %.1f",
                           snap->system.load_1, ctx->config.load_warning);
        } else {
            logger_critical("Load average %.2f exceeded critical %.1f",
                            snap->system.load_1, ctx->config.load_critical);
        }
        emitted++;
    }

    return emitted;
}
