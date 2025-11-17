# rapidsnark Tests

This directory contains tests for the rapidsnark prover server.

## DoS Protection Test

### Overview

`test_dos_protection.sh` verifies that the server's DoS protection mechanisms work correctly by sending multiple waves of concurrent requests.

### What it tests

1. **Request rejection at capacity** - Server rejects requests when concurrent limit is reached
2. **Semaphore release** - Server accepts new requests after previous ones complete
3. **Consistent limit enforcement** - Server maintains the configured concurrent request limit

### Prerequisites

- Server must be running with standard configuration
- Sample input file must be available (default: `../binaries/sampleZKPInputs.json`)

### Usage

Basic usage (assumes server running on localhost:8080):
```bash
cd tests
./test_dos_protection.sh
```

Custom configuration:
```bash
# Use different server URL
SERVER_URL=http://example.com:8080/input ./test_dos_protection.sh

# Use different input file
SAMPLE_INPUT=/path/to/input.json ./test_dos_protection.sh

# Both
SERVER_URL=http://example.com:8080/input SAMPLE_INPUT=/path/to/input.json ./test_dos_protection.sh
```

### Expected Results

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
