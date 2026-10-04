/*
 * src/network.c
 *
 * Network monitoring via /proc/net/dev.
 *
 * The file has two header lines then one line per interface:
 *   eth0: 123456  789   ...  987654  321  ...
 *         ^rx_bytes                     ^tx_bytes
 *
 * We parse the first number after the colon (rx_bytes) and the first number
 * of the transmit group (tx_bytes, field 9). The loopback "lo" interface is
 * skipped.
 */

#include "network.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

#define PROC_NET_DEV "/proc/net/dev"

int network_read(NetworkSnapshot *snap)
{
    FILE *fp;
    char line[512];
    int count = 0;

    if (snap == NULL) {
        return -1;
    }
    snap->count = 0;

    fp = fopen(PROC_NET_DEV, "r");
    if (fp == NULL) {
        return -1;
    }

    /* Skip the two header lines. */
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL && count < NET_MAX_INTERFACES) {
        char name[NET_IFACE_NAME_MAX];
        unsigned long long rx_bytes, rx_packets, tx_bytes, tx_packets;
        char *colon;
        int n;

        /* Interface name is everything up to ':'. */
        colon = strchr(line, ':');
        if (colon == NULL) {
            continue;
        }
        {
            size_t name_len = (size_t)(colon - line);
            /* Trim trailing spaces in the name field (e.g. "  eth0"). */
            char *start = line;
            while (*start == ' ' || *start == '\t') {
                start++;
                name_len--;
            }
            if (name_len >= NET_IFACE_NAME_MAX) {
                name_len = NET_IFACE_NAME_MAX - 1;
            }
            memcpy(name, start, name_len);
            name[name_len] = '\0';
        }

        /* Skip loopback unless we ever support an "include lo" option. */
        if (strcmp(name, "lo") == 0) {
            continue;
        }

        /* Parse: rx_bytes rx_packets <6 more> tx_bytes tx_packets ... */
        n = sscanf(colon + 1,
                   "%llu %llu %*u %*u %*u %*u %*u %*u %llu %llu",
                   &rx_bytes, &rx_packets, &tx_bytes, &tx_packets);
        if (n < 4) {
            continue;
        }

        /* Copy the interface name (bounded, always NUL-terminated). */
        {
            size_t copy_len = strlen(name);
            if (copy_len >= NET_IFACE_NAME_MAX) {
                copy_len = NET_IFACE_NAME_MAX - 1;
            }
            memcpy(snap->ifaces[count].name, name, copy_len);
            snap->ifaces[count].name[copy_len] = '\0';
        }
        snap->ifaces[count].rx_bytes = rx_bytes;
        snap->ifaces[count].tx_bytes = tx_bytes;
        snap->ifaces[count].rx_packets = rx_packets;
        snap->ifaces[count].tx_packets = tx_packets;
        snap->ifaces[count].rx_rate = 0.0;
        snap->ifaces[count].tx_rate = 0.0;
        count++;
    }

    fclose(fp);
    snap->count = count;
    return 0;
}

int network_calc_rates(const NetworkSnapshot *prev, NetworkSnapshot *curr,
                       double elapsed_seconds)
{
    int i;
    int j;

    if (prev == NULL || curr == NULL || elapsed_seconds <= 0.0) {
        return -1;
    }

    for (i = 0; i < curr->count; i++) {
        /* Find the matching interface in prev. */
        for (j = 0; j < prev->count; j++) {
            if (strcmp(curr->ifaces[i].name, prev->ifaces[j].name) == 0) {
                long long rx_delta;
                long long tx_delta;

                rx_delta = (long long)(curr->ifaces[i].rx_bytes -
                                       prev->ifaces[j].rx_bytes);
                tx_delta = (long long)(curr->ifaces[i].tx_bytes -
                                       prev->ifaces[j].tx_bytes);

                if (rx_delta < 0) {
                    rx_delta = 0; /* counter reset */
                }
                if (tx_delta < 0) {
                    tx_delta = 0;
                }

                curr->ifaces[i].rx_rate = (double)rx_delta / elapsed_seconds;
                curr->ifaces[i].tx_rate = (double)tx_delta / elapsed_seconds;
                break;
            }
        }
        /* If no match in prev, rate stays 0 (interface appeared since). */
    }

    return 0;
}
