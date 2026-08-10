#!/bin/bash
set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ZKEY_PATH="${ZKEY_PATH:-$SCRIPT_DIR/binaries/zkLogin.zkey}"
WITNESS_GRAPH_PATH="${WITNESS_GRAPH_PATH:-$SCRIPT_DIR/binaries/zkLogin.bin}"
SERVER_PORT="${SERVER_PORT:-8080}"
LOG_FILE="${LOG_FILE:-/tmp/prover.log}"

cd "$SCRIPT_DIR"

echo "=== Configuration ==="
echo "ZKEY_PATH: $ZKEY_PATH"
echo "WITNESS_GRAPH_PATH: $WITNESS_GRAPH_PATH"
echo "SERVER_PORT: $SERVER_PORT"
echo "LOG_FILE: $LOG_FILE"
echo ""

# Validate required files exist
if [ ! -f "$ZKEY_PATH" ]; then
    echo "ERROR: ZKEY file not found at $ZKEY_PATH"
    exit 1
fi

if [ ! -f "$WITNESS_GRAPH_PATH" ]; then
    echo "ERROR: WITNESS_GRAPH file not found at $WITNESS_GRAPH_PATH"
    exit 1
fi

if [ ! -f "$SCRIPT_DIR/build/proverServer" ]; then
    echo "ERROR: proverServer binary not found. Please build first:"
    echo "  npm install && npx task createFieldSources && npx task buildPistache && npx task buildProverServer"
    exit 1
fi

# Kill any existing server
echo "Stopping any existing server..."
pkill -f "proverServer" || true
screen -S rapidsnark -X quit 2>/dev/null || true
sleep 1

# Start server
echo "Starting server..."
rm -f "$LOG_FILE"
screen -dmS rapidsnark bash -c \
  "cd $SCRIPT_DIR && \
   ZKEY=$ZKEY_PATH \
   WITNESS_GRAPH=$WITNESS_GRAPH_PATH \
   PORT=$SERVER_PORT \
   LD_LIBRARY_PATH=$SCRIPT_DIR/depends/pistache/build/src:$SCRIPT_DIR/depends/circom-witnesscalc/target/release:\$LD_LIBRARY_PATH \
   ./build/proverServer 2>&1 | tee $LOG_FILE"

# Wait for server to be ready
echo "Waiting for server to start..."
for i in {1..30}; do
    if curl -s http://localhost:$SERVER_PORT/input > /dev/null 2>&1; then
        echo "✓ Server is ready on port $SERVER_PORT"
        echo "  Logs: $LOG_FILE"
        echo "  Screen session: rapidsnark"
        exit 0
    fi
    sleep 1
done

echo "ERROR: Server failed to start within 30 seconds"
echo "=== Server logs ==="
cat "$LOG_FILE"
exit 1
