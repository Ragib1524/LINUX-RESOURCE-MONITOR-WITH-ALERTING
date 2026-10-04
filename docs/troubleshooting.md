# Troubleshooting

## "cannot open config file 'config/monitor.conf'"

The binary must be run from the project root (where `config/` and `logs/`
exist), or you must pass an absolute `--config` path. Under systemd, the
`WorkingDirectory` directive handles this.

## "CPU Usage: 0.0%" in `--once` mode

Expected: single-run mode takes the baseline and second sample nearly
simultaneously, so the delta (and thus CPU%) is ~zero. Use continuous mode to
see meaningful CPU percentages.

## "memory monitor: /proc/meminfo unreadable"

`/proc/meminfo` is normally world-readable. If unreadable, check that `/proc`
is mounted (`mount | grep proc`) and that you are on a real Linux kernel
(not a container/chroot without procfs, and not Git Bash/MSYS2).

## "system monitor: load/uptime unreadable"

`getloadavg()` and `/proc/uptime` require a genuine Linux `/proc`. Under WSL1
or non-Linux shells these are unavailable — use WSL2 or native Linux.

## "disk monitor: statvfs(...) failed"

The configured `MONITOR_DISK` path is invalid/unmounted. Check the value in
`config/monitor.conf`.

## Processes show "0 KB"

If RSS is reported as zero, the `/proc/<pid>/stat` parse may have failed for
that PID (e.g. a kernel thread with no userspace RSS). This is benign.

## Build warnings

The build uses `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2`.
Any warning indicates a real issue; the codebase compiles warning-free.

## systemd not running (WSL)

WSL2 does not run systemd by default (although `systemd=true` can be enabled
in `/etc/wsl.conf`). The core monitor works without systemd — just run the
binary directly. The systemd unit is provided for native Linux deployments.

## Toolchain (non-root prefix)

If `gcc`/`make` are not installed system-wide, they can be bootstrapped to
`~/.local` without root (as was done for this environment). See
`docs/testing.md` for the ASan caveat in that scenario.
