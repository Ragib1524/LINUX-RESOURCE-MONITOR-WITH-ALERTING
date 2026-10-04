/*
 * include/disk.h
 *
 * Disk monitor: uses the POSIX statvfs() system call (not a shell 'df').
 *
 * statvfs() fills a struct with filesystem statistics; we compute total,
 * available, used, and a usage percentage. The correct block-size field to
 * use for the capacity calculation is f_frsize (fragment size), with
 * f_blocks / f_bavail.
 */

#ifndef LINUX_RESOURCE_MONITOR_DISK_H
#define LINUX_RESOURCE_MONITOR_DISK_H

typedef struct {
    unsigned long long total_bytes;
    unsigned long long available_bytes;
    unsigned long long used_bytes;
    double usage_percent;
} DiskMetrics;

/*
 * disk_read()
 * Query the filesystem containing 'path' via statvfs().
 * Returns 0 on success, -1 on failure (invalid path, permission denied,
 * zero blocks, etc.).
 */
int disk_read(const char *path, DiskMetrics *d);

#endif /* LINUX_RESOURCE_MONITOR_DISK_H */
