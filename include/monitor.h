/*
 * include/monitor.h
 *
 * Central monitoring engine: aggregates all subsystem metrics into a single
 * snapshot and runs the main loop. This module ties the layered architecture
 * together (CLI -> engine -> subsystem monitors -> threshold eval -> alerts).
 */

#ifndef LINUX_RESOURCE_MONITOR_MONITOR_H
#define LINUX_RESOURCE_MONITOR_MONITOR_H

#include "alert.h"
#include "config.h"
#include "cpu.h"
#include "disk.h"
#include "memory.h"
#include "network.h"
#include "process.h"
#include "system.h"

/*
 * A single collected snapshot of the whole system, plus the alert states for
 * the numeric resources.
 */
typedef struct {
    CpuMetrics cpu;
    MemoryMetrics memory;
    DiskMetrics disk;
    SystemMetrics system;
    NetworkSnapshot network;
    ProcessInfo processes[PROCESS_MAX];
    int process_count;
} SystemMetricsSnapshot;

/*
 * Runtime context for the monitor. Owns configuration, alert states, and
 * baseline samples (for CPU/network which require two samples).
 */
typedef struct {
    MonitorConfig config;
    AlertState cpu_alert;
    AlertState memory_alert;
    AlertState disk_alert;
    AlertState load_alert;
    int have_cpu_baseline;
    CpuTimes cpu_prev;
    int have_net_baseline;
    NetworkSnapshot net_prev;
    /* Per-process CPU% requires two scans and the CPU busy-delta. */
    int have_proc_baseline;
    ProcessInfo proc_prev[PROCESS_MAX];
    int proc_prev_count;
    unsigned long long cpu_busy_delta;
    volatile int *running;      /* points to the signal-controlled flag */
} MonitorContext;

/*
 * monitor_init()
 * Initialize ctx from config and install baselines. Returns 0 on success.
 */
int monitor_init(MonitorContext *ctx, const MonitorConfig *config,
                 volatile int *running);

/*
 * monitor_collect()
 * Collect one full snapshot into snap, computing CPU/network rates using the
 * stored baselines. Returns 0 on success. Subsystem failures are non-fatal
 * (the snapshot simply carries zeroed values for the failed subsystem).
 */
int monitor_collect(MonitorContext *ctx, SystemMetricsSnapshot *snap);

/*
 * monitor_evaluate()
 * Run threshold evaluation against the snapshot and emit/log alerts. Returns
 * the number of alert events emitted this cycle.
 */
int monitor_evaluate(MonitorContext *ctx, const SystemMetricsSnapshot *snap);

#endif /* LINUX_RESOURCE_MONITOR_MONITOR_H */
