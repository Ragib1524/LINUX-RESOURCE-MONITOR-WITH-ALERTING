/*
 * include/config.h
 *
 * Configuration system: structure definition, defaults, parsing and validation.
 *
 * The monitor is driven by a single MonitorConfig structure. It is populated
 * from a KEY=VALUE file (see config/monitor.conf) and then validated. Any
 * invalid value produces a clear error naming the key and the reason, and the
 * program exits safely rather than running with a corrupt configuration.
 */

#ifndef LINUX_RESOURCE_MONITOR_CONFIG_H
#define LINUX_RESOURCE_MONITOR_CONFIG_H

#include <stddef.h>

/* Maximum length of a single line in the configuration file. */
#define CONFIG_MAX_LINE 512

/* Thresholds are percentages (0..100) unless otherwise noted. */
typedef struct {
    /* CPU thresholds (percent). */
    double cpu_warning;
    double cpu_critical;

    /* Memory thresholds (percent). */
    double memory_warning;
    double memory_critical;

    /* Disk thresholds (percent). */
    double disk_warning;
    double disk_critical;

    /* Load average thresholds (absolute load values). */
    double load_warning;
    double load_critical;

    /* Monitoring interval in seconds. */
    int monitor_interval;

    /* Number of top processes shown per category. */
    int top_processes;

    /* Mount point to monitor for disk usage. */
    char disk_path[256];

    /* Log configuration. */
    char log_level[16];
    char log_file[512];

    /* Feature toggles. */
    int network_monitor;
    int enable_desktop_notification;

    /* Alert cooldown in seconds (avoid alert flooding). */
    int alert_cooldown;
} MonitorConfig;

/*
 * config_set_defaults()
 * Load built-in safe defaults into cfg.
 */
void config_set_defaults(MonitorConfig *cfg);

/*
 * config_load()
 * Parse the configuration file at 'path' into cfg.
 * Returns 0 on success. On failure returns -1 and writes a human-readable
 * message to errbuf (if non-NULL).
 */
int config_load(const char *path, MonitorConfig *cfg, char *errbuf, size_t errbuf_len);

/*
 * config_validate()
 * Check that cfg holds sensible, consistent values.
 * Returns 0 if valid, -1 otherwise (with a message in errbuf if provided).
 */
int config_validate(const MonitorConfig *cfg, char *errbuf, size_t errbuf_len);

/*
 * config_print()
 * Print the effective configuration to stdout (used by --help/verbose mode).
 */
void config_print(const MonitorConfig *cfg);

#endif /* LINUX_RESOURCE_MONITOR_CONFIG_H */
