/*
 * include/cpu.h
 *
 * CPU monitor: reads /proc/stat and computes CPU utilization.
 *
 * CPU utilization cannot be measured from a single /proc/stat sample: the
 * file reports *cumulative* jiffies since boot. Utilization is therefore the
 * *difference* between two samples taken at an interval:
 *
 *   total_delta = current_total - previous_total
 *   idle_delta  = current_idle  - previous_idle
 *   usage%      = ((total_delta - idle_delta) / total_delta) * 100
 *
 * The "idle" figure should include idle + iowait (time the CPU had nothing
 * runnable to do). The module keeps two samples internally.
 */

#ifndef LINUX_RESOURCE_MONITOR_CPU_H
#define LINUX_RESOURCE_MONITOR_CPU_H

/*
 * Raw CPU counters from /proc/stat's aggregate "cpu" line.
 * All values are in jiffies (USER_HZ ticks).
 */
typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} CpuTimes;

/* Computed utilization between two samples. */
typedef struct {
    double usage_percent;          /* 0..100 */
    unsigned long long total_delta;
    unsigned long long idle_delta;
} CpuMetrics;

/*
 * cpu_read_times()
 * Read the aggregate "cpu" line from /proc/stat into *times.
 * Returns 0 on success, -1 on failure (missing/unreadable/malformed file).
 */
int cpu_read_times(CpuTimes *times);

/*
 * cpu_calc_usage()
 * Compute utilization between 'prev' and 'curr'.
 * Returns 0 on success, -1 if the delta is zero or invalid (e.g. counters
 * went backwards). Stores usage percent (0..100) in *usage_percent.
 */
int cpu_calc_usage(const CpuTimes *prev, const CpuTimes *curr, double *usage_percent);

/*
 * cpu_total()
 * Return the sum of all jiffies in a CpuTimes sample (with overflow guard).
 */
unsigned long long cpu_total(const CpuTimes *t);

/*
 * cpu_idle()
 * Return idle + iowait jiffies (time the CPU had nothing to do).
 */
unsigned long long cpu_idle(const CpuTimes *t);

#endif /* LINUX_RESOURCE_MONITOR_CPU_H */
