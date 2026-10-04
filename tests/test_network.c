/*
 * tests/test_network.c
 *
 * Unit tests for the network monitor: real /proc/net/dev scan and rate calc.
 */

#include <stdio.h>
#include <string.h>

#include "network.h"

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

int main(void)
{
    NetworkSnapshot a;
    NetworkSnapshot b;

    printf("test_network: real read\n");
    CHECK(network_read(&a) == 0, "network_read succeeds");
    printf("  (found %d interfaces)\n", a.count);

    printf("test_network: loopback excluded\n");
    {
        int i;
        int has_lo = 0;
        for (i = 0; i < a.count; i++) {
            if (a.ifaces[i].name[0] == 'l' && a.ifaces[i].name[1] == 'o') {
                has_lo = 1;
            }
        }
        CHECK(!has_lo, "loopback 'lo' is not listed");
    }

    printf("test_network: rate calculation\n");
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    strncpy(a.ifaces[0].name, "eth0", NET_IFACE_NAME_MAX - 1);
    a.ifaces[0].rx_bytes = 1000;
    a.ifaces[0].tx_bytes = 2000;
    a.count = 1;
    strncpy(b.ifaces[0].name, "eth0", NET_IFACE_NAME_MAX - 1);
    b.ifaces[0].rx_bytes = 1500;
    b.ifaces[0].tx_bytes = 2400;
    b.count = 1;

    CHECK(network_calc_rates(&a, &b, 2.0) == 0, "calc succeeds");
    CHECK(b.ifaces[0].rx_rate > 249.0 && b.ifaces[0].rx_rate < 251.0,
          "rx_rate == 250 B/s");
    CHECK(b.ifaces[0].tx_rate > 199.0 && b.ifaces[0].tx_rate < 201.0,
          "tx_rate == 200 B/s");

    printf("test_network: counter reset clamped\n");
    b.ifaces[0].rx_bytes = 500; /* went backwards */
    network_calc_rates(&a, &b, 1.0);
    CHECK(b.ifaces[0].rx_rate == 0.0, "reset => rate 0");

    printf("test_network: NULL safe\n");
    CHECK(network_read(NULL) == -1, "NULL snapshot rejected");
    CHECK(network_calc_rates(NULL, &b, 1.0) == -1, "NULL prev rejected");
    CHECK(network_calc_rates(&a, NULL, 1.0) == -1, "NULL curr rejected");
    CHECK(network_calc_rates(&a, &b, 0.0) == -1, "zero elapsed rejected");

    printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
