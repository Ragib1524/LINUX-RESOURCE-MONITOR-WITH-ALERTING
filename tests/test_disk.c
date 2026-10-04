/*
 * tests/test_disk.c
 *
 * Unit tests for disk monitoring via statvfs(): real mount point query plus
 * failure cases (invalid path, empty path, NULL args).
 */

#include <stdio.h>

#include "disk.h"

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
    DiskMetrics d;

    printf("test_disk: real filesystem\n");
    CHECK(disk_read("/", &d) == 0, "statvfs('/') succeeds");
    if (d.total_bytes > 0) {
        CHECK(d.total_bytes > 0, "total > 0");
        CHECK(d.available_bytes <= d.total_bytes, "available <= total");
        CHECK(d.usage_percent >= 0.0 && d.usage_percent <= 100.0, "usage in [0,100]");
        printf("  (real: %.2f%% used, %llu bytes total)\n",
               d.usage_percent, d.total_bytes);
    }

    printf("test_disk: invalid path\n");
    CHECK(disk_read("/nonexistent/path/xyz", &d) == -1, "invalid path -> error");

    printf("test_disk: empty path\n");
    CHECK(disk_read("", &d) == -1, "empty path -> error");

    printf("test_disk: NULL args\n");
    CHECK(disk_read(NULL, &d) == -1, "NULL path -> error");
    CHECK(disk_read("/", NULL) == -1, "NULL out -> error");

    printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
