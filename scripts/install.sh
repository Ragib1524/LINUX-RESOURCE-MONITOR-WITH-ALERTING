#!/bin/bash
#
# scripts/install.sh
#
# Install the Linux Resource Monitor. This script ONLY builds and installs:
#   - the compiled C binary
#   - the example configuration
#
# All monitoring logic is in C; this script performs no data collection. It
# optionally installs a systemd unit if systemd is present.
#
# Usage: sudo ./scripts/install.sh [PREFIX]
#
set -euo pipefail

PREFIX="${1:-/usr/local}"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "==> Building release binary (make release)..."
cd "$PROJECT_DIR"
make release

echo "==> Installing to ${PREFIX}..."
install -d "${PREFIX}/bin"
install -m 0755 linux-resource-monitor "${PREFIX}/bin/"

install -d "${PREFIX}/etc/linux-resource-monitor"
install -m 0644 config/monitor.conf "${PREFIX}/etc/linux-resource-monitor/"

install -d "${PREFIX}/share/doc/linux-resource-monitor"
install -m 0644 README.md "${PREFIX}/share/doc/linux-resource-monitor/" 2>/dev/null || true

# Optional systemd unit.
if command -v systemctl >/dev/null 2>&1 && [ -d /run/systemd/system ]; then
    echo "==> systemd detected; installing unit..."
    install -d /etc/systemd/system
    install -m 0644 systemd/linux-resource-monitor.service /etc/systemd/system/
    systemctl daemon-reload
    echo "    Enable with: systemctl enable --now linux-resource-monitor"
else
    echo "==> systemd not available; skipping unit install (core monitor still works)."
fi

echo "==> Done. Binary: ${PREFIX}/bin/linux-resource-monitor"
