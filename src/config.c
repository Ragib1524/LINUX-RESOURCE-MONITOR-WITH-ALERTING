/*
 * src/config.c
 *
 * Configuration parser and validator.
 *
 * The file format is a simple KEY=VALUE text format:
 *   - blank lines are ignored
 *   - lines starting with '#' are comments
 *   - whitespace around keys and values is trimmed
 *   - unknown keys are ignored (forward compatibility) but the key count is
 *     available for diagnostics
 *
 * All numeric parsing goes through the overflow-safe helpers in utils.c, so a
 * hostile or malformed value can never cause undefined behaviour.
 */

#include "config.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

/*
 * Internal helper: parse a key/value pair and assign to the correct field.
 * Returns 0 on success, -2 for unknown key, -1 for a malformed value.
 * On a value error, fills errbuf (if non-NULL) with a message naming the key.
 */
static int config_apply_pair(MonitorConfig *cfg, const char *key, const char *value,
                             char *errbuf, size_t errbuf_len)
{
    /* Helper macro: parse a double in [0,1000] (thresholds). */
#define PARSE_DOUBLE_FIELD(field)                                      \
    do {                                                               \
        double v;                                                      \
        if (parse_double(value, &v) != 0) {                            \
            if (errbuf != NULL) {                                      \
                snprintf(errbuf, errbuf_len,                           \
                         "invalid numeric value for '%s': '%s'",       \
                         key, value);                                  \
            }                                                          \
            return -1;                                                 \
        }                                                              \
        (cfg)->field = v;                                              \
    } while (0)

    if (strcmp(key, "CPU_WARNING") == 0) {
        PARSE_DOUBLE_FIELD(cpu_warning);
    } else if (strcmp(key, "CPU_CRITICAL") == 0) {
        PARSE_DOUBLE_FIELD(cpu_critical);
    } else if (strcmp(key, "MEMORY_WARNING") == 0) {
        PARSE_DOUBLE_FIELD(memory_warning);
    } else if (strcmp(key, "MEMORY_CRITICAL") == 0) {
        PARSE_DOUBLE_FIELD(memory_critical);
    } else if (strcmp(key, "DISK_WARNING") == 0) {
        PARSE_DOUBLE_FIELD(disk_warning);
    } else if (strcmp(key, "DISK_CRITICAL") == 0) {
        PARSE_DOUBLE_FIELD(disk_critical);
    } else if (strcmp(key, "LOAD_WARNING") == 0) {
        PARSE_DOUBLE_FIELD(load_warning);
    } else if (strcmp(key, "LOAD_CRITICAL") == 0) {
        PARSE_DOUBLE_FIELD(load_critical);
    } else if (strcmp(key, "MONITOR_INTERVAL") == 0) {
        long long v;
        if (parse_long_long(value, &v) != 0) {
            if (errbuf != NULL) {
                snprintf(errbuf, errbuf_len, "invalid integer for '%s': '%s'",
                         key, value);
            }
            return -1;
        }
        cfg->monitor_interval = (int)v;
    } else if (strcmp(key, "TOP_PROCESSES") == 0) {
        long long v;
        if (parse_long_long(value, &v) != 0) {
            if (errbuf != NULL) {
                snprintf(errbuf, errbuf_len, "invalid integer for '%s': '%s'",
                         key, value);
            }
            return -1;
        }
        cfg->top_processes = (int)v;
    } else if (strcmp(key, "ALERT_COOLDOWN") == 0) {
        long long v;
        if (parse_long_long(value, &v) != 0) {
            if (errbuf != NULL) {
                snprintf(errbuf, errbuf_len, "invalid integer for '%s': '%s'",
                         key, value);
            }
            return -1;
        }
        cfg->alert_cooldown = (int)v;
    } else if (strcmp(key, "MONITOR_DISK") == 0) {
        if (strlen(value) >= sizeof(cfg->disk_path)) {
            if (errbuf != NULL) {
                snprintf(errbuf, errbuf_len, "value too long for '%s'", key);
            }
            return -1;
        }
        strncpy(cfg->disk_path, value, sizeof(cfg->disk_path) - 1);
        cfg->disk_path[sizeof(cfg->disk_path) - 1] = '\0';
    } else if (strcmp(key, "LOG_LEVEL") == 0) {
        if (strlen(value) >= sizeof(cfg->log_level)) {
            if (errbuf != NULL) {
                snprintf(errbuf, errbuf_len, "value too long for '%s'", key);
            }
            return -1;
        }
        strncpy(cfg->log_level, value, sizeof(cfg->log_level) - 1);
        cfg->log_level[sizeof(cfg->log_level) - 1] = '\0';
    } else if (strcmp(key, "LOG_FILE") == 0) {
        if (strlen(value) >= sizeof(cfg->log_file)) {
            if (errbuf != NULL) {
                snprintf(errbuf, errbuf_len, "value too long for '%s'", key);
            }
            return -1;
        }
        strncpy(cfg->log_file, value, sizeof(cfg->log_file) - 1);
        cfg->log_file[sizeof(cfg->log_file) - 1] = '\0';
    } else if (strcmp(key, "NETWORK_MONITOR") == 0) {
        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
            strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
            cfg->network_monitor = 1;
        } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 ||
                   strcmp(value, "no") == 0 || strcmp(value, "off") == 0) {
            cfg->network_monitor = 0;
        } else {
            if (errbuf != NULL) {
                snprintf(errbuf, errbuf_len,
                         "invalid boolean for '%s': '%s' (use true/false)",
                         key, value);
            }
            return -1;
        }
    } else if (strcmp(key, "ENABLE_DESKTOP_NOTIFICATION") == 0) {
        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
            strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
            cfg->enable_desktop_notification = 1;
        } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 ||
                   strcmp(value, "no") == 0 || strcmp(value, "off") == 0) {
            cfg->enable_desktop_notification = 0;
        } else {
            if (errbuf != NULL) {
                snprintf(errbuf, errbuf_len,
                         "invalid boolean for '%s': '%s' (use true/false)",
                         key, value);
            }
            return -1;
        }
    } else {
        /* Unknown key: ignore for forward compatibility. */
        return -2;
    }

    return 0;
