/*
 * src/terminal.c
 *
 * Terminal color detection and ANSI escape helpers.
 */

#include "terminal.h"

#include <stdio.h>
#include <unistd.h>

static int g_color_enabled = 0;

int terminal_init(int no_color)
{
    /* Colors only if stdout is a TTY and the user did not pass --no-color. */
    g_color_enabled = (!no_color) && (isatty(STDOUT_FILENO) != 0);
    return 0;
}

int terminal_color_enabled(void)
{
    return g_color_enabled;
}

const char *terminal_color_code(TerminalColor c)
{
    if (!g_color_enabled) {
        return "";
    }
    switch (c) {
    case COLOR_RESET:  return "\033[0m";
    case COLOR_RED:    return "\033[31m";
    case COLOR_GREEN:  return "\033[32m";
    case COLOR_YELLOW: return "\033[33m";
    case COLOR_CYAN:   return "\033[36m";
    case COLOR_BOLD:   return "\033[1m";
    default:           return "\033[0m";
    }
}
