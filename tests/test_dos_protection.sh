#!/bin/bash
#
# DoS Protection Test for rapidsnark prover server
# 
# This test sends multiple waves of concurrent requests to verify:
# 1. Server rejects requests when at capacity
# 2. Semaphore is properly released after request completion
# 3. Server maintains consistent concurrent request limit

set -e

# Configuration
SERVER_URL="${SERVER_URL:-http://localhost:8080/input}"
SAMPLE_INPUT="${SAMPLE_INPUT:-sampleZKPInputs.json}"

if [ ! -f "$SAMPLE_INPUT" ]; then
    echo "Error: Sample input file not found at $SAMPLE_INPUT"
    echo "Please set SAMPLE_INPUT environment variable or ensure binaries/sampleZKPInputs.json exists"
    exit 1
fi

echo "=== DoS Protection Test ==="
echo "Server URL: $SERVER_URL"
echo "Sample input: $SAMPLE_INPUT"
echo ""

# Test function to send a single request
send_request() {
    local wave=$1
    local num=$2
    
    response=$(curl -s -w "\n%{http_code}" -X POST \
        -H "Content-Type: application/json" \
        -d @"$SAMPLE_INPUT" "$SERVER_URL" 2>&1)
    
    http_code=$(echo "$response" | tail -n1)
    echo "Wave $wave - Request $num: HTTP $http_code"
}

# Wave 1: 25 concurrent requests (expect 20 accepted, 5 rejected)
echo "Wave 1: Sending 25 concurrent requests..."
for i in {1..25}; do
    send_request 1 $i &
done
wait
echo "Wave 1 complete."
echo ""

sleep 1
echo ""

# Wave 2: 22 concurrent requests (verify semaphore is released)
echo "Wave 2: Sending 22 concurrent requests..."
for i in {1..22}; do
    send_request 2 $i &
done
wait
echo "Wave 2 complete."
echo ""

# Wait again
sleep 1
echo ""

# Wave 3: 30 concurrent requests (stress test)
echo "Wave 3: Sending 30 concurrent requests (stress test)..."
for i in {1..30}; do
    send_request 3 $i &
done
wait
echo "Wave 3 complete."
echo ""

echo "=== Test Complete ==="
echo "Check server logs for detailed timing and rejection information."
