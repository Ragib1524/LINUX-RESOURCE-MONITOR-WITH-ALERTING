#!/bin/bash
#
# scripts/uninstall.sh
#
# Remove the Linux Resource Monitor binary, config, and (optionally) the
# systemd unit. Reversible counterpart to install.sh.
#
# Usage: sudo ./scripts/uninstall.sh [PREFIX]
#
set -euo pipefail

PREFIX="${1:-/usr/local}"

echo "==> Stopping and disabling service (if present)..."
if command -v systemctl >/dev/null 2>&1; then
    systemctl disable --now linux-resource-monitor 2>/dev/null || true
    rm -f /etc/systemd/system/linux-resource-monitor.service
    systemctl daemon-reload 2>/dev/null || true
fi

echo "==> Removing installed files..."
rm -f "${PREFIX}/bin/linux-resource-monitor"
rm -rf "${PREFIX}/etc/linux-resource-monitor"
rm -rf "${PREFIX}/share/doc/linux-resource-monitor"

echo "==> Done."
