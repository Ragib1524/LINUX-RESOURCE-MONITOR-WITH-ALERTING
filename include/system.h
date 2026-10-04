/*
 * include/system.h
 *
 * System monitor: load average and uptime.
 *
 * Load average is read from /proc/loadavg (three floating-point values: the
 * 1-, 5-, and 15-minute exponentially-damped run-queue averages). Uptime is
 * read from /proc/uptime (seconds since boot).
 *
 * Note: load average is NOT a CPU percentage. It is a count of runnable (and
 * uninterruptible) tasks, so it should be compared against the number of
 * logical CPUs, not against 100.
 */

#ifndef LINUX_RESOURCE_MONITOR_SYSTEM_H
#define LINUX_RESOURCE_MONITOR_SYSTEM_H

typedef struct {
    double load_1;
    double load_5;
    double load_15;
    double uptime_seconds;
} SystemMetrics;

/*
 * system_read()
 * Read load average (getloadavg) and uptime (/proc/uptime) into *s.
 * Returns 0 on success, -1 on failure.
 */
int system_read(SystemMetrics *s);

#endif /* LINUX_RESOURCE_MONITOR_SYSTEM_H */
