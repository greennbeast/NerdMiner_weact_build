#!/usr/bin/env zsh
set -euo pipefail

BAUD=${1:-115200}
LOG_DIR="logs"
mkdir -p "$LOG_DIR"
LOG_FILE="$LOG_DIR/monitor-$(date +%Y%m%d-%H%M%S).log"

echo "Saving to $LOG_FILE"
# Start monitor in background and capture output
(pio device monitor -b "$BAUD" | tee "$LOG_FILE") & MONPID=$!
# Capture ~10 seconds
sleep 10
# Gracefully stop the monitor
kill -INT $MONPID 2>/dev/null || true
sleep 1

echo "--- 10s capture complete ---"
echo "$LOG_FILE"