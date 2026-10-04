# Testing

## Unit tests

Run with `make test`. Each suite is a standalone C program (no external test
framework) that returns non-zero on failure:

| Suite | Covers |
|-------|--------|
| `test_config.c` | default values, parsing, whitespace/comments, invalid numeric, missing `=`, validation ranges, unknown keys, missing file |
| `test_utils.c` | overflow-safe integer parsers, double parser, trimmer, bounded file reader, monotonic clock |
| `test_cpu.c` | utilization math, idle, zero-delta, backwards counters, NULL safety, real `/proc/stat` |
| `test_memory.c` | usage calc, clamping, zero-total, real `/proc/meminfo` |
| `test_disk.c` | real `statvfs`, invalid/empty/NULL paths |
| `test_process.c` | real scan, sort, per-process CPU% calc, NULL/zero safety |
| `test_network.c` | real read, loopback exclusion, rate calc, counter reset, NULL safety |
| `test_alert.c` | NORMAL→WARNING→CRITICAL→RECOVERY, cooldown re-alert, critical-first, NULL safety |

Total: **112 assertions**.

## Failure testing

The tests deliberately cover hostile inputs:

- missing `/proc` files (simulated by nonexistent paths)
- malformed config (non-numeric, missing `=`, out-of-range)
- `PID` disappearing mid-scan (the `process_scan` loop tolerates ENOENT)
- zero CPU delta, backwards counters
- network counter reset
- invalid/empty/NULL arguments

## Sanitizers

`make debug` builds with `-fsanitize=undefined` (clean; no UB reported).

### AddressSanitizer note

The ideal build uses `-fsanitize=address,undefined`. On a **standard** Ubuntu
install (root-installed `gcc`/`libc`), this links and runs cleanly.

On a **non-root local toolchain prefix** (used to bootstrap this exact
environment, where GCC/libc were extracted to `~/.local` without root), the
static `libasan.a` interceptor for `lgamma` references the GNU-unique symbol
`signgam`, which fails to resolve at link time (a linker/GNU-unique quirk, not
a code defect). A `src/signgam_compat.c` shim is provided; if it does not
resolve the symbol on your toolchain, use `-fsanitize=undefined` (the more
relevant sanitizer for this integer-heavy code).

Since the codebase performs **zero heap allocation**, ASan's heap leak/overflow
detection is of limited value here; UBSan's integer-overflow/shift checks are
the higher-value instrumentation.

To build with ASan natively:

```bash
cc -O1 -g -fsanitize=address,undefined -Iinclude -o mon src/*.c
```
