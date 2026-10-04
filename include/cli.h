/*
 * include/cli.h
 *
 * Command-line interface: option parsing and the Options structure.
 *
 * Supports getopt_long for: --help, --version, --config, --interval,
 * --once, --no-color.
 */

#ifndef LINUX_RESOURCE_MONITOR_CLI_H
#define LINUX_RESOURCE_MONITOR_CLI_H

#include <stdio.h>

#define CLI_CONFIG_PATH_MAX 512

typedef struct {
    char config_path[CLI_CONFIG_PATH_MAX];
    int interval;        /* override seconds; 0 = use config value */
    int once;            /* single-run mode */
    int no_color;        /* disable ANSI colors */
    int json;            /* emit JSON instead of the dashboard */
    int show_help;
    int show_version;
} CliOptions;

/*
 * cli_parse()
 * Parse argc/argv into opts. Returns 0 on success, -1 on an invalid option
 * (usage is printed to stderr).
 */
int cli_parse(int argc, char **argv, CliOptions *opts);

/*
 * cli_print_usage()
 * Write usage text to the given stream.
 */
void cli_print_usage(FILE *stream);

/*
 * cli_print_version()
 * Write version banner to the given stream.
 */
void cli_print_version(FILE *stream);

#endif /* LINUX_RESOURCE_MONITOR_CLI_H */
