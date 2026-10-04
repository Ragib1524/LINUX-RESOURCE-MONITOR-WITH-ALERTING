# Interview Questions & Answers

A guide for discussing this project in a technical interview / PPO.

1. **How does Linux calculate CPU usage?**
   `/proc/stat` reports cumulative jiffies since boot. Utilization is the
   *delta* of busy time over total time between two samples: `(total_delta -
   idle_delta) / total_delta * 100`.

2. **Why are two `/proc/stat` samples needed?**
   A single read gives absolute counters (since boot), not a rate. Usage is a
   rate, so you must measure the change between two points in time.

3. **What is `/proc`?**
   A virtual filesystem (procfs) where the kernel exposes runtime state as
   files generated on read. It is not disk-backed.

4. **What is `/sys`?**
   sysfs, another virtual filesystem exposing kernel objects/devices in a
   structured hierarchy. This project uses `/proc` and POSIX APIs primarily.

5. **What is a system call?**
   A controlled entry point from userspace into the kernel (e.g. `open`,
   `read`, `statvfs`, `clock_gettime`). The transition switches CPU to kernel
   mode.

6. **What is a process?**
   A running instance of a program: an address space, kernel bookkeeping
   (task_struct), and one or more threads. Identified by a PID.

7. **How did you enumerate processes?**
   `opendir("/proc")`, `readdir`, and selecting numeric directory names
   (PIDs), then reading `/proc/<pid>/stat` and `/proc/<pid>/status`.

8. **Why can `/proc/<pid>` disappear?**
   Processes exit asynchronously; between listing a PID and opening its files
   the process may terminate (or the PID be recycled). Every access must
   handle `ENOENT` gracefully.

9. **How did you calculate memory utilization?**
   `used = MemTotal - MemAvailable`, `usage = used/MemTotal*100`, from
   `/proc/meminfo`.

10. **Why use `MemAvailable` instead of `MemFree`?**
    Linux reclaims page cache under pressure; `MemAvailable` is the kernel's
    estimate of memory a new allocation can obtain without swapping. `MemFree`
    understates real headroom.

11. **How does `statvfs` work?**
    A POSIX syscall filling a struct with filesystem stats. Capacity =
    `f_blocks * f_frsize`; available = `f_bavail * f_frsize`.

12. **What is load average?**
    Exponentially-damped average of runnable + uninterruptible tasks (1/5/15
    min). It is a *count of tasks*, not a percentage; compare to CPU count.

13. **How did you calculate network throughput?**
    Read cumulative RX/TX bytes from `/proc/net/dev`, take the delta between
    two samples, divide by elapsed time; clamp negative deltas (counter reset)
    to zero.

14. **How does signal handling work?**
    `sigaction()` installs a handler for SIGINT/SIGTERM. The handler sets a
    `volatile sig_atomic_t` flag; the main loop polls it and performs cleanup.

15. **Why use `sig_atomic_t`?**
    It is the only type guaranteed to be readable/writable atomically even if
    a signal interrupts mid-operation. Other types could be torn.

16. **How does your alert state machine work?**
    Each resource tracks NORMAL/WARNING/CRITICAL. An event fires only on
    transition (or after cooldown), with a RECOVERY event on return to normal.

17. **How did you avoid alert flooding?**
    Transition-only events + a monotonic-clock cooldown (`ALERT_COOLDOWN`)
    suppress repeated same-severity notifications.

18. **Why avoid `system()`?**
    It spawns a shell (`/bin/sh -c`), introducing command-injection risk,
    portability issues, and overhead. Reading `/proc` directly is safer and
    faster.

19. **How did you prevent buffer overflows?**
    Bounded APIs (`snprintf`, `fgets`, `memcpy` with validated lengths),
    fixed-size buffers, overflow-checked numeric parsing, and no `strcpy`/
    `strcat`/`sprintf`/`gets`.

20. **How does systemd run your application?**
    A unit file sets `Type=simple`, `ExecStart`, `Restart=on-failure`,
    and hardening options (`ProtectSystem=full`, `NoNewPrivileges`).

21. **What happens if a monitored subsystem fails?**
    The subsystem returns an error, the engine logs a warning, and monitoring
    continues with the other subsystems. Only config/init failures are fatal.

22. **How did you test memory safety?**
    Zero heap allocation (no leak/double-free surface) plus UBSan (no UB);
    ASan is documented for native toolchains.

23. **What is AddressSanitizer?**
    A compiler instrumentation that detects memory errors (heap/stack
    overflow, use-after-free, leaks) at runtime.

24. **What is the difference between a process and a thread?**
    A process has its own address space; threads share an address space within
    a process. A process is a container; a thread is a unit of execution.

25. **What Linux concepts did you learn?**
    procfs/sysfs, the `/proc/<pid>/stat` parsing subtlety, delta-based rate
    calculation, `MemAvailable` vs `MemFree`, `statvfs`, monotonic clocks,
    async-signal-safe handlers, and defensive parsing of kernel-exposed data.
