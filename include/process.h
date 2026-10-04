/*
 * include/process.h
 *
 * Process monitor: scans /proc for running processes and reports resource
 * usage. This is pure Linux systems programming: we walk /proc/<pid>/, parse
 * /proc/<pid>/stat (the trickiest proc format) and /proc/<pid>/status.
 *
 * Critical difficulty: /proc/<pid>/stat's second field (the process name in
 * parentheses) may itself contain spaces AND parentheses, so naive whitespace
 * splitting is wrong. We locate the LAST ')' to find the name's end and parse
 * the numeric fields that follow it.
 */

#ifndef LINUX_RESOURCE_MONITOR_PROCESS_H
#define LINUX_RESOURCE_MONITOR_PROCESS_H

#define PROCESS_MAX 4096            /* upper bound on processes we will track */
#define PROCESS_NAME_MAX 256

typedef struct {
    int pid;
    char name[PROCESS_NAME_MAX];
    unsigned long long utime_jiffies;   /* user CPU jiffies */
    unsigned long long stime_jiffies;   /* system CPU jiffies */
    unsigned long long memory_kb;       /* resident set size in KiB */
    double cpu_percent;                 /* computed across two samples */
} ProcessInfo;

/*
 * process_scan()
 * Scan /proc and fill 'list' (caller-provided array of 'capacity' entries).
 * Returns the number of processes found, or -1 on fatal error.
 * Processes that disappear mid-scan are silently skipped (expected and safe).
 */
int process_scan(ProcessInfo *list, int capacity);

/*
 * process_sort_by_memory()
 * Sort 'list' in-place (descending memory). n is the count.
 */
void process_sort_by_memory(ProcessInfo *list, int n);

/*
 * process_sort_by_cpu()
 * Sort 'list' in-place (descending CPU percent). n is the count.
 */
void process_sort_by_cpu(ProcessInfo *list, int n);

/*
 * process_calc_cpu_percent()
 * Compute per-process CPU% for 'curr' by comparing against the previous
 * sample 'prev' (matched by PID). The total CPU time delta is 'total_delta'
 * (sum of busy jiffies across all CPUs between the two samples).
 *
 *   cpu% = ((utime+stime) delta of this process) / total_delta * 100
 *
 * Processes present in only one sample get 0%. Processes that vanished are
 * simply absent from curr. Returns 0 on success.
 */
int process_calc_cpu_percent(const ProcessInfo *prev, int prev_count,
                             ProcessInfo *curr, int curr_count,
                             unsigned long long total_delta);

#endif /* LINUX_RESOURCE_MONITOR_PROCESS_H */
