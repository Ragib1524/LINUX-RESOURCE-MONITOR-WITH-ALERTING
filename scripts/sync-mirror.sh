#!/bin/bash
# Sync the canonical WSL project to the Windows Desktop mirror (and back).
# Usage: scripts/sync-mirror.sh [wsl2desktop|desktop2wsl]
set -e
SRC="/mnt/c/Users/Md Ragib Raza/Desktop/LINUX RESOURCE MONITOR WITH ALERTING"
DST="$HOME/linux-resource-monitor"
case "${1:-both}" in
  wsl2desktop|both)
    rm -rf "$SRC/src" "$SRC/include" "$SRC/tests" "$SRC/config" "$SRC/logs" "$SRC/scripts" "$SRC/systemd" "$SRC/docs"
    for d in src include tests config logs scripts systemd docs; do cp -r "$DST/$d" "$SRC/" ; done
    for f in Makefile README.md .gitignore LICENSE; do cp "$DST/$f" "$SRC/" ; done
    ;;
esac
case "${1:-both}" in
  desktop2wsl|both)
    for d in src include tests config logs scripts systemd docs; do cp -r "$SRC/$d/." "$DST/$d/" 2>/dev/null || true; done
    for f in Makefile README.md .gitignore LICENSE; do cp "$SRC/$f" "$DST/$f" 2>/dev/null || true; done
    ;;
esac
echo "Sync complete ($1)"
