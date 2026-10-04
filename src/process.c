/*
 * src/process.c
 *
 * Process enumeration via /proc.
 *
 * We open /proc and iterate numeric directory names. For each numeric PID we
 * read /proc/<pid>/stat and /proc/<pid>/status. Processes may exit at any
 * instant, so every /proc access must tolerate ENOENT and simply skip.
 *
 * /proc/<pid>/stat layout (fields after the comm string, 1-based from the
 * start of the whole line):
 *   pid (1)  (comm) (2)  state (3)  ppid (4) ... utime (14) stime (15) ...
 *   ... vsize (23) rss (24) ...
 *
 * After locating the final ')' we parse the trailing field list; field 14 and
 * 15 (utime/stime) and 24 (rss) are what we need. rss is in pages; we convert
 * to KiB using sysconf(_SC_PAGESIZE)/1024.
 */

#include "process.h"
#include "utils.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Parse a PID's stat file into a ProcessInfo. Returns 0 on success.
 *
 * /proc/<pid>/stat layout (1-based fields):
 *   1=pid  2=(comm)  3=state  4=ppid  5=pgrp  6=session  7=tty_nr  8=tpgid
 *   9=flags 10=minflt 11=cminflt 12=majflt 13=cmajflt 14=utime 15=stime
 *   16=cutime 17=cstime 18=priority 19=nice 20=num_threads 21=itrealvalue
 *   22=starttime 23=vsize 24=rss
 *
 * The comm field (2) may contain spaces and parentheses, so we locate it as
 * the substring between the first '(' and the LAST ')', then parse the
 * remaining 22 whitespace-delimited fields after the final ')'. */
static int process_parse_stat(int pid, ProcessInfo *info)
{
    char path[64];
    char line[1024];
    FILE *fp;
    char *open_paren;
    char *close_paren;
    char *p;
    char state;
    long long ppid, pgrp, session, tty_nr, tpgid, flags;
    long long minflt, cminflt, majflt, cmajflt;
    long long utime, stime, cutime, cstime, priority, nice_val;
    long long num_threads, itrealvalue, starttime, vsize, rss_pages;
    int name_len;
    int parsed;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1; /* process vanished */
    }
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    open_paren = strchr(line, '(');
    if (open_paren == NULL) {
        return -1;
    }
    close_paren = strrchr(line, ')');
    if (close_paren == NULL || close_paren <= open_paren) {
        return -1;
    }

    name_len = (int)(close_paren - open_paren - 1);
    if (name_len <= 0) {
        return -1;
    }
    if (name_len >= PROCESS_NAME_MAX) {
        name_len = PROCESS_NAME_MAX - 1;
    }
    memcpy(info->name, open_paren + 1, (size_t)name_len);
    info->name[name_len] = '\0';

    p = close_paren + 1;

    /* 22 fields follow the comm field (fields 3..24). Parse them all at once.
     * utime is field 14, stime 15, rss 24. */
    parsed = sscanf(p,
        " %c %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld"
        " %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
        &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
        &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime,
        &cutime, &cstime, &priority, &nice_val, &num_threads,
        &itrealvalue, &starttime, &vsize, &rss_pages);

    if (parsed < 22) {
        /* Not all 22 fields present; fall back to zeros for safety. */
        utime = stime = rss_pages = 0;
    }

    info->utime_jiffies = (unsigned long long)(utime > 0 ? utime : 0);
    info->stime_jiffies = (unsigned long long)(stime > 0 ? stime : 0);

    /* rss_pages -> KiB. */
    {
        long page_size = sysconf(_SC_PAGESIZE);
        if (rss_pages > 0 && page_size > 0) {
            info->memory_kb = (unsigned long long)rss_pages *
                              (unsigned long long)(page_size / 1024);
        } else {
            info->memory_kb = 0;
        }
    }

    info->pid = pid;
    info->cpu_percent = 0.0;

    return 0;
}

int process_scan(ProcessInfo *list, int capacity)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    if (list == NULL || capacity <= 0) {
        return -1;
    }

    dir = opendir("/proc");
    if (dir == NULL) {
        return -1;
    }

    while ((entry = readdir(dir)) != NULL && count < capacity) {
        int pid;
        ProcessInfo info;
        const char *name = entry->d_name;

        /* Only numeric directory names correspond to PIDs. */
        if (!isdigit((unsigned char)name[0])) {
            continue;
        }

        pid = atoi(name); /* bounded: numeric only + kernel PID range */
        if (pid <= 0) {
            continue;
        }

        memset(&info, 0, sizeof(info));
        if (process_parse_stat(pid, &info) != 0) {
            continue; /* process disappeared mid-scan: skip safely */
        }

        list[count++] = info;
    }

    closedir(dir);
    return count;
}

/* qsort comparator: descending memory. */
static int cmp_memory_desc(const void *a, const void *b)
{
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    if (pa->memory_kb < pb->memory_kb) {
        return 1;
    }
    if (pa->memory_kb > pb->memory_kb) {
        return -1;
    }
    return 0;
}

/* qsort comparator: descending CPU. */
static int cmp_cpu_desc(const void *a, const void *b)
{
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    if (pa->cpu_percent < pb->cpu_percent) {
        return 1;
    }
    if (pa->cpu_percent > pb->cpu_percent) {
        return -1;
    }
    return 0;
}

void process_sort_by_memory(ProcessInfo *list, int n)
{
    if (list != NULL && n > 1) {
        qsort(list, (size_t)n, sizeof(ProcessInfo), cmp_memory_desc);
    }
}

void process_sort_by_cpu(ProcessInfo *list, int n)
{
    if (list != NULL && n > 1) {
        qsort(list, (size_t)n, sizeof(ProcessInfo), cmp_cpu_desc);
    }
}

int process_calc_cpu_percent(const ProcessInfo *prev, int prev_count,
                             ProcessInfo *curr, int curr_count,
                             unsigned long long total_delta)
{
    int i;
    int j;

    if (prev == NULL || curr == NULL || total_delta == 0) {
        return -1;
    }

    for (i = 0; i < curr_count; i++) {
        /* Find the matching PID in prev. */
        for (j = 0; j < prev_count; j++) {
            if (prev[j].pid == curr[i].pid) {
                unsigned long long prev_cpu = prev[j].utime_jiffies +
                                              prev[j].stime_jiffies;
                unsigned long long curr_cpu = curr[i].utime_jiffies +
                                              curr[i].stime_jiffies;
                unsigned long long delta;

                if (curr_cpu >= prev_cpu) {
                    delta = curr_cpu - prev_cpu;
                } else {
                    delta = 0; /* counter reset / PID recycled */
                }

                curr[i].cpu_percent =
                    ((double)delta / (double)total_delta) * 100.0;
                break;
            }
        }
        /* No match: cpu_percent stays 0 (new process since last scan). */
    }

    return 0;
}
