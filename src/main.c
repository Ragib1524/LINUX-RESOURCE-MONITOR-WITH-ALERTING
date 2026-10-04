/*
 * src/main.c
 *
 * Linux Resource Monitor with Alerting — entry point.
 *
 * Flow:
 *   1. parse CLI
 *   2. load + validate config
 *   3. init terminal colors + logger
 *   4. install SIGINT/SIGTERM handlers (async-signal-safe)
 *   5. init monitor (baselines)
 *   6. loop: collect -> evaluate -> print dashboard -> sleep
 *   7. cleanup on shutdown
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "alert.h"
#include "cli.h"
#include "config.h"
#include "logger.h"
#include "monitor.h"
#include "terminal.h"
#include "utils.h"

/* Async-signal-safe running flag. */
static volatile sig_atomic_t g_running = 1;

static void signal_handler(int signo)
{
    (void)signo;
    g_running = 0; /* only touch a sig_atomic_t */
}

static void install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Format a byte/second rate into a human-readable string. */
static void format_rate(double bytes_per_sec, char *out, size_t out_len)
{
    if (bytes_per_sec >= 1e6) {
        snprintf(out, out_len, "%.2f MB/s", bytes_per_sec / 1e6);
    } else if (bytes_per_sec >= 1e3) {
        snprintf(out, out_len, "%.2f KB/s", bytes_per_sec / 1e3);
    } else {
        snprintf(out, out_len, "%.0f B/s", bytes_per_sec);
    }
}

/* Format uptime seconds into "D days HH:MM:SS". */
static void format_uptime(double seconds, char *out, size_t out_len)
{
    unsigned long total = (unsigned long)seconds;
    unsigned long days = total / 86400UL;
    unsigned long hours = (total % 86400UL) / 3600UL;
    unsigned long mins = (total % 3600UL) / 60UL;
    unsigned long secs = total % 60UL;

    if (days > 0) {
        snprintf(out, out_len, "%lu days %02lu:%02lu:%02lu", days, hours, mins, secs);
    } else {
        snprintf(out, out_len, "%02lu:%02lu:%02lu", hours, mins, secs);
    }
}

