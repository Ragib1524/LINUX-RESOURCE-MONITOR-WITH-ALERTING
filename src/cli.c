/*
 * src/cli.c
 *
 * Command-line parsing with getopt_long.
 */

#include "cli.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_NAME    "linux-resource-monitor"
#define APP_VERSION "1.0.0"

static void set_config_path(CliOptions *opts, const char *path)
{
    strncpy(opts->config_path, path, CLI_CONFIG_PATH_MAX - 1);
    opts->config_path[CLI_CONFIG_PATH_MAX - 1] = '\0';
}

int cli_parse(int argc, char **argv, CliOptions *opts)
{
    int c;

    static const struct option long_opts[] = {
        { "help",     no_argument,       NULL, 'h' },
        { "version",  no_argument,       NULL, 'v' },
        { "config",   required_argument, NULL, 'c' },
        { "interval", required_argument, NULL, 'i' },
        { "once",     no_argument,       NULL, 'o' },
        { "no-color", no_argument,       NULL, 'n' },
        { "json",     no_argument,       NULL, 'j' },
        { NULL, 0, NULL, 0 }
    };

    if (opts == NULL) {
        return -1;
    }

    memset(opts, 0, sizeof(*opts));
    set_config_path(opts, "config/monitor.conf");

    opterr = 0; /* we handle error messages ourselves */

    while ((c = getopt_long(argc, argv, "hvc:i:onj", long_opts, NULL)) != -1) {
        switch (c) {
        case 'h':
            opts->show_help = 1;
            break;
        case 'v':
            opts->show_version = 1;
            break;
        case 'c':
            set_config_path(opts, optarg);
            break;
        case 'i': {
            long long v;
            char *end;
            /* parse interval; must be a positive integer */
            v = strtoll(optarg, &end, 10);
            if (*optarg == '\0' || *end != '\0' || v <= 0) {
                fprintf(stderr, "error: invalid interval '%s'\n", optarg);
                return -1;
            }
            opts->interval = (int)v;
            break;
        }
        case 'o':
            opts->once = 1;
            break;
        case 'n':
            opts->no_color = 1;
            break;
        case 'j':
            opts->json = 1;
            break;
        case '?':
        default:
            fprintf(stderr, "error: invalid option '%s'\n", argv[optind - 1]);
            return -1;
        }
    }

    /* Unexpected positional arguments. */
    if (optind < argc) {
        fprintf(stderr, "error: unexpected argument '%s'\n", argv[optind]);
        return -1;
    }

    return 0;
}

void cli_print_usage(FILE *stream)
{
    fprintf(stream,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Linux resource monitor (reads /proc and POSIX APIs directly).\n"
        "\n"
        "Options:\n"
        "  -h, --help           Show this help\n"
        "  -v, --version        Show version\n"
        "  -c, --config FILE    Config file (default: config/monitor.conf)\n"
        "  -i, --interval SEC   Monitoring interval in seconds\n"
        "  -o, --once           Run a single collection then exit\n"
        "  -n, --no-color       Disable ANSI colors\n"
        "  -j, --json           Emit metrics as JSON (single snapshot)\n",
        APP_NAME);
}

void cli_print_version(FILE *stream)
{
    fprintf(stream, "%s v%s (Linux systems programming, C11)\n",
            APP_NAME, APP_VERSION);
}
