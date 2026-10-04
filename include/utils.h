/*
 * include/utils.h
 *
 * Utility module: common safe helpers shared across the monitor.
 */

#ifndef LINUX_RESOURCE_MONITOR_UTILS_H
#define LINUX_RESOURCE_MONITOR_UTILS_H

#include <stddef.h>

/*
 * read_file_lines()
 * Read up to max_lines lines from path into lines (each 256 bytes).
 * Returns number of lines read, or -1 on error.
 */
int read_file_lines(const char *path, char (*lines)[256], int max_lines);

/*
 * trim_whitespace()
 * Strip leading/trailing whitespace in place. Returns start pointer.
 */
char *trim_whitespace(char *s);

/*
 * parse_ull_bounded()
 * Parse unsigned long long from decimal string. 0 on success, -1 on error.
 */
int parse_ull_bounded(const char *s, unsigned long long *out);

/*
 * parse_long_long()
 * Parse signed long long from decimal string. 0 on success, -1 on error.
 */
int parse_long_long(const char *s, long long *out);

/*
 * parse_double()
 * Parse double from string. 0 on success, -1 on error.
 */
int parse_double(const char *s, double *out);

/*
 * monotonic_seconds()
 * Monotonic timestamp in seconds (CLOCK_MONOTONIC).
 */
double monotonic_seconds(void);

/*
 * sleep_millis()
 * Sleep for millis using nanosleep; handles EINTR. 0 on success.
 */
int sleep_millis(long millis);

#endif /* LINUX_RESOURCE_MONITOR_UTILS_H */