#undef PARSE_DOUBLE_FIELD
}

void config_set_defaults(MonitorConfig *cfg)
{
    if (cfg == NULL) {
        return;
    }

    cfg->cpu_warning = 70.0;
    cfg->cpu_critical = 90.0;
    cfg->memory_warning = 75.0;
    cfg->memory_critical = 90.0;
    cfg->disk_warning = 80.0;
    cfg->disk_critical = 95.0;
    cfg->load_warning = 4.0;
    cfg->load_critical = 8.0;
    cfg->monitor_interval = 5;
    cfg->top_processes = 5;
    strncpy(cfg->disk_path, "/", sizeof(cfg->disk_path) - 1);
    cfg->disk_path[sizeof(cfg->disk_path) - 1] = '\0';
    strncpy(cfg->log_level, "INFO", sizeof(cfg->log_level) - 1);
    cfg->log_level[sizeof(cfg->log_level) - 1] = '\0';
    strncpy(cfg->log_file, "logs/monitor.log", sizeof(cfg->log_file) - 1);
    cfg->log_file[sizeof(cfg->log_file) - 1] = '\0';
    cfg->network_monitor = 1;
    cfg->enable_desktop_notification = 0;
    cfg->alert_cooldown = 300;
}

int config_load(const char *path, MonitorConfig *cfg, char *errbuf, size_t errbuf_len)
{
    FILE *fp;
    char line[CONFIG_MAX_LINE];

    if (path == NULL || cfg == NULL) {
        if (errbuf != NULL && errbuf_len > 0) {
            snprintf(errbuf, errbuf_len, "config_load: null argument");
        }
        return -1;
    }

    config_set_defaults(cfg);

    fp = fopen(path, "r");
    if (fp == NULL) {
        if (errbuf != NULL && errbuf_len > 0) {
            snprintf(errbuf, errbuf_len, "cannot open config file '%s'", path);
        }
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *p = trim_whitespace(line);
        char *eq;
        char *key;
        char *value;

        /* Blank line or comment. */
        if (*p == '\0' || *p == '#') {
            continue;
        }

        /* Split on first '='. */
        eq = strchr(p, '=');
        if (eq == NULL) {
            if (errbuf != NULL && errbuf_len > 0) {
                snprintf(errbuf, errbuf_len,
                         "malformed line (no '='): '%s'", p);
            }
            fclose(fp);
            return -1;
        }

        *eq = '\0';
        key = trim_whitespace(p);
        value = trim_whitespace(eq + 1);

        if (*key == '\0') {
            if (errbuf != NULL && errbuf_len > 0) {
                snprintf(errbuf, errbuf_len, "missing key before '='");
            }
            fclose(fp);
            return -1;
        }

        /* -2 means unknown key (ignored); -1 is a real error. */
        if (config_apply_pair(cfg, key, value, errbuf, errbuf_len) == -1) {
            fclose(fp);
            return -1;
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        if (errbuf != NULL && errbuf_len > 0) {
            snprintf(errbuf, errbuf_len, "error reading config file '%s'", path);
        }
        return -1;
    }

    fclose(fp);
    return 0;
}

int config_validate(const MonitorConfig *cfg, char *errbuf, size_t errbuf_len)
{
    if (cfg == NULL) {
        return -1;
    }

#define CHECK_RANGE(field, lo, hi)                                     \
    do {                                                               \
        if ((cfg)->field < (lo) || (cfg)->field > (hi)) {              \
            if (errbuf != NULL) {                                      \
                snprintf(errbuf, errbuf_len,                          \
                         "'%s' out of range [%.0f, %.0f]",             \
                         #field, (double)(lo), (double)(hi));          \
            }                                                          \
            return -1;                                                 \
        }                                                              \
    } while (0)

    CHECK_RANGE(cpu_warning, 0.0, 100.0);
    CHECK_RANGE(cpu_critical, 0.0, 100.0);
    CHECK_RANGE(memory_warning, 0.0, 100.0);
    CHECK_RANGE(memory_critical, 0.0, 100.0);
    CHECK_RANGE(disk_warning, 0.0, 100.0);
    CHECK_RANGE(disk_critical, 0.0, 100.0);

#undef CHECK_RANGE

    /* Warning must be strictly less than critical for a sane alert ladder. */
    if (cfg->cpu_warning >= cfg->cpu_critical) {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "CPU_WARNING must be < CPU_CRITICAL");
        }
        return -1;
    }
    if (cfg->memory_warning >= cfg->memory_critical) {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "MEMORY_WARNING must be < MEMORY_CRITICAL");
        }
        return -1;
    }
    if (cfg->disk_warning >= cfg->disk_critical) {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "DISK_WARNING must be < DISK_CRITICAL");
        }
        return -1;
    }

    /* Load thresholds are absolute (>= 0). */
    if (cfg->load_warning < 0.0 || cfg->load_critical < 0.0) {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "load thresholds must be >= 0");
        }
        return -1;
    }
    if (cfg->load_warning >= cfg->load_critical) {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "LOAD_WARNING must be < LOAD_CRITICAL");
        }
        return -1;
    }

    /* Interval must be positive. */
    if (cfg->monitor_interval <= 0) {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "MONITOR_INTERVAL must be > 0");
        }
        return -1;
    }

    /* Top processes must be positive and bounded. */
    if (cfg->top_processes <= 0 || cfg->top_processes > 100) {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "TOP_PROCESSES must be in [1, 100]");
        }
        return -1;
    }

    /* Cooldown must be >= 0. */
    if (cfg->alert_cooldown < 0) {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "ALERT_COOLDOWN must be >= 0");
        }
        return -1;
    }

    if (cfg->disk_path[0] == '\0') {
        if (errbuf != NULL) {
            snprintf(errbuf, errbuf_len, "MONITOR_DISK cannot be empty");
        }
        return -1;
    }

    return 0;
}