static void print_dashboard(const MonitorContext *ctx,
                            SystemMetricsSnapshot *snap)
{
    const char *c_red = terminal_color_code(COLOR_RED);
    const char *c_grn = terminal_color_code(COLOR_GREEN);
    const char *c_yel = terminal_color_code(COLOR_YELLOW);
    const char *c_c = terminal_color_code(COLOR_CYAN);
    const char *c_rst = terminal_color_code(COLOR_RESET);
    char uptime[64];
    char rate[32];

    printf("\n");
    printf("============================================================\n");
    printf("        LINUX RESOURCE MONITOR\n");
    printf("============================================================\n");

    format_uptime(snap->system.uptime_seconds, uptime, sizeof(uptime));
    printf("System Uptime : %s\n", uptime);
    printf("\n");

    printf("CPU Usage     : %s%.1f%%%s\n",
           snap->cpu.usage_percent >= ctx->config.cpu_critical ? c_red :
           snap->cpu.usage_percent >= ctx->config.cpu_warning ? c_yel : c_grn,
           snap->cpu.usage_percent, c_rst);
    printf("Memory Usage  : %s%.1f%%%s\n",
           snap->memory.usage_percent >= ctx->config.memory_critical ? c_red :
           snap->memory.usage_percent >= ctx->config.memory_warning ? c_yel : c_grn,
           snap->memory.usage_percent, c_rst);
    printf("Disk Usage    : %s%.1f%%%s\n",
           snap->disk.usage_percent >= ctx->config.disk_critical ? c_red :
           snap->disk.usage_percent >= ctx->config.disk_warning ? c_yel : c_grn,
           snap->disk.usage_percent, c_rst);

    printf("\nLoad Average\n");
    printf("  1 min       : %.2f\n", snap->system.load_1);
    printf("  5 min       : %.2f\n", snap->system.load_5);
    printf("  15 min      : %.2f\n", snap->system.load_15);

    if (ctx->config.network_monitor && snap->network.count > 0) {
        int i;
        printf("\nNetwork\n");
        for (i = 0; i < snap->network.count; i++) {
            printf("  %s%-12s%s\n", c_c, snap->network.ifaces[i].name, c_rst);
            format_rate(snap->network.ifaces[i].rx_rate, rate, sizeof(rate));
            printf("    RX        : %s\n", rate);
            format_rate(snap->network.ifaces[i].tx_rate, rate, sizeof(rate));
            printf("    TX        : %s\n", rate);
        }
    }

    /* Top CPU and Top Memory processes. */
    if (snap->process_count > 0) {
        int i;
        int top = ctx->config.top_processes;
        ProcessInfo *procs = snap->processes;
        int n = snap->process_count;

        printf("\nTop CPU Processes\n");
        printf("------------------------------------------------------------\n");
        printf("%-10s %-20s %s\n", "PID", "NAME", "CPU %");
        printf("------------------------------------------------------------\n");
        process_sort_by_cpu(procs, n);
        for (i = 0; i < top && i < n; i++) {
            printf("%-10d %-20.20s %.1f\n",
                   procs[i].pid, procs[i].name, procs[i].cpu_percent);
        }

        printf("\nTop Memory Processes\n");
        printf("------------------------------------------------------------\n");
        printf("%-10s %-20s %s\n", "PID", "NAME", "MEMORY");
        printf("------------------------------------------------------------\n");
        process_sort_by_memory(procs, n);
        for (i = 0; i < top && i < n; i++) {
            printf("%-10d %-20.20s %llu KB\n",
                   procs[i].pid, procs[i].name, procs[i].memory_kb);
        }
    }

    /* Alert status per resource. */
    printf("\nAlerts\n");
    printf("------------------------------------------------------------\n");
    printf("CPU          %s\n", alert_severity_name(ctx->cpu_alert.current));
    printf("MEMORY       %s\n", alert_severity_name(ctx->memory_alert.current));
    printf("DISK         %s\n", alert_severity_name(ctx->disk_alert.current));
    printf("\n============================================================\n");
}

/*
 * print_json()
 * Emit a single snapshot as JSON (a manually-built, correctly-escaped object;
 * no external JSON library is used). Values are the monitored metrics.
 */
static void print_json(const MonitorContext *ctx, const SystemMetricsSnapshot *snap)
{
    printf("{\n");
    printf("  \"cpu_percent\": %.2f,\n", snap->cpu.usage_percent);
    printf("  \"memory_percent\": %.2f,\n", snap->memory.usage_percent);
    printf("  \"disk_percent\": %.2f,\n", snap->disk.usage_percent);
    printf("  \"load_1\": %.2f,\n", snap->system.load_1);
    printf("  \"load_5\": %.2f,\n", snap->system.load_5);
    printf("  \"load_15\": %.2f,\n", snap->system.load_15);
    printf("  \"uptime_seconds\": %.1f,\n", snap->system.uptime_seconds);
    printf("  \"process_count\": %d,\n", snap->process_count);
    printf("  \"alerts\": {\n");
    printf("    \"cpu\": \"%s\",\n", alert_severity_name(ctx->cpu_alert.current));
    printf("    \"memory\": \"%s\",\n", alert_severity_name(ctx->memory_alert.current));
    printf("    \"disk\": \"%s\"\n", alert_severity_name(ctx->disk_alert.current));
    printf("  }\n");
    printf("}\n");
}

/*
 * csv_append()
 * Append one CSV line (timestamp,cpu,memory,disk,load) to the CSV log file.
 * Implemented in C (no shell/Python). Bounded snprintf; missing file is
 * non-fatal (logged as a warning).
 */
