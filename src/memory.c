/*
 * src/memory.c
 *
 * Memory monitoring via /proc/meminfo.
 *
 * /proc/meminfo is a key:value file with a "kB" suffix on each value, e.g.:
 *   MemTotal:       16384000 kB
 *   MemAvailable:   12000000 kB
 *
 * We parse the numeric value before the "kB" suffix. Modern memory
 * utilization uses MemAvailable (kernel's estimate of memory a new
 * allocation can obtain without swapping), not MemFree.
 */

#include "memory.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

#define PROC_MEMINFO "/proc/meminfo"

static int parse_kb_line(const char *line, unsigned long long *out)
{
    /* Line format: "Name:   <number> kB". The numeric value is followed by
     * a space and the "kB" suffix, so we stop parsing at the first non-digit
     * rather than letting parse_ull_bounded reject the trailing text. */
    const char *colon = strchr(line, ':');
    const char *p;
    unsigned long long val;
    char num[64];
    size_t i = 0;

    if (colon == NULL || out == NULL) {
        return -1;
    }
    p = colon + 1;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    /* Copy the leading digit run into a bounded buffer. */
    while (*p >= '0' && *p <= '9' && i < sizeof(num) - 1) {
        num[i++] = *p++;
    }
    num[i] = '\0';
    if (i == 0) {
        return -1;
    }
    if (parse_ull_bounded(num, &val) != 0) {
        return -1;
    }
    *out = val;
    return 0;
}

int memory_read(MemoryMetrics *m)
{
    FILE *fp;
    char line[256];

    if (m == NULL) {
        return -1;
    }

    /* Zero the struct so missing fields stay 0. */
    memset(m, 0, sizeof(*m));

    fp = fopen(PROC_MEMINFO, "r");
    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            parse_kb_line(line, &m->total_kb);
        } else if (strncmp(line, "MemAvailable:", 13) == 0) {
            parse_kb_line(line, &m->available_kb);
        } else if (strncmp(line, "Buffers:", 8) == 0) {
            parse_kb_line(line, &m->buffers_kb);
        } else if (strncmp(line, "Cached:", 7) == 0) {
            parse_kb_line(line, &m->cached_kb);
        } else if (strncmp(line, "SwapTotal:", 10) == 0) {
            parse_kb_line(line, &m->swap_total_kb);
        } else if (strncmp(line, "SwapFree:", 9) == 0) {
            parse_kb_line(line, &m->swap_free_kb);
        }
    }

    fclose(fp);

    /* Required fields must be present and plausible. */
    if (m->total_kb == 0) {
        return -1;
    }

    memory_calc_usage(m);
    return 0;
}

void memory_calc_usage(MemoryMetrics *m)
{
    if (m == NULL || m->total_kb == 0) {
        return;
    }
    if (m->available_kb > m->total_kb) {
        m->available_kb = m->total_kb; /* Clamp for safety. */
    }
    m->used_kb = m->total_kb - m->available_kb;
    m->usage_percent = ((double)m->used_kb / (double)m->total_kb) * 100.0;
}
