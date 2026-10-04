/*
 * src/cpu.c
 *
 * CPU monitoring via /proc/stat.
 *
 * /proc/stat's first line looks like:
 *   cpu  user nice system idle iowait irq softirq steal guest guest_nice
 *
 * We parse the first eight numeric fields after "cpu". The values are
 * cumulative jiffies since boot, so utilization requires two samples.
 */

#include "cpu.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

#define PROC_STAT "/proc/stat"

int cpu_read_times(CpuTimes *times)
{
    FILE *fp;
    char line[512];
    int fields;

    if (times == NULL) {
        return -1;
    }

    fp = fopen(PROC_STAT, "r");
    if (fp == NULL) {
        return -1;
    }

    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    /* Expect line to start with "cpu ". */
    if (strncmp(line, "cpu ", 4) != 0) {
        return -1;
    }

    fields = sscanf(line + 4,
                    "%llu %llu %llu %llu %llu %llu %llu %llu",
                    &times->user, &times->nice, &times->system, &times->idle,
                    &times->iowait, &times->irq, &times->softirq, &times->steal);

    if (fields < 8) {
        return -1;
    }

    return 0;
}

unsigned long long cpu_total(const CpuTimes *t)
{
    if (t == NULL) {
        return 0;
    }
    /* Sum can overflow in theory; saturate rather than wrap. */
    unsigned long long sum = 0;
    unsigned long long x;

#define ADD(v)                                                         \
    x = (t)->v;                                                        \
    if (sum > 18446744073709551615ULL - x) {                           \
        sum = 18446744073709551615ULL;                                 \
    } else {                                                           \
        sum += x;                                                      \
    }

    ADD(user); ADD(nice); ADD(system); ADD(idle);
    ADD(iowait); ADD(irq); ADD(softirq); ADD(steal);
#undef ADD
    return sum;
}

unsigned long long cpu_idle(const CpuTimes *t)
{
    if (t == NULL) {
        return 0;
    }
    /* idle + iowait = time CPU had no runnable work. */
    if (t->idle > 18446744073709551615ULL - t->iowait) {
        return 18446744073709551615ULL;
    }
    return t->idle + t->iowait;
}

int cpu_calc_usage(const CpuTimes *prev, const CpuTimes *curr, double *usage_percent)
{
    unsigned long long prev_total;
    unsigned long long curr_total;
    unsigned long long prev_idle;
    unsigned long long curr_idle;
    unsigned long long total_delta;
    unsigned long long idle_delta;
    unsigned long long busy_delta;

    if (prev == NULL || curr == NULL || usage_percent == NULL) {
        return -1;
    }

    prev_total = cpu_total(prev);
    curr_total = cpu_total(curr);
    prev_idle = cpu_idle(prev);
    curr_idle = cpu_idle(curr);

    /* Counters went backwards or did not advance: invalid sample. */
    if (curr_total <= prev_total) {
        return -1;
    }

    total_delta = curr_total - prev_total;
    if (total_delta == 0) {
        return -1;
    }

    idle_delta = (curr_idle >= prev_idle) ? (curr_idle - prev_idle) : 0;
    busy_delta = (total_delta > idle_delta) ? (total_delta - idle_delta) : 0;

    *usage_percent = ((double)busy_delta / (double)total_delta) * 100.0;
    return 0;
}
