# Linux Resource Monitor with Alerting

A production-quality Linux system resource monitor written **entirely in C**.

The application reads system metrics directly from the Linux kernel via the
`/proc` virtual filesystem and POSIX system calls — it never shells out to
`top`, `ps`, `free`, `df`, `uptime`, or any other external utility. It monitors
CPU, memory, disk, load average, uptime, processes, and network traffic,
compares each metric against configurable thresholds, and emits WARNING /
CRITICAL / RECOVERY alerts through a state machine that avoids alert flooding.

---

## Features

- **Pure C (C11)** — no Python, no Bash for monitoring logic.
- **Direct kernel access** — reads `/proc/stat`, `/proc/meminfo`,
  `/proc/uptime`, `/proc/loadavg`, `/proc/net/dev`, `/proc/<pid>/stat`;
  uses `statvfs()` and `getloadavg()`.
- **CPU utilization** from two `/proc/stat` samples (correct delta-based calc).
- **Memory utilization** using `MemAvailable` (modern Linux accounting).
- **Disk utilization** via `statvfs()` (no `df`).
- **Process monitor** scanning `/proc`, robust to vanishing PIDs and the
  notoriously tricky `/proc/<pid>/stat` comm field; computes **per-process
  CPU%** across two samples.
- **Network RX/TX rates** from `/proc/net/dev` (loopback excluded).
- **Alert state machine** (NORMAL/WARNING/CRITICAL/RECOVERY) with cooldown to
  prevent log flooding.
- **Reusable logger** with DEBUG..CRITICAL levels and timestamps.
- **Terminal dashboard** with optional ANSI colors.
- **CSV historical logging** (timestamp, cpu, memory, disk, load) — in C.
- **JSON output** (`--json`) for a single snapshot — in C.
- **Configurable** via a `KEY=VALUE` file with validation.
- **Signal-safe** SIGINT/SIGTERM shutdown.
- **systemd** service unit + install/uninstall scripts.
- **Thorough** unit tests and sanitizer coverage.

---

## Technologies

| Component | Technology |
|-----------|-----------|
| Language  | C (C11)    |
| Compiler  | GCC         |
| Build     | GNU Make    |
| Data sources | `/proc`, `/sys`, POSIX APIs, libc |
| Service   | systemd     |
| Testing   | standalone C tests, UBSan/ASan |

---

## Architecture

Layered and modular:

```
CLI (cli.c)
   └── Monitoring Engine (monitor.c)
          ├── CPU Monitor (cpu.c)         /proc/stat
          ├── Memory Monitor (memory.c)   /proc/meminfo
          ├── Disk Monitor (disk.c)       statvfs()
          ├── System Monitor (system.c)   getloadavg() + /proc/uptime
          ├── Process Monitor (process.c) /proc/<pid>/stat
          └── Network Monitor (network.c) /proc/net/dev
          ├── Alert Engine (alert.c)
          ├── Logger (logger.c)
          └── Terminal (terminal.c)
```

See `docs/architecture.md` for full details.

---

## Installation

Prerequisites: `gcc` and `make` on a Linux system (or WSL2).

```bash
# Build (release)
make

# Build debug (UBSan)
make debug

# Run unit tests
make test

# Install to /usr/local (requires sudo)
sudo ./scripts/install.sh

# Uninstall
sudo ./scripts/uninstall.sh
```

---

## Usage

```bash
./linux-resource-monitor                          # continuous (default)
./linux-resource-monitor --once                   # single collection then exit
./linux-resource-monitor --config config/monitor.conf
./linux-resource-monitor --interval 5             # override interval
./linux-resource-monitor --no-color               # disable colors
./linux-resource-monitor --json                   # single snapshot as JSON
./linux-resource-monitor --version
./linux-resource-monitor --help
```

Options: `-h/--help`, `-v/--version`, `-c/--config FILE`, `-i/--interval SEC`,
`-o/--once`, `-n/--no-color`, `-j/--json`.

---

## Configuration

See `config/monitor.conf`. Keys are `KEY=VALUE`; blank lines and `#` comments
are ignored. Thresholds are validated (0–100 percent, warning < critical).

| Key | Meaning |
|-----|---------|
| `CPU_WARNING` / `CPU_CRITICAL` | CPU% thresholds |
| `MEMORY_WARNING` / `MEMORY_CRITICAL` | memory% thresholds |
| `DISK_WARNING` / `DISK_CRITICAL` | disk% thresholds |
| `LOAD_WARNING` / `LOAD_CRITICAL` | load average thresholds |
| `MONITOR_INTERVAL` | seconds between cycles |
| `TOP_PROCESSES` | number of top processes shown |
| `MONITOR_DISK` | mount point for disk monitor |
| `LOG_LEVEL` / `LOG_FILE` | logging configuration |
| `NETWORK_MONITOR` | enable/disable network monitoring |
| `ALERT_COOLDOWN` | seconds between repeat alerts |

---

## Alert System

The alert engine is a per-resource state machine with three states (NORMAL,
WARNING, CRITICAL) plus a RECOVERY event. It fires an alert **only on
transition** and uses a cooldown to avoid re-alerting every cycle. See
`docs/alerts.md`.

---

## Example Output

```
============================================================
        LINUX RESOURCE MONITOR
============================================================
System Uptime : 00:20:56
CPU Usage     : 0.2%
Memory Usage  : 7.1%
Disk Usage    : 5.3%
Load Average
  1 min       : 1.04
  5 min       : 0.82
  15 min      : 0.40
Network
  eth0
    RX        : 42 B/s
    TX        : 132 B/s
Top Memory Processes
------------------------------------------------------------
PID        NAME                 MEMORY
178        snapd                30848 KB
...
Alerts
------------------------------------------------------------
CPU          NORMAL
MEMORY       NORMAL
DISK         NORMAL
============================================================
```

---

## Testing

```bash
make test       # 110+ assertions, 8 suites
make debug      # UBSan-instrumented build
```

See `docs/testing.md` for sanitizer details and the ASan `signgam` note.

---

## systemd

```bash
sudo systemctl enable --now linux-resource-monitor
sudo systemctl status linux-resource-monitor
journalctl -u linux-resource-monitor -f
```

---

## Troubleshooting

- **"cannot open config file"** — run from the project root, or pass
  `--config /absolute/path`.
- **CPU shows 0.0% in `--once`** — expected: two samples are taken nearly
  simultaneously; CPU% needs an interval (use continuous mode).
- **load/uptime unreadable** — `/proc/uptime` must be readable; check
  permissions (normally world-readable).

See `docs/troubleshooting.md`.

---

## Security

- No `system()`, `popen()`, `sprintf()`, `gets()`, `strcpy()`, `strcat()`.
- Bounded `snprintf`/`vsnprintf`/`fgets`/`memcpy` throughout.
- Overflow-checked integer parsing.
- No shell command construction from any input.
- Zero heap allocation (all fixed-size stack buffers) — no leak/overflow surface.

---

## Future Improvements

- CSV historical logging (already planned in C).
- Per-process CPU% (requires two `/proc/<pid>/stat` samples per PID).
- JSON output mode.
- Desktop notifications (optional, disabled by default).

---

## Project Structure

```
linux-resource-monitor/
├── src/           # C implementation
├── include/       # headers
├── tests/         # unit tests
├── config/        # example config
├── logs/          # log output (gitignored)
├── scripts/       # install/uninstall (bash only for setup)
├── systemd/       # service unit
├── docs/          # documentation
├── Makefile
└── README.md
```

## License

MIT — see `LICENSE`.
