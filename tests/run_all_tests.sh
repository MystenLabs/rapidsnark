#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FAILED_TESTS=()
PASSED_TESTS=()

echo "========================================"
echo "  Running All Tests"
echo "========================================"
echo ""

# Start server
echo "Starting server..."
if ! "$SCRIPT_DIR/start_server.sh"; then
    echo "✗ Failed to start server"
    exit 1
fi
echo ""

# Run tests
run_test() {
    local test_name=$1
    local test_script=$2
    
    echo "Running $test_name..."
    if "$test_script"; then
        PASSED_TESTS+=("$test_name")
    else
        FAILED_TESTS+=("$test_name")
    fi
    echo ""
}

run_test "Basic Functionality" "$SCRIPT_DIR/test_basic.sh"
run_test "DoS Protection" "$SCRIPT_DIR/test_dos_protection.sh"

# Stop server
echo "Stopping server..."
"$SCRIPT_DIR/stop_server.sh"
echo ""

# Summary
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo "Passed: ${#PASSED_TESTS[@]}"
for test in "${PASSED_TESTS[@]}"; do
    echo "  ✓ $test"
done

if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    echo ""
    echo "Failed: ${#FAILED_TESTS[@]}"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  ✗ $test"
    done
    exit 1
fi

echo ""
echo "✓ All tests passed!"
