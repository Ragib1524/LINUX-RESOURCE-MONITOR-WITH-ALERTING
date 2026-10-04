/*
 * tests/test_cpu.c
 *
 * Unit tests for CPU utilization calculation.
 *
 * These tests feed synthetic CpuTimes samples (no dependency on the live
 * /proc/stat) so the calculation is deterministic and independent of the
 * running system.
 */

#include <stdio.h>

#include "cpu.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                     \
    do {                                                     \
        if (cond) {                                          \
            printf("  PASS: %s\n", msg);                     \
        } else {                                             \
            printf("  FAIL: %s\n", msg);                     \
            g_failures++;                                    \
        }                                                    \
    } while (0)

int main(void)
{
    CpuTimes prev;
    CpuTimes curr;
    double usage;

    printf("test_cpu: basic utilization\n");
    /* prev: total 1000 (user100+system100+idle700+iowait100), idle 800.
     * curr: total 1055 (110+115+730+100), idle 830.
     * total_delta=55, idle_delta=30, busy=25 -> 45.45% */
    prev.user = 100; prev.nice = 0; prev.system = 100; prev.idle = 700;
    prev.iowait = 100; prev.irq = 0; prev.softirq = 0; prev.steal = 0;
    curr.user = 110; curr.nice = 0; curr.system = 115; curr.idle = 730;
    curr.iowait = 100; curr.irq = 0; curr.softirq = 0; curr.steal = 0;
    CHECK(cpu_calc_usage(&prev, &curr, &usage) == 0, "calc succeeds");
    CHECK(usage > 45.0 && usage < 46.0, "expected ~45.45% utilization");

    printf("test_cpu: fully idle\n");
    /* idle increases by the full delta -> 0% usage */
    prev.idle = 700; prev.iowait = 100; prev.user = 100; prev.system = 100;
    prev.nice = 0; prev.irq = 0; prev.softirq = 0; prev.steal = 0;
    curr.idle = 800; curr.iowait = 100; curr.user = 100; curr.system = 100;
    curr.nice = 0; curr.irq = 0; curr.softirq = 0; curr.steal = 0;
    CHECK(cpu_calc_usage(&prev, &curr, &usage) == 0, "fully idle calc succeeds");
    CHECK(usage < 0.01, "fully idle -> ~0%");

    printf("test_cpu: zero delta rejected\n");
    curr = prev;
    CHECK(cpu_calc_usage(&prev, &curr, &usage) == -1, "zero delta returns error");

    printf("test_cpu: backwards counters rejected\n");
    prev.idle = 500; prev.user = 500; prev.system = 0; prev.nice = 0;
    prev.iowait = 0; prev.irq = 0; prev.softirq = 0; prev.steal = 0;
    curr.idle = 400; curr.user = 400; curr.system = 0; curr.nice = 0;
    curr.iowait = 0; curr.irq = 0; curr.softirq = 0; curr.steal = 0;
    CHECK(cpu_calc_usage(&prev, &curr, &usage) == -1, "backwards counters rejected");

    printf("test_cpu: NULL argument safe\n");
    CHECK(cpu_calc_usage(NULL, &curr, &usage) == -1, "NULL prev rejected");
    CHECK(cpu_calc_usage(&prev, NULL, &usage) == -1, "NULL curr rejected");
    CHECK(cpu_calc_usage(&prev, &curr, NULL) == -1, "NULL out rejected");

    printf("test_cpu: real /proc/stat readable\n");
    {
        CpuTimes real;
        int r = cpu_read_times(&real);
        CHECK(r == 0, "cpu_read_times succeeded on real system");
        if (r == 0) {
            CHECK(cpu_total(&real) > 0, "real total jiffies > 0");
        }
    }

    printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
