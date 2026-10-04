/*
 * tests/test_memory.c
 *
 * Unit tests for memory monitoring: /proc/meminfo parsing and the
 * usage calculation, plus the real system read.
 */

#include <stdio.h>
#include <string.h>

#include "memory.h"

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
    MemoryMetrics m;

    printf("test_memory: usage calculation\n");
    memset(&m, 0, sizeof(m));
    m.total_kb = 10000;
    m.available_kb = 4000;
    memory_calc_usage(&m);
    CHECK(m.used_kb == 6000, "used = total - available");
    CHECK(m.usage_percent > 59.9 && m.usage_percent < 60.1, "usage == 60%");

    printf("test_memory: available clamped to total\n");
    memset(&m, 0, sizeof(m));
    m.total_kb = 1000;
    m.available_kb = 5000; /* impossible */
    memory_calc_usage(&m);
    CHECK(m.used_kb == 0, "clamped => used 0");
    CHECK(m.usage_percent == 0.0, "clamped => 0%");

    printf("test_memory: zero total safe\n");
    memset(&m, 0, sizeof(m));
    memory_calc_usage(&m);
    CHECK(m.usage_percent == 0.0, "no divide-by-zero on empty struct");

    printf("test_memory: real /proc/meminfo\n");
    CHECK(memory_read(&m) == 0, "memory_read succeeds");
    if (m.total_kb > 0) {
        CHECK(m.total_kb > 0, "real MemTotal > 0");
        CHECK(m.available_kb <= m.total_kb, "available <= total");
        CHECK(m.usage_percent >= 0.0 && m.usage_percent <= 100.0, "usage in [0,100]");
        printf("  (real: %llu KB total, %.1f%% used)\n",
               m.total_kb, m.usage_percent);
    }

    printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
