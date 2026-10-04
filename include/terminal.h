/*
 * include/terminal.h
 *
 * Terminal output helpers: ANSI colors (auto-detected, can be disabled) and
 * a few formatting utilities for the dashboard.
 */

#ifndef LINUX_RESOURCE_MONITOR_TERMINAL_H
#define LINUX_RESOURCE_MONITOR_TERMINAL_H

typedef enum {
    COLOR_RESET = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_BOLD
} TerminalColor;

/*
 * terminal_init()
 * Detect whether stdout is a TTY (enables colors only if so) and record the
 * user's --no-color preference. Returns 0.
 */
int terminal_init(int no_color);

/*
 * terminal_color_enabled()
 * Return 1 if colors are enabled, 0 otherwise.
 */
int terminal_color_enabled(void);

/*
 * terminal_color_code()
 * Return the ANSI escape sequence for a color (or empty string if disabled).
 */
const char *terminal_color_code(TerminalColor c);

#endif /* LINUX_RESOURCE_MONITOR_TERMINAL_H */
