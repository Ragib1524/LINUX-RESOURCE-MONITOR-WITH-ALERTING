/*
 * src/disk.c
 *
 * Disk monitoring via statvfs().
 *
 * statvfs() is a POSIX system call that returns filesystem statistics without
 * spawning a subprocess, unlike the 'df' utility. We use:
 *   total     = f_blocks  * f_frsize
 *   available = f_bavail  * f_frsize     (space available to unprivileged)
 *   used      = total - available
 */

#include "disk.h"

#include <stddef.h>
#include <string.h>
#include <sys/statvfs.h>

int disk_read(const char *path, DiskMetrics *d)
{
    struct statvfs st;
    unsigned long long total;
    unsigned long long avail;

    if (path == NULL || d == NULL || *path == '\0') {
        return -1;
    }

    memset(d, 0, sizeof(*d));

    if (statvfs(path, &st) != 0) {
        return -1;
    }

    if (st.f_blocks == 0 || st.f_frsize == 0) {
        return -1; /* Degenerate filesystem: avoid divide-by-zero. */
    }

    total = (unsigned long long)st.f_blocks * (unsigned long long)st.f_frsize;
    avail = (unsigned long long)st.f_bavail * (unsigned long long)st.f_frsize;

    d->total_bytes = total;
    d->available_bytes = avail;
    d->used_bytes = (total > avail) ? (total - avail) : 0;
    d->usage_percent = ((double)d->used_bytes / (double)total) * 100.0;

    return 0;
}
