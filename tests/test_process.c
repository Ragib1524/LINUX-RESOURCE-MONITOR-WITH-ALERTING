/*
 * tests/test_process.c
 *
 * Unit tests for the process monitor: real /proc scan plus sort correctness.
 */

#include <stdio.h>
#include <string.h>

#include "process.h"

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
    ProcessInfo list[PROCESS_MAX];
    int n;

    printf("test_process: real scan\n");
    n = process_scan(list, PROCESS_MAX);
    CHECK(n > 0, "process_scan found processes");
    printf("  (found %d processes)\n", n);

    if (n > 0) {
        CHECK(list[0].pid > 0, "PID is positive");
        CHECK(list[0].name[0] != '\0', "name is non-empty");
        CHECK(list[0].memory_kb <= (unsigned long long)1 << 40, "memory is plausible");
    }

    printf("test_process: sort by memory descending\n");
    if (n >= 2) {
        process_sort_by_memory(list, n);
        CHECK(list[0].memory_kb >= list[n - 1].memory_kb,
              "first has >= memory than last after sort");
    } else {
        printf("  (skip: not enough processes)\n");
    }

    printf("test_process: NULL/edge safe\n");
    CHECK(process_scan(NULL, 10) == -1, "NULL list rejected");
    CHECK(process_scan(list, 0) == -1, "zero capacity rejected");

    printf("test_process: per-process CPU%% calculation\n");
    {
        ProcessInfo prev[2];
        ProcessInfo curr[2];
        memset(prev, 0, sizeof(prev));
        memset(curr, 0, sizeof(curr));
        prev[0].pid = 100;
        prev[0].utime_jiffies = 10;
        prev[0].stime_jiffies = 5;
        prev[1].pid = 200;
        prev[1].utime_jiffies = 100;
        prev[1].stime_jiffies = 0;

        curr[0].pid = 100;
        curr[0].utime_jiffies = 20;   /* +10 jiffies */
        curr[0].stime_jiffies = 10;   /* +5 jiffies  -> +15 total */
        curr[1].pid = 200;
        curr[1].utime_jiffies = 100;  /* unchanged */
        curr[1].stime_jiffies = 0;

        /* total_delta = 100 busy jiffies; process 100 used 15 -> 15% */
        process_calc_cpu_percent(prev, 2, curr, 2, 100ULL);
        CHECK(curr[0].cpu_percent > 14.9 && curr[0].cpu_percent < 15.1,
              "process 100 => 15%% CPU");
        CHECK(curr[1].cpu_percent < 0.01, "process 200 => 0%% CPU");
    }

    printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
