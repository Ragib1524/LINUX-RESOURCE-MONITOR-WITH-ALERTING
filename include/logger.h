/*
 * include/logger.h
 *
 * Reusable, thread-safe-enough (single-threaded here) logging facility.
 *
 * Levels: DEBUG < INFO < WARNING < ERROR < CRITICAL.
 * A global minimum level filters messages; a lower level (e.g. DEBUG) enables
 * more output than a higher one (e.g. ERROR).
 *
 * The logger owns an optional FILE* opened by logger_init(); logger_close()
 * releases it. All formatting uses bounded vsnprintf into a fixed buffer, so
 * no unsafe string operations are involved.
 */

#ifndef LINUX_RESOURCE_MONITOR_LOGGER_H
#define LINUX_RESOURCE_MONITOR_LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
} LogLevel;

/*
 * logger_init()
 * Open the log file at 'path' (append mode). If 'path' is NULL or cannot be
 * opened, logging falls back to stderr only. 'min_level' filters messages.
 * Returns 0 on success.
 */
int logger_init(const char *path, LogLevel min_level);

/*
 * logger_set_min_level()
 * Change the minimum level at runtime.
 */
void logger_set_min_level(LogLevel level);

/*
 * logger_log()
 * Emit a message at 'level' with printf-style formatting. The message is
 * prefixed with a timestamp and the level name.
 */
void logger_log(LogLevel level, const char *fmt, ...);

void logger_debug(const char *fmt, ...);
void logger_info(const char *fmt, ...);
void logger_warning(const char *fmt, ...);
void logger_error(const char *fmt, ...);
void logger_critical(const char *fmt, ...);

/*
 * logger_close()
 * Flush and close the log file (if open). Safe to call more than once.
 */
void logger_close(void);

#endif /* LINUX_RESOURCE_MONITOR_LOGGER_H */
