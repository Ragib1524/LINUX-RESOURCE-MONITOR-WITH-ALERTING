/*
 * src/logger.c
 *
 * Logging implementation. A single FILE* (log file) plus a fallback to
 * stderr. All formatting is via bounded vsnprintf.
 */

#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define LOG_MSG_MAX 1024

static FILE *g_log_file = NULL;   /* NULL means "not open / use stderr only" */
static LogLevel g_min_level = LOG_INFO;

static const char *level_name(LogLevel level)
{
    switch (level) {
    case LOG_DEBUG:    return "DEBUG";
    case LOG_INFO:     return "INFO";
    case LOG_WARNING:  return "WARNING";
    case LOG_ERROR:    return "ERROR";
    case LOG_CRITICAL: return "CRITICAL";
    default:           return "UNKNOWN";
    }
}

int logger_init(const char *path, LogLevel min_level)
{
    g_min_level = min_level;

    if (path == NULL || *path == '\0') {
        g_log_file = NULL;
        return 0;
    }

    g_log_file = fopen(path, "a");
    if (g_log_file == NULL) {
        /* Fall back to stderr; logging still works. */
        g_log_file = NULL;
        return -1;
    }

    return 0;
}

void logger_set_min_level(LogLevel level)
{
    g_min_level = level;
}

void logger_log(LogLevel level, const char *fmt, ...)
{
    char msg[LOG_MSG_MAX];
    char line[LOG_MSG_MAX + 64];
    va_list ap;
    time_t now;
    struct tm tm_info;
    char ts[32];

    if (level < g_min_level) {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    now = time(NULL);
    if (now != (time_t)-1 && localtime_r(&now, &tm_info) != NULL) {
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_info);
    } else {
        strcpy(ts, "0000-00-00 00:00:00");
    }

    snprintf(line, sizeof(line), "%s [%s] %s\n", ts, level_name(level), msg);

    /* Write to the log file (if open) AND to stderr. */
    if (g_log_file != NULL) {
        fputs(line, g_log_file);
        fflush(g_log_file);
    }
    fputs(line, stderr);
}

void logger_debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    /* Delegate via a small wrapper to keep varargs single-vararg. */
    {
        char msg[LOG_MSG_MAX];
        vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        logger_log(LOG_DEBUG, "%s", msg);
    }
}

void logger_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    {
        char msg[LOG_MSG_MAX];
        vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        logger_log(LOG_INFO, "%s", msg);
    }
}

void logger_warning(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    {
        char msg[LOG_MSG_MAX];
        vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        logger_log(LOG_WARNING, "%s", msg);
    }
}

void logger_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    {
        char msg[LOG_MSG_MAX];
        vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        logger_log(LOG_ERROR, "%s", msg);
    }
}

void logger_critical(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    {
        char msg[LOG_MSG_MAX];
        vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        logger_log(LOG_CRITICAL, "%s", msg);
    }
}

void logger_close(void)
{
    if (g_log_file != NULL) {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = NULL;
    }
}