void config_print(const MonitorConfig *cfg)
{
    if (cfg == NULL) {
        return;
    }
    printf("Configuration:\n");
    printf("  CPU_WARNING=%.1f  CPU_CRITICAL=%.1f\n", cfg->cpu_warning, cfg->cpu_critical);
    printf("  MEMORY_WARNING=%.1f  MEMORY_CRITICAL=%.1f\n", cfg->memory_warning, cfg->memory_critical);
    printf("  DISK_WARNING=%.1f  DISK_CRITICAL=%.1f\n", cfg->disk_warning, cfg->disk_critical);
    printf("  LOAD_WARNING=%.1f  LOAD_CRITICAL=%.1f\n", cfg->load_warning, cfg->load_critical);
    printf("  MONITOR_INTERVAL=%d s\n", cfg->monitor_interval);
    printf("  TOP_PROCESSES=%d\n", cfg->top_processes);
    printf("  MONITOR_DISK=%s\n", cfg->disk_path);
    printf("  LOG_LEVEL=%s  LOG_FILE=%s\n", cfg->log_level, cfg->log_file);
    printf("  NETWORK_MONITOR=%s\n", cfg->network_monitor ? "true" : "false");
    printf("  ENABLE_DESKTOP_NOTIFICATION=%s\n", cfg->enable_desktop_notification ? "true" : "false");
    printf("  ALERT_COOLDOWN=%d s\n", cfg->alert_cooldown);
}