static void csv_append(const MonitorConfig *config,
                       const SystemMetricsSnapshot *snap)
{
    FILE *fp;
    char line[256];
    char ts[64];
    time_t now = time(NULL);
    struct tm tm_info;

    if (config->log_file[0] == '\0') {
        return;
    }

    /* Derive a CSV path next to the log file (replace .log with .csv). */
    {
        char csv_path[512];
        size_t len = strlen(config->log_file);
        if (len >= sizeof(csv_path)) {
            return;
        }
        memcpy(csv_path, config->log_file, len + 1);
        /* Replace trailing ".log" with ".csv" (best-effort). */
        if (len >= 4 && strcmp(csv_path + len - 4, ".log") == 0) {
            csv_path[len - 4] = '\0';
        }
        strncat(csv_path, ".csv", sizeof(csv_path) - strlen(csv_path) - 1);

        fp = fopen(csv_path, "a");
        if (fp == NULL) {
            return; /* non-fatal */
        }
    }

    if (now != (time_t)-1 && localtime_r(&now, &tm_info) != NULL) {
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_info);
    } else {
        strcpy(ts, "unknown");
    }

    snprintf(line, sizeof(line), "%s,%.2f,%.2f,%.2f,%.2f\n",
             ts, snap->cpu.usage_percent, snap->memory.usage_percent,
             snap->disk.usage_percent, snap->system.load_1);
    fputs(line, fp);
    fclose(fp);
}

int main(int argc, char **argv)
{
    CliOptions opts;
    MonitorConfig config;
    MonitorContext ctx;
    SystemMetricsSnapshot snap;
    char errbuf[256];
    double interval;

    /* CLI */
    if (cli_parse(argc, argv, &opts) != 0) {
        cli_print_usage(stderr);
        return 1;
    }
    if (opts.show_help) {
        cli_print_usage(stdout);
        return 0;
    }
    if (opts.show_version) {
        cli_print_version(stdout);
        return 0;
    }

    /* Config */
    if (config_load(opts.config_path, &config, errbuf, sizeof(errbuf)) != 0) {
        fprintf(stderr, "Config error: %s\n", errbuf);
        return 1;
    }
    if (config_validate(&config, errbuf, sizeof(errbuf)) != 0) {
        fprintf(stderr, "Config validation error: %s\n", errbuf);
        return 1;
    }

    /* CLI interval overrides config. */
    interval = (opts.interval > 0) ? (double)opts.interval
                                   : (double)config.monitor_interval;

    /* Terminal + logger */
    terminal_init(opts.no_color);
    logger_init(config.log_file, LOG_INFO);

    logger_info("Monitor started (interval %.0fs, config %s)",
                interval, opts.config_path);

    /* Signals */
    install_signal_handlers();

    /* Monitor engine */
    if (monitor_init(&ctx, &config, &g_running) != 0) {
        fprintf(stderr, "Failed to initialize monitor\n");
        logger_close();
        return 1;
    }

    if (!opts.json) {
        cli_print_version(stdout);
        printf("Monitoring interval: %.0f seconds\n", interval);
        printf("Configuration: %s\n", opts.config_path);
    }

    /* Main loop */
    while (g_running) {
        if (monitor_collect(&ctx, &snap) == 0) {
            monitor_evaluate(&ctx, &snap);
            if (opts.json) {
                print_json(&ctx, &snap);
            } else {
                print_dashboard(&ctx, &snap);
                csv_append(&config, &snap);
            }
        }

        if (opts.once || opts.json) {
            break; /* single-run mode, or JSON emits one snapshot */
        }

        /* Sleep in small chunks so SIGINT is noticed promptly. */
        {
            double remaining = interval;
            while (remaining > 0.0 && g_running) {
                long ms = (remaining >= 0.1) ? 100L : (long)(remaining * 1000.0);
                if (ms <= 0) {
                    ms = 1;
                }
                sleep_millis(ms);
                remaining -= (double)ms / 1000.0;
            }
        }
    }

    printf("\n");
    logger_info("Stopping Linux Resource Monitor...");

    logger_close();
    if (!opts.json) {
        printf("Monitor stopped successfully.\n");
    }
    return 0;
}
