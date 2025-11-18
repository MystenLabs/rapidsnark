#!/bin/bash

echo "Stopping prover server..."
pkill -f "proverServer" || true
screen -S rapidsnark -X quit 2>/dev/null || true
sleep 1

# Verify it's stopped
if pgrep -f "proverServer" > /dev/null; then
    echo "Warning: Server process still running"
    exit 1
fi

echo "✓ Server stopped"
