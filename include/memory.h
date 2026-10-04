/*
 * include/memory.h
 *
 * Memory monitor: reads /proc/meminfo and computes memory utilization.
 *
 * Modern Linux memory accounting uses MemAvailable rather than MemFree: the
 * kernel reclaims buffered/cached pages on demand, so "used" from the
 * perspective of an application is MemTotal - MemAvailable.
 */

#ifndef LINUX_RESOURCE_MONITOR_MEMORY_H
#define LINUX_RESOURCE_MONITOR_MEMORY_H

typedef struct {
    unsigned long long total_kb;
    unsigned long long available_kb;
    unsigned long long used_kb;
    unsigned long long buffers_kb;
    unsigned long long cached_kb;
    unsigned long long swap_total_kb;
    unsigned long long swap_free_kb;
    double usage_percent;   /* used = total - available */
} MemoryMetrics;

/*
 * memory_read()
 * Read and parse /proc/meminfo into *m.
 * Returns 0 on success, -1 on failure (missing/unreadable/malformed).
 */
int memory_read(MemoryMetrics *m);

/*
 * memory_calc_usage()
 * Compute used_kb and usage_percent from total/available after a read.
 * Safe to call with a freshly read struct.
 */
void memory_calc_usage(MemoryMetrics *m);

#endif /* LINUX_RESOURCE_MONITOR_MEMORY_H */
