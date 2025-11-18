#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_INPUT="${TEST_INPUT:-$SCRIPT_DIR/tests/sampleZKPInputs.json}"
SERVER_PORT="${SERVER_PORT:-8080}"

echo "=== Basic Functionality Test ==="

# Send test request
response=$(curl -s -w "\n%{http_code}" -X POST http://localhost:$SERVER_PORT/input \
    -H "Content-Type: application/json" \
    -d @"$TEST_INPUT")

http_code=$(echo "$response" | tail -n1)
body=$(echo "$response" | sed '$d')

echo "HTTP Status: $http_code"
echo "Response: $body"
echo ""

# Validate response
if [ "$http_code" != "200" ]; then
    echo "✗ FAILED: Expected HTTP 200, got $http_code"
    exit 1
fi

if ! echo "$body" | jq empty 2>/dev/null; then
    echo "✗ FAILED: Response is not valid JSON"
    exit 1
fi

# Check for expected proof fields
if ! echo "$body" | jq -e '.pi_a' > /dev/null 2>&1; then
    echo "✗ FAILED: Missing proof field 'pi_a'"
    exit 1
fi

echo "✓ PASSED: Proof generated successfully"
