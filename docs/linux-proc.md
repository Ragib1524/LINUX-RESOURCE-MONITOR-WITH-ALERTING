# Linux /proc Filesystem

`/proc` is a pseudo-filesystem (procfs) exposing kernel and process
information as files. It is not stored on disk; reading a file triggers the
kernel to generate the content on the fly.

## Files used by this monitor

| File | Contents |
|------|----------|
| `/proc/stat` | CPU time counters (aggregate + per-CPU), interrupts, context switches |
| `/proc/meminfo` | Memory statistics (MemTotal, MemAvailable, Buffers, Cached, Swap…) |
| `/proc/loadavg` | 1/5/15-minute load averages + running/total tasks |
| `/proc/uptime` | seconds since boot + idle seconds |
| `/proc/net/dev` | per-interface RX/TX byte and packet counters |
| `/proc/<pid>/stat` | per-process state, CPU times, memory |
| `/proc/<pid>/status` | human-readable per-process status |

## Why /proc and not shell commands?

Shell utilities (`top`, `ps`, `free`, `df`, `uptime`) are themselves programs
that read these same `/proc` files. Reading `/proc` directly removes a process
spawn, gives exact control over parsing, and avoids the fragility and security
concerns of shelling out (no `system()`/`popen()`).

## The `/proc/<pid>/stat` parsing challenge

The second field (comm) is the process name wrapped in parentheses, but the
name may contain spaces AND parentheses. Naive whitespace/token splitting is
incorrect. The robust approach:

1. Find the **first** `(` and the **last** `)`.
2. The comm is everything between them.
3. Parse the 22 numeric fields after the final `)` with a single `sscanf`.

## The `/proc/<pid>` race

A process can exit at any moment between `readdir()` and opening its files.
Every access must tolerate `ENOENT`; the monitor simply skips that PID.

## /sys

`/sys` (sysfs) exposes device/kernel objects. This monitor primarily uses
`/proc` and POSIX APIs, but `statvfs()` internally interacts with the same
kernel filesystem layer. sysfs remains available for future extensions
(e.g. per-device I/O statistics).
