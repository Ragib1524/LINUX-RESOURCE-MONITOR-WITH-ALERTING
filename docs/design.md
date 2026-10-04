# Design

## Data structures

Central snapshot aggregates all metrics:

```c
typedef struct {
    CpuMetrics cpu;
    MemoryMetrics memory;
    DiskMetrics disk;
    SystemMetrics system;
    NetworkSnapshot network;
    ProcessInfo processes[PROCESS_MAX];
    int process_count;
} SystemMetricsSnapshot;
```

Each subsystem struct is defined in its own header (see `include/*.h`).

## Control flow

1. `cli_parse()` → `CliOptions`.
2. `config_load()` + `config_validate()` → `MonitorConfig`.
3. `terminal_init()` + `logger_init()`.
4. `install_signal_handlers()` (SIGINT/SIGTERM → set `g_running = 0`).
5. `monitor_init()` — captures CPU and network baselines.
6. Loop while `g_running`:
   - `monitor_collect()` — second samples + full snapshot.
   - `monitor_evaluate()` — threshold state machines + logging.
   - `print_dashboard()`.
   - chunked `sleep_millis()`.
7. `logger_close()` + clean exit.

## Error handling

Every system call is checked. Recoverable failures (missing `/proc` file,
vanished PID) return -1 and are logged/skipped. Only unrecoverable config or
init failures terminate the program.

## Memory management

**Zero heap allocation.** All buffers are fixed-size stack arrays. This is a
deliberate choice for a small, long-running daemon: it eliminates leaks,
double-free, and use-after-free by construction, and keeps memory usage flat.

## Performance

- No busy-waiting: `nanosleep` with EINTR handling.
- Process scan throttled by the monitoring interval.
- `qsort` for top-N (O(n log n)) instead of maintaining a full ranking each
  cycle.
- Monotonic clock for all interval/cooldown measurement.

## Security

- No external command execution (`system`/`popen`/`exec`).
- Bounded string APIs throughout.
- Overflow-checked numeric parsers.
- No shell command construction from config or CLI input.
