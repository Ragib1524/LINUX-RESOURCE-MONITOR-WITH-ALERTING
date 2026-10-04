# Architecture

## Overview

The Linux Resource Monitor is a layered, modular C application. Each layer has
a single responsibility and communicates with the next through well-defined
structures and function pointers (implicitly via static linking).

## Layers

```
+----------------------+
|      CLI Layer       |  cli.c   — getopt_long parsing → CliOptions
+----------+-----------+
           |
           v
+----------------------+
|  Monitoring Engine   |  monitor.c — init/collect/evaluate; owns baselines
+----------+-----------+
           |
   +-------+--------+----------------+----------------+
   |                |                |                |
   v                v                v                v
+--------+    +----------+    +----------+    +-----------+
|  CPU   |    |  Memory  |    |  Disk    |    |  System   |
+--------+    +----------+    +----------+    +-----------+
   |                |                |                |
   +-------+--------+----------------+----------------+
           |
   +-------+--------+----------------+
   |                |                |
   v                v                v
+--------+    +----------+    +-----------+
|Process |    | Network  |    |  (sysfs)  |
+--------+    +----------+    +-----------+
           |
           v
+----------------------+
|  Threshold/Alert     |  alert.c — state machine per resource
+----------+-----------+
           |
     +-----+-----+
     |           |
     v           v
+--------+  +--------+
| Console|  |  Log   |  terminal.c + logger.c
+--------+  +--------+
```

## Module responsibilities

| Module | Responsibility |
|--------|----------------|
| `main.c` | Entry point, signal handling, main loop, dashboard |
| `cli.c` | Command-line parsing |
| `config.c` | Config parse + validate |
| `monitor.c` | Engine: init baselines, collect snapshot, evaluate alerts |
| `cpu.c` | `/proc/stat` sampling and utilization |
| `memory.c` | `/proc/meminfo` + usage |
| `disk.c` | `statvfs()` |
| `system.c` | `getloadavg()` + `/proc/uptime` |
| `process.c` | `/proc` scan + `/proc/<pid>/stat` |
| `network.c` | `/proc/net/dev` + rates |
| `logger.c` | logging |
| `alert.c` | threshold state machine |
| `terminal.c` | colors/formatting |
| `utils.c` | safe parsers, timers |

## Design principles

1. **Loose coupling** — modules depend only on their own header + `utils.h`.
2. **No global mutable state** (except the logger's single FILE* and the
   terminal color flag, both confined to their modules).
3. **Explicit structs** for all metrics and config.
4. **Non-fatal subsystem failures** — a failed subsystem logs a warning and
   continues; it never crashes the monitor.
5. **No dynamic allocation** — fixed-size stack buffers eliminate an entire
   class of memory-safety bugs.
