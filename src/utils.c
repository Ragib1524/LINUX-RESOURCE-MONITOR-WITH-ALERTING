/*
 * src/utils.c
 *
 * Safe utility helpers. These are deliberately dependency-free and defensive:
 * a resource monitor consumes kernel-exposed text that may be malformed,
 * truncated, or missing fields, so every parser here fails softly.
 */

#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int read_file_lines(const char *path, char (*lines)[256], int max_lines)
{
    FILE *fp;
    int count = 0;

    if (path == NULL || lines == NULL || max_lines <= 0) {
        return -1;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    while (count < max_lines && fgets(lines[count], 256, fp) != NULL) {
        /* Strip trailing newline and carriage return. */
        size_t len = strlen(lines[count]);
        while (len > 0 &&
               (lines[count][len - 1] == '\n' ||
                lines[count][len - 1] == '\r')) {
            lines[count][len - 1] = '\0';
            len--;
        }
        count++;
    }

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return count;
}

char *trim_whitespace(char *s)
{
    char *end;

    if (s == NULL) {
        return NULL;
    }

    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }

    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';

    return s;
}

int parse_ull_bounded(const char *s, unsigned long long *out)
{
    unsigned long long value = 0;
    const unsigned char *p;

    if (s == NULL || out == NULL || *s == '\0') {
        return -1;
    }

    for (p = (const unsigned char *)s; *p != '\0'; p++) {
        unsigned long long digit;
        if (*p < '0' || *p > '9') {
            return -1;
        }
        digit = (unsigned long long)(*p - '0');
        if (value > (18446744073709551615ULL - digit) / 10ULL) {
            return -1;
        }
        value = value * 10ULL + digit;
    }

    *out = value;
    return 0;
}

int parse_long_long(const char *s, long long *out)
{
    long long value = 0;
    int negative = 0;
    const unsigned char *p;
    int saw_digit = 0;

    if (s == NULL || out == NULL || *s == '\0') {
        return -1;
    }

    p = (const unsigned char *)s;
    if (*p == '-') {
        negative = 1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    if (*p == '\0') {
        return -1;
    }

    for (; *p != '\0'; p++) {
        long long digit;
        if (*p < '0' || *p > '9') {
            return -1;
        }
        saw_digit = 1;
        digit = (long long)(*p - '0');
        if (negative) {
            if (value < (-9223372036854775807LL - 1 + digit) / 10LL) {
                return -1;
            }
            value = value * 10LL - digit;
        } else {
            if (value > (9223372036854775807LL - digit) / 10LL) {
                return -1;
            }
            value = value * 10LL + digit;
        }
    }

    if (!saw_digit) {
        return -1;
    }

    *out = value;
    return 0;
}

int parse_double(const char *s, double *out)
{
    double result = 0.0;
    double fraction = 0.0;
    double divisor = 1.0;
    int negative = 0;
    const unsigned char *p = (const unsigned char *)s;
    int in_fraction = 0;
    int saw_digit = 0;

    if (s == NULL || out == NULL || *s == '\0') {
        return -1;
    }

    if (*p == '-') {
        negative = 1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    for (; *p != '\0'; p++) {
        if (*p == '.') {
            if (in_fraction) {
                return -1;
            }
            in_fraction = 1;
            continue;
        }
        if (*p < '0' || *p > '9') {
            return -1;
        }
        saw_digit = 1;
        if (in_fraction) {
            fraction = fraction * 10.0 + (double)(*p - '0');
            divisor *= 10.0;
        } else {
            result = result * 10.0 + (double)(*p - '0');
        }
    }

    if (!saw_digit) {
        return -1;
    }

    result += fraction / divisor;
    if (negative) {
        result = -result;
    }

    *out = result;
    return 0;
}

double monotonic_seconds(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

int sleep_millis(long millis)
{
    struct timespec req;
    struct timespec rem;

    if (millis <= 0) {
        return 0;
    }

    req.tv_sec = millis / 1000;
    req.tv_nsec = (millis % 1000) * 1000000L;

    while (nanosleep(&req, &rem) != 0) {
        if (errno == EINTR) {
            req = rem;
            continue;
        }
        return -1;
    }
    return 0;
}
