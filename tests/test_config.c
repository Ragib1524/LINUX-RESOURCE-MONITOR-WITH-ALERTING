/*
 * tests/test_config.c
 *
 * Unit tests for the configuration parser and validator.
 *
 * Builds and runs standalone (no external test framework) to prove the config
 * module behaves correctly on happy paths and hostile inputs.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

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

/* Write a config string to a temp file and return its path (static buffer). */
static const char *write_temp_config(const char *content)
{
    static char path[256];
    FILE *fp;
    snprintf(path, sizeof(path), "/tmp/lrm_test_config_%d.conf", (int)getpid());
    fp = fopen(path, "w");
    if (fp == NULL) {
        return NULL;
    }
    fputs(content, fp);
    fclose(fp);
    return path;
}

int main(void)
{
    MonitorConfig cfg;
    char errbuf[256];
    const char *path;

    printf("test_config: defaults\n");
    config_set_defaults(&cfg);
    CHECK(cfg.cpu_warning == 70.0, "default cpu_warning == 70");
    CHECK(cfg.cpu_critical == 90.0, "default cpu_critical == 90");
    CHECK(cfg.monitor_interval == 5, "default interval == 5");
    CHECK(strcmp(cfg.disk_path, "/") == 0, "default disk path == /");

    printf("test_config: valid file parse\n");
    path = write_temp_config(
        "# comment\n"
        "\n"
        "CPU_WARNING=60\n"
        "CPU_CRITICAL=85\n"
        "MONITOR_INTERVAL=10\n"
        "TOP_PROCESSES=7\n"
        "NETWORK_MONITOR=false\n"
        "LOG_FILE=/tmp/test.log\n");
    errbuf[0] = '\0';
    CHECK(config_load(path, &cfg, errbuf, sizeof(errbuf)) == 0, "parse valid file succeeds");
    CHECK(cfg.cpu_warning == 60.0, "cpu_warning parsed to 60");
    CHECK(cfg.monitor_interval == 10, "interval parsed to 10");
    CHECK(cfg.top_processes == 7, "top_processes parsed to 7");
    CHECK(cfg.network_monitor == 0, "boolean false parsed correctly");
    CHECK(strcmp(cfg.log_file, "/tmp/test.log") == 0, "log_file parsed");

    printf("test_config: invalid numeric value\n");
    path = write_temp_config("CPU_WARNING=abc\n");
    errbuf[0] = '\0';
    CHECK(config_load(path, &cfg, errbuf, sizeof(errbuf)) == -1, "parse rejects non-numeric");
    CHECK(strstr(errbuf, "CPU_WARNING") != NULL, "error message names the key");

    printf("test_config: missing '='\n");
    path = write_temp_config("CPU_WARNING 70\n");
    errbuf[0] = '\0';
    CHECK(config_load(path, &cfg, errbuf, sizeof(errbuf)) == -1, "parse rejects missing '='");

    printf("test_config: validation - warning < critical\n");
    config_set_defaults(&cfg);
    cfg.cpu_warning = 95.0; /* > critical 90 -> invalid */
    errbuf[0] = '\0';
    CHECK(config_validate(&cfg, errbuf, sizeof(errbuf)) == -1, "validate rejects warning>=critical");
    CHECK(strstr(errbuf, "CPU_WARNING") != NULL, "validation message names the key");

    printf("test_config: validation - out of range\n");
    config_set_defaults(&cfg);
    cfg.memory_critical = 150.0; /* > 100 */
    errbuf[0] = '\0';
    CHECK(config_validate(&cfg, errbuf, sizeof(errbuf)) == -1, "validate rejects >100 percent");
    CHECK(strstr(errbuf, "memory_critical") != NULL, "validation message names the field");

    printf("test_config: validation - valid config passes\n");
    config_set_defaults(&cfg);
    CHECK(config_validate(&cfg, errbuf, sizeof(errbuf)) == 0, "defaults are valid");

    printf("test_config: unknown key ignored\n");
    path = write_temp_config("FUTURE_KEY=123\nCPU_WARNING=50\n");
    errbuf[0] = '\0';
    CHECK(config_load(path, &cfg, errbuf, sizeof(errbuf)) == 0, "unknown key does not break parse");
    CHECK(cfg.cpu_warning == 50.0, "known key still parsed after unknown key");

    printf("test_config: missing file\n");
    errbuf[0] = '\0';
    CHECK(config_load("/tmp/does_not_exist_xyz.conf", &cfg, errbuf, sizeof(errbuf)) == -1,
          "missing file returns error");
    CHECK(strstr(errbuf, "cannot open") != NULL, "missing file error is descriptive");

    printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
