/*
 * src/system.c
 *
 * Load average and uptime.
 *
 * Load average: use the POSIX getloadavg() API (a thin wrapper over the same
 * kernel data exposed via /proc/loadavg). Uptime: parse /proc/uptime.
 */

#include "system.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROC_UPTIME "/proc/uptime"

int system_read(SystemMetrics *s)
{
    double loads[3];
    FILE *fp;
    char line[128];

    if (s == NULL) {
        return -1;
    }

    memset(s, 0, sizeof(*s));

    /* getloadavg() fills up to 3 entries; returns the count. */
    if (getloadavg(loads, 3) < 3) {
        return -1;
    }
    s->load_1 = loads[0];
    s->load_5 = loads[1];
    s->load_15 = loads[2];

    /* Uptime: first field of /proc/uptime is seconds since boot. The line
     * also contains a second field (idle seconds), so parse only the first
     * whitespace-delimited token. */
    fp = fopen(PROC_UPTIME, "r");
    if (fp == NULL) {
        return -1;
    }
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    {
        char token[64];
        size_t i = 0;
        const char *q = line;
        while (*q != '\0' && *q != ' ' && *q != '\t' && i < sizeof(token) - 1) {
            token[i++] = *q++;
        }
        token[i] = '\0';
        if (parse_double(token, &s->uptime_seconds) != 0) {
            return -1;
        }
    }

    return 0;
}
