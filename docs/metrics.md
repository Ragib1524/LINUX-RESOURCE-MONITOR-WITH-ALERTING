# Metrics

This document explains exactly how each metric is computed.

## CPU utilization

`/proc/stat` reports **cumulative** jiffies since boot. A single read cannot
yield a utilization percentage; we need two samples separated by an interval:

```
prev: cpu  user nice system idle iowait irq softirq steal
curr: cpu  user nice system idle iowait irq softirq steal

total_delta = curr_total - prev_total
idle_delta  = curr_idle  - prev_idle        (idle = idle + iowait)
usage%      = (total_delta - idle_delta) / total_delta * 100
```

- `idle + iowait` is "CPU had nothing runnable" — treating only `idle` as free
  is a common mistake.
- Zero total delta (or backwards counters) → invalid sample, report error.

## Memory

`/proc/meminfo` provides `MemTotal` and `MemAvailable` (KiB). Modern Linux
memory utilization:

```
used  = MemTotal - MemAvailable
usage% = used / MemTotal * 100
```

`MemAvailable` (not `MemFree`) is the correct "free" metric: the kernel
reclaims page cache under pressure, so `MemFree` alone understates available
memory.

## Disk

`statvfs(path)` fills a struct; capacity uses the fragment size `f_frsize`:

```
total      = f_blocks  * f_frsize
available  = f_bavail  * f_frsize     (space available to unprivileged)
used       = total - available
usage%     = used / total * 100
```

## Load average

`getloadavg()` returns 1/5/15-minute exponentially-damped run-queue averages.
Load average is a **count of runnable (+ uninterruptible) tasks**, not a
percentage — it should be compared to the number of logical CPUs, not to 100.

## Uptime

`/proc/uptime` first field is seconds since boot.

## Process memory

`/proc/<pid>/stat` field 24 is `rss` (resident set size) in pages:

```
memory_kb = rss * page_size / 1024
```

## Per-process CPU

Each process's `utime` (field 14) + `stime` (field 15) are CPU jiffies. The
per-process CPU% is the process's delta between two scans, divided by the
total busy jiffies across all CPUs over the same interval:

```
cpu% = ((utime+stime) delta of this process) / busy_delta_all_cpus * 100
```

This requires two process scans and two `/proc/stat` samples, so it is
available only in continuous mode (not `--once`).

## Network rates

`/proc/net/dev` reports cumulative RX/TX bytes per interface. Rates are deltas
over an interval, clamped to zero on counter reset:

```
rx_rate = (curr_rx - prev_rx) / elapsed_seconds
```
