# rapidsnark Tests

This directory contains tests for the rapidsnark prover server and corresponding test inputs.

## Test Scripts

### Build & Server Management

- **`build_server.sh`** - Build the prover server binary
- **`start_server.sh`** - Start the prover server in a screen session
- **`stop_server.sh`** - Stop the prover server

### Individual Tests

- **`test_basic.sh`** - Basic proof generation functionality test
- **`test_dos_protection.sh`** - DoS protection mechanism test

### Test Suite

- **`run_all_tests.sh`** - Run all tests automatically (starts server, runs tests, stops server)

## Test Inputs

`sampleZKPInputs.json` contains the test inputs, generated using the [genZKPInputs.ts script](https://github.com/MystenLabs/zklogin-circuits/blob/main/bin/genZKPInputs.ts) and [sample wallet inputs](https://github.com/MystenLabs/zklogin-circuits/blob/main/testvectors/sampleWalletInputs.json).

## Quick Start

```bash
# Build the server
./tests/build_server.sh

# Run all tests
./tests/run_all_tests.sh
```

## Usage

### Building

First-time setup or after code changes:

```bash
cd tests
./build_server.sh
```

### Running All Tests

```bash
cd tests
./run_all_tests.sh
```

This will automatically start the server, run all tests, and stop the server.

### Manual Testing

For debugging or running individual tests:

```bash
# Start the server
./start_server.sh

# Run individual tests
./test_basic.sh
./test_dos_protection.sh

# Stop the server when done
./stop_server.sh
```

### Configuration

All scripts support environment variable configuration:

```bash
# Use custom paths
ZKEY_PATH=/path/to/custom.zkey \
WITNESS_BINARIES_PATH=/path/to/binaries \
TEST_INPUT=/path/to/input.json \
./run_all_tests.sh

# Use different server port
SERVER_PORT=9090 ./start_server.sh

# Custom log file location
LOG_FILE=/tmp/my-prover.log ./start_server.sh
```

## Test Details

### Basic Functionality Test

`test_basic.sh` verifies that the server can successfully generate proofs.

**What it tests:**
- Server accepts valid proof requests
- Returns HTTP 200 status
- Response contains valid JSON
- Proof includes required fields (pi_a, pi_b, pi_c)

**Expected result:** Successful proof generation with valid output

### DoS Protection Test

`test_dos_protection.sh` verifies that the server's DoS protection mechanisms work correctly by sending multiple waves of concurrent requests.

**What it tests:**
1. **Request rejection at capacity** - Server rejects requests when concurrent limit is reached
2. **Semaphore release** - Server accepts new requests after previous ones complete
3. **Consistent limit enforcement** - Server maintains the configured concurrent request limit

**Expected results:**

With default configuration (10 concurrent request limit):

**Wave 1** (15 concurrent requests):
- 10 requests accepted (HTTP 200)
- 5 requests rejected (HTTP 503)

**Wave 2** (12 concurrent requests after 3s wait):
- 10 requests accepted (HTTP 200)
- 2 requests rejected (HTTP 503)

**Wave 3** (20 concurrent requests after 6s wait):
- 10 requests accepted (HTTP 200)
- 10 requests rejected (HTTP 503)
