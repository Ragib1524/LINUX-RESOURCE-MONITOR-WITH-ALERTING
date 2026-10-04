/*
 * include/network.h
 *
 * Network monitor: reads /proc/net/dev for interface byte counters and
 * computes RX/TX rates across two samples.
 *
 * /proc/net/dev format (after two header lines):
 *   iface:  rx_bytes rx_packets ...  tx_bytes tx_packets ...
 *
 * The "lo" (loopback) interface is ignored unless configured otherwise.
 */

#ifndef LINUX_RESOURCE_MONITOR_NETWORK_H
#define LINUX_RESOURCE_MONITOR_NETWORK_H

#define NET_MAX_INTERFACES 64
#define NET_IFACE_NAME_MAX 32

typedef struct {
    char name[NET_IFACE_NAME_MAX];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    double rx_rate;   /* bytes/second, computed across two samples */
    double tx_rate;   /* bytes/second */
} NetworkInterface;

/* Snapshot of all interfaces at one point in time. */
typedef struct {
    NetworkInterface ifaces[NET_MAX_INTERFACES];
    int count;
} NetworkSnapshot;

/*
 * network_read()
 * Read all non-loopback interfaces from /proc/net/dev into *snap.
 * Returns 0 on success, -1 on failure.
 */
int network_read(NetworkSnapshot *snap);

/*
 * network_calc_rates()
 * Compute per-interface RX/TX rates (bytes/second) between 'prev' and 'curr'
 * using the elapsed time 'elapsed_seconds'. Matches interfaces by name and
 * stores the rates in curr's rx_rate/tx_rate fields.
 *
 * Returns 0 on success. Rates are clamped to >= 0 to handle counter resets.
 */
int network_calc_rates(const NetworkSnapshot *prev, NetworkSnapshot *curr,
                       double elapsed_seconds);

#endif /* LINUX_RESOURCE_MONITOR_NETWORK_H */
