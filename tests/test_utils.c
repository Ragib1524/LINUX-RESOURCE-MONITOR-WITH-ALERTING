/*
 * tests/test_utils.c
 *
 * Unit tests for the utility module: parsers, trimmer, bounded file reader.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

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
    unsigned long long ull;
    long long ll;
    double d;

    printf("test_utils: parse_ull_bounded\n");
    CHECK(parse_ull_bounded("12345", &ull) == 0 && ull == 12345ULL, "parse 12345");
    CHECK(parse_ull_bounded("0", &ull) == 0 && ull == 0ULL, "parse 0");
    CHECK(parse_ull_bounded("18446744073709551615", &ull) == 0, "parse ULLONG_MAX");
    CHECK(parse_ull_bounded("18446744073709551616", &ull) == -1, "reject overflow");
    CHECK(parse_ull_bounded("", &ull) == -1, "reject empty");
    CHECK(parse_ull_bounded("12a", &ull) == -1, "reject non-digit");
    CHECK(parse_ull_bounded("-5", &ull) == -1, "reject negative");
    CHECK(parse_ull_bounded(NULL, &ull) == -1, "reject NULL string");

    printf("test_utils: parse_long_long\n");
    CHECK(parse_long_long("42", &ll) == 0 && ll == 42LL, "parse 42");
    CHECK(parse_long_long("-42", &ll) == 0 && ll == -42LL, "parse -42");
    CHECK(parse_long_long("9223372036854775807", &ll) == 0, "parse LLONG_MAX");
    CHECK(parse_long_long("-9223372036854775808", &ll) == 0, "parse LLONG_MIN");
    CHECK(parse_long_long("9223372036854775808", &ll) == -1, "reject LLONG overflow");
    CHECK(parse_long_long("", &ll) == -1, "reject empty");
    CHECK(parse_long_long("12x", &ll) == -1, "reject trailing junk");

    printf("test_utils: parse_double\n");
    CHECK(parse_double("42.5", &d) == 0 && d > 42.4 && d < 42.6, "parse 42.5");
    CHECK(parse_double("-3.25", &d) == 0 && d < -3.24 && d > -3.26, "parse -3.25");
    CHECK(parse_double("0", &d) == 0 && d == 0.0, "parse 0");
    CHECK(parse_double("100", &d) == 0 && d == 100.0, "parse 100");
    CHECK(parse_double("1.2.3", &d) == -1, "reject double dot");
    CHECK(parse_double("abc", &d) == -1, "reject non-numeric");
    CHECK(parse_double("", &d) == -1, "reject empty");

    printf("test_utils: trim_whitespace\n");
    {
        char s1[] = "  hello  ";
        CHECK(strcmp(trim_whitespace(s1), "hello") == 0, "trim both ends");
        char s2[] = "no_space";
        CHECK(strcmp(trim_whitespace(s2), "no_space") == 0, "no-op when clean");
        char s3[] = "\t\n x \t";
        CHECK(strcmp(trim_whitespace(s3), "x") == 0, "trim tabs/newlines");
        char s4[] = "   ";
        CHECK(strcmp(trim_whitespace(s4), "") == 0, "all whitespace -> empty");
    }

    printf("test_utils: read_file_lines\n");
    {
        char path[256];
        char lines[8][256];
        FILE *fp;
        snprintf(path, sizeof(path), "/tmp/lrm_utils_%d.txt", (int)getpid());
        fp = fopen(path, "w");
        fputs("line1\nline2\nline3\n", fp);
        fclose(fp);

        CHECK(read_file_lines(path, lines, 8) == 3, "read 3 lines");
        CHECK(strcmp(lines[0], "line1") == 0, "line1 correct");
        CHECK(strcmp(lines[2], "line3") == 0, "line3 correct");
        CHECK(read_file_lines(path, lines, 2) == 2, "respect max_lines");
        CHECK(read_file_lines("/tmp/does_not_exist_xyz", lines, 8) == -1, "missing file -> -1");
    }

    printf("test_utils: monotonic_seconds increases\n");
    {
        double a = monotonic_seconds();
        sleep_millis(2);
        double b = monotonic_seconds();
        CHECK(b >= a, "monotonic clock does not go backwards");
    }

    printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
