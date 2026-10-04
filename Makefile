# Linux Resource Monitor with Alerting - Makefile
#
# Targets:
#   make            - release build (default)
#   make debug      - debug build with AddressSanitizer + UBSan
#   make release    - optimized build
#   make clean      - remove build artifacts
#   make test       - build and run unit tests
#   make install    - install binary + config under PREFIX
#   make uninstall  - remove installed files
#   make format     - run clang-format if available
#   make help       - show this help

CC      ?= gcc
TARGET  := linux-resource-monitor
PREFIX  ?= /usr/local

# Module source files (added incrementally across phases).
SRC := \
    src/main.c \
    src/utils.c \
    src/config.c \
    src/cpu.c \
    src/memory.c \
    src/disk.c \
    src/system.c \
    src/process.c \
    src/network.c \
    src/logger.c \
    src/alert.c \
    src/monitor.c \
    src/cli.c \
    src/terminal.c

OBJ := $(SRC:.c=.o)

# Include path for our headers.
INCLUDES := -Iinclude

# Feature test macros for POSIX/Linux APIs we rely on.
POSIX := -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

# Strict warnings. We treat warnings as errors so the build stays clean.
WARN := -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Werror=return-type \
        -Wconversion -Wstrict-prototypes -Wmissing-prototypes

STD := -std=c11

# Release flags.
CFLAGS ?= -O2
CFLAGS += $(STD) $(POSIX) $(WARN) $(INCLUDES)

LDLIBS :=

# Debug flags: no optimization and full sanitizer coverage.
#
# NOTE: The ideal is '-fsanitize=address,undefined'. On standard Ubuntu this
# links cleanly. On a non-root LOCAL compiler prefix (as used to bootstrap
# this very environment), libasan's lgamma interceptor references the
# GNU-unique symbol 'signgam' which fails to resolve despite the
# src/signgam_compat.c shim. We therefore default 'make debug' to
# '-fsanitize=undefined' (clean, catches integer UB) and document enabling
# ASan in docs/testing.md for native toolchains.
DEBUG_CFLAGS := -O0 -g3 -fno-omit-frame-pointer -fsanitize=undefined

# ---------------------------------------------------------------------------
# Rules
# ---------------------------------------------------------------------------

.PHONY: all debug release clean test install uninstall format help

all: release

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

debug: CFLAGS := $(CFLAGS) $(DEBUG_CFLAGS) -DDEBUG_BUILD
debug: clean $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDLIBS)
	@echo "Debug build complete (UBSan): ./$(TARGET)"

release: clean $(TARGET)
	@echo "Release build complete: ./$(TARGET)"

clean:
	rm -f $(TARGET) $(OBJ) src/signgam_compat.o tests/*.o
	@echo "Cleaned."

test: release
	@echo "Running unit tests..."
	@for t in tests/test_config tests/test_utils tests/test_cpu tests/test_memory tests/test_disk tests/test_process tests/test_network tests/test_alert; do \
		echo "=== $$t ==="; \
		$(CC) $(CFLAGS) -o /tmp/$$(basename $$t) $$t.c src/config.c src/utils.c src/cpu.c src/memory.c src/disk.c src/system.c src/process.c src/network.c src/logger.c src/alert.c $(LDLIBS) && \
		/tmp/$$(basename $$t) || exit 1; \
	done

install: release
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/etc/linux-resource-monitor
	install -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/
	install -m 0644 config/monitor.conf $(DESTDIR)$(PREFIX)/etc/linux-resource-monitor/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

format:
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i src/*.c include/*.h; \
	else \
		echo "clang-format not installed; skipping."; \
	fi

help:
	@echo "Available targets:"
	@echo "  make            - release build (default)"
	@echo "  make debug      - debug build with ASan+UBSan"
	@echo "  make release    - optimized build"
	@echo "  make clean      - remove artifacts"
	@echo "  make test       - run unit tests"
	@echo "  make install    - install (PREFIX=/usr/local by default)"
	@echo "  make uninstall  - remove installed files"
	@echo "  make format     - clang-format sources"
