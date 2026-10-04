# Linux Resource Monitor with Alerting

![C11](https://img.shields.io/badge/C-C11-blue)
![Linux](https://img.shields.io/badge/Linux-supported-brightgreen)
![Build](https://img.shields.io/badge/build-GNU%20Make-orange)
![License](https://img.shields.io/badge/license-MIT-green)

A modular, production-oriented **Linux system resource monitor written entirely in C11**.

The application collects CPU, memory, disk, load average, uptime, process, and network metrics directly from Linux kernel interfaces such as `/proc`, along with POSIX system calls. It does **not** invoke external monitoring utilities such as `top`, `ps`, `free`, `df`, or `uptime`.

The monitor evaluates configurable thresholds and generates **WARNING, CRITICAL, and RECOVERY** events through a state-based alert engine designed to prevent repeated alert flooding.

> **Design principle:** Monitoring data is collected directly from Linux kernel interfaces and POSIX APIs rather than by shelling out to external utilities.

---

## Why This Project?

Most traditional resource-monitoring tools rely on commands such as `top`, `ps`, `free`, and `df`.

This project implements the monitoring stack directly in **C**, providing a lightweight and dependency-minimal approach while demonstrating practical Linux systems programming concepts:

* Linux `/proc` filesystem parsing
* CPU and process accounting
* Memory and disk monitoring
* Network traffic measurement
* POSIX system calls
* Signal-safe shutdown
* Threshold-based alerting
* State-machine design
* systemd service integration
* Defensive C programming
* Sanitizer-based testing

---

## Features

* **Pure C11** — no Python or Bash for monitoring logic.
* **Direct kernel access** — reads:

  * `/proc/stat`
  * `/proc/meminfo`
  * `/proc/uptime`
  * `/proc/loadavg`
  * `/proc/net/dev`
  * `/proc/<pid>/stat`
* **CPU utilization** calculated from two `/proc/stat` samples using delta-based accounting.
* **Memory utilization** calculated using Linux `MemAvailable`.
* **Disk utilization** calculated with `statvfs()` without using `df`.
* **System monitoring** for load average and uptime.
* **Process monitoring** by scanning `/proc`, handling vanishing PIDs safely, and correctly parsing the `/proc/<pid>/stat` `comm` field.
* **Per-process CPU utilization** calculated across two process samples.
* **Network RX/TX rates** calculated from `/proc/net/dev`, with loopback excluded.
* **Alert state machine** with NORMAL, WARNING, CRITICAL, and RECOVERY events.
* **Alert cooldown** to prevent repeated log flooding.
* **Reusable logger** with DEBUG through CRITICAL log levels and timestamps.
* **Terminal dashboard** with optional ANSI colors.
* **CSV historical logging** for CPU, memory, disk, and load metrics.
* **JSON output** for a single snapshot.
* **Configurable thresholds** through a `KEY=VALUE` configuration file.
* **Signal-safe shutdown** using SIGINT and SIGTERM handling.
* **systemd integration** with service and installation scripts.
* **Unit tests** covering the monitoring components.
* **ASan/UBSan support** for memory and undefined-behavior checking.
* **No shell execution** from monitoring logic.
* **Fixed-size buffers** and bounded input handling throughout the monitoring path.

---

## Engineering Highlights

This project focuses on low-level Linux monitoring rather than wrapping existing command-line utilities.

### CPU Accounting

CPU utilization is calculated from consecutive `/proc/stat` samples:

```text
CPU% = (busy_delta / total_delta) × 100
```

This provides interval-based utilization rather than relying on an external utility.

### Memory Monitoring

Memory utilization uses Linux's `MemAvailable` value instead of simply subtracting `MemFree`, producing a more practical estimate of memory available to applications.

### Process Monitoring

The process monitor scans `/proc` directly and reads `/proc/<pid>/stat`.

Special care is taken when parsing the `comm` field because process names can contain characters that make naive whitespace-based parsing unreliable.

The monitor also tolerates processes disappearing between directory enumeration and file access.

### Network Monitoring

Network traffic is read directly from `/proc/net/dev`.

RX and TX byte deltas are converted into rates, while the loopback interface is excluded from the displayed network statistics.

### Alert State Machine

Each monitored resource maintains its own state:

```text
                 ┌───────────┐
                 │  NORMAL   │
                 └─────┬─────┘
                       │ warning threshold exceeded
                       ▼
                 ┌───────────┐
                 │  WARNING  │
                 └─────┬─────┘
                       │ critical threshold exceeded
                       ▼
                 ┌───────────┐
                 │ CRITICAL  │
                 └─────┬─────┘
                       │ resource recovers
                       ▼
                 ┌───────────┐
                 │ RECOVERY  │
                 └─────┬─────┘
                       │
                       ▼
                 ┌───────────┐
                 │  NORMAL   │
                 └───────────┘
```

Alerts are generated primarily on state transitions, while a configurable cooldown limits repeated alerts when a resource remains unhealthy.

See `docs/alerts.md` for the complete alerting design.

---

## Technologies

| Component         | Technology                                    |
| ----------------- | --------------------------------------------- |
| Language          | C11                                           |
| Compiler          | GCC                                           |
| Build System      | GNU Make                                      |
| Kernel Interfaces | `/proc`                                       |
| System APIs       | POSIX / libc                                  |
| Disk Statistics   | `statvfs()`                                   |
| System Service    | systemd                                       |
| Testing           | Standalone C tests                            |
| Sanitizers        | AddressSanitizer / UndefinedBehaviorSanitizer |
| Output            | Terminal / CSV / JSON                         |

---

## Architecture

The application follows a modular layered architecture:

```text
CLI (cli.c)
   │
   ▼
Monitoring Engine (monitor.c)
   ├── CPU Monitor       (cpu.c)       /proc/stat
   ├── Memory Monitor    (memory.c)    /proc/meminfo
   ├── Disk Monitor      (disk.c)      statvfs()
   ├── System Monitor    (system.c)    /proc/uptime + getloadavg()
   ├── Process Monitor   (process.c)   /proc/<pid>/stat
   └── Network Monitor   (network.c)  /proc/net/dev
   │
   ├── Alert Engine      (alert.c)
   ├── Logger            (logger.c)
   └── Terminal          (terminal.c)
```

See `docs/architecture.md` for the detailed architecture and component responsibilities.

---

## Quick Start

### Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/linux-resource-monitor.git
cd linux-resource-monitor
```

### Build

```bash
make
```

### Run a Single Snapshot

```bash
./linux-resource-monitor --once
```

### Run Continuously

```bash
./linux-resource-monitor
```

### Run Unit Tests

```bash
make test
```

### Build with Sanitizers

```bash
make debug
```

---

## Installation

### Prerequisites

* Linux system or WSL2
* GCC
* GNU Make

### Release Build

```bash
make
```

### Debug / Sanitizer Build

```bash
make debug
```

### Unit Tests

```bash
make test
```

### Install

The project includes an installation script for `/usr/local`.

```bash
sudo ./scripts/install.sh
```

### Uninstall

```bash
sudo ./scripts/uninstall.sh
```

---

## Usage

```bash
./linux-resource-monitor
```

Run continuously using the configured monitoring interval.

### Single Snapshot

```bash
./linux-resource-monitor --once
```

### Custom Configuration

```bash
./linux-resource-monitor --config config/monitor.conf
```

### Override Monitoring Interval

```bash
./linux-resource-monitor --interval 5
```

### Disable Terminal Colors

```bash
./linux-resource-monitor --no-color
```

### JSON Output

```bash
./linux-resource-monitor --json
```

### Display Version

```bash
./linux-resource-monitor --version
```

### Display Help

```bash
./linux-resource-monitor --help
```

### Command-Line Options

| Option                 | Description                        |
| ---------------------- | ---------------------------------- |
| `-h`, `--help`         | Display help                       |
| `-v`, `--version`      | Display version                    |
| `-c`, `--config FILE`  | Load configuration file            |
| `-i`, `--interval SEC` | Override monitoring interval       |
| `-o`, `--once`         | Collect a single snapshot and exit |
| `-n`, `--no-color`     | Disable ANSI colors                |
| `-j`, `--json`         | Output a single snapshot as JSON   |

---

## Configuration

The default configuration file is:

```text
config/monitor.conf
```

Configuration uses a simple `KEY=VALUE` format.

Blank lines and lines beginning with `#` are ignored.

Example:

```text
CPU_WARNING=70
CPU_CRITICAL=90

MEMORY_WARNING=75
MEMORY_CRITICAL=90

DISK_WARNING=80
DISK_CRITICAL=95

LOAD_WARNING=4.0
LOAD_CRITICAL=8.0

MONITOR_INTERVAL=5
TOP_PROCESSES=5

MONITOR_DISK=/
LOG_LEVEL=INFO
LOG_FILE=logs/monitor.log

NETWORK_MONITOR=true
ALERT_COOLDOWN=60
```

### Configuration Keys

| Key                | Meaning                              |
| ------------------ | ------------------------------------ |
| `CPU_WARNING`      | CPU warning threshold                |
| `CPU_CRITICAL`     | CPU critical threshold               |
| `MEMORY_WARNING`   | Memory warning threshold             |
| `MEMORY_CRITICAL`  | Memory critical threshold            |
| `DISK_WARNING`     | Disk warning threshold               |
| `DISK_CRITICAL`    | Disk critical threshold              |
| `LOAD_WARNING`     | Load average warning threshold       |
| `LOAD_CRITICAL`    | Load average critical threshold      |
| `MONITOR_INTERVAL` | Seconds between monitoring cycles    |
| `TOP_PROCESSES`    | Number of top processes displayed    |
| `MONITOR_DISK`     | Filesystem mount point to monitor    |
| `LOG_LEVEL`        | Minimum logger severity              |
| `LOG_FILE`         | Log output file                      |
| `NETWORK_MONITOR`  | Enable or disable network monitoring |
| `ALERT_COOLDOWN`   | Seconds between repeated alerts      |

Threshold configuration is validated so that percentage values remain within valid limits and warning thresholds remain below critical thresholds.

---

## Alert System

The alert engine maintains a state for each monitored resource.

Supported states:

```text
NORMAL
WARNING
CRITICAL
```

Recovery is emitted as an event when a previously unhealthy resource returns to normal conditions.

Example transition:

```text
CPU: NORMAL
       ↓
CPU: WARNING
       ↓
CPU: CRITICAL
       ↓
CPU: RECOVERY
       ↓
CPU: NORMAL
```

This design prevents logs from being flooded with identical alerts every monitoring cycle.

The cooldown period can be configured with:

```text
ALERT_COOLDOWN=60
```

See:

```text
docs/alerts.md
```

for the detailed alert-state implementation.

---

## Example Dashboard Output

```text
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

## JSON Output

The `--json` option produces a single machine-readable snapshot.

Example:

```json
{
  "cpu": 12.4,
  "memory": 38.7,
  "disk": 54.2,
  "load_1m": 0.82
}
```

This makes it possible to consume monitoring data from another program without requiring external monitoring commands.

---

## CSV Historical Logging

The monitor can record historical system metrics in CSV format.

Example:

```text
timestamp,cpu,memory,disk,load
2026-10-04T22:10:01,12.4,38.7,54.2,0.82
2026-10-04T22:10:06,14.1,39.0,54.2,0.91
2026-10-04T22:10:11,11.7,38.8,54.2,0.76
```

This data can later be imported into tools such as Excel, Power BI, Python, or other visualization systems for historical analysis.

---

## Testing

The project includes standalone C tests covering the monitoring components.

Run the full test suite:

```bash
make test
```

Build with UndefinedBehaviorSanitizer:

```bash
make debug
```

The project is designed to support sanitizer-based testing for detecting:

* Buffer misuse
* Integer problems
* Undefined behavior
* Memory-related issues

See:

```text
docs/testing.md
```

for testing details and sanitizer notes.

---

## systemd Integration

The project includes a systemd service unit for running the monitor as a background service.

Enable and start the service:

```bash
sudo systemctl enable --now linux-resource-monitor
```

Check service status:

```bash
sudo systemctl status linux-resource-monitor
```

Follow service logs:

```bash
journalctl -u linux-resource-monitor -f
```

This allows the monitor to start automatically with the system and run continuously as a managed service.

---

## Security and Defensive C Practices

The implementation avoids common unsafe C and shell-execution patterns.

### Security-related design choices

* No `system()`
* No `popen()`
* No `sprintf()`
* No `gets()`
* No `strcpy()`
* No `strcat()`
* No shell command construction
* No external monitoring command execution
* Bounded `snprintf()` / `vsnprintf()` usage
* Bounded `fgets()` usage
* Controlled `memcpy()` usage
* Overflow-checked integer parsing
* Defensive handling of disappearing `/proc/<pid>` entries
* Fixed-size buffers in the monitoring path
* Signal-safe shutdown handling

The monitoring path avoids dynamic heap allocation where possible and relies on fixed-size stack buffers and bounded operations.

---

## Signal Handling

The application handles:

```text
SIGINT
SIGTERM
```

Shutdown is coordinated through a signal-safe mechanism so that the main monitoring loop can terminate cleanly.

This allows normal interruption with:

```bash
Ctrl + C
```

and graceful termination when managed by systemd.

---

## Troubleshooting

### `cannot open config file`

Run the program from the project root:

```bash
./linux-resource-monitor
```

or provide an absolute path:

```bash
./linux-resource-monitor --config /absolute/path/monitor.conf
```

### CPU Shows `0.0%` in `--once`

This is expected.

CPU utilization is calculated from two samples. In `--once` mode the samples occur almost immediately, so the measured delta may be extremely small.

For meaningful CPU utilization, use continuous mode:

```bash
./linux-resource-monitor
```

### Load or Uptime Cannot Be Read

The monitor expects `/proc/uptime` and related `/proc` interfaces to be accessible.

On a normal Linux system they should generally be readable.

Check:

```bash
ls -l /proc/uptime
ls -l /proc/loadavg
```

---

## Project Structure

```text
linux-resource-monitor/
├── src/                  # C implementation
│   ├── alert.c
│   ├── cli.c
│   ├── cpu.c
│   ├── disk.c
│   ├── logger.c
│   ├── memory.c
│   ├── monitor.c
│   ├── network.c
│   ├── process.c
│   ├── system.c
│   └── terminal.c
│
├── include/              # Header files
│
├── tests/                # Unit tests
│
├── config/               # Example configuration
│   └── monitor.conf
│
├── logs/                 # Runtime logs (gitignored)
│
├── scripts/              # Installation/uninstallation scripts
│
├── systemd/              # systemd service unit
│
├── docs/                 # Project documentation
│   ├── architecture.md
│   ├── alerts.md
│   ├── testing.md
│   └── troubleshooting.md
│
├── Makefile
├── README.md
└── LICENSE
```

---

## Project Status

**Core implementation:** Completed

The core monitoring engine, alerting system, logging, JSON output, CSV logging, testing infrastructure, and systemd integration are implemented.

The project can be built and executed directly on Linux or WSL2 using GCC and GNU Make.

---

## Future Improvements

* Configurable alert destinations such as email, Slack, or webhooks
* Historical trend visualization using the generated CSV data
* Optional desktop notifications
* Additional filesystem and network metrics
* Configurable process filtering and per-process alert thresholds
* Optional API or daemon mode for external dashboard integration

---

## Documentation

Additional technical documentation is available in the `docs/` directory:

```text
docs/
├── architecture.md
├── alerts.md
├── testing.md
└── troubleshooting.md
```

These documents provide deeper explanations of the system design, alert engine, testing strategy, and troubleshooting procedures.

---

## Dashboard Preview

Add a screenshot of the terminal dashboard here:

```markdown
![Linux Resource Monitor Dashboard](docs/images/dashboard.png)
```

A short GIF demonstrating monitoring and alert transitions can also be added:

```markdown
![Alert State Transition](docs/images/alert-demo.gif)
```

---

## License

This project is licensed under the **MIT License**.

See [LICENSE](LICENSE) for details.
