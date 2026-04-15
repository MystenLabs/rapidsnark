#!/usr/bin/env bash
# Build and run the witness-calc integration test.
#
# Verifies the full witness path used by SingleProver:
#   1. load .bin graph from disk
#   2. call gw_calc_witness() from libcircom_witnesscalc
#   3. parse the wtns buffer in-memory via BinFileUtils::openFromBuffer
#   4. write the wtns to disk
#   5. validate it with `npx snarkjs wtns check <r1cs> <wtns>`
#
# Usage:
#   GRAPH=path/to/circuit.bin \
#   INPUTS=path/to/inputs.json \
#   R1CS=path/to/circuit.r1cs \
#   bash tests/test_witness.sh
#
# Defaults point at the cached zkLogin artifacts under
# ~/.cache/zklogin/artifacts/zkLogin-016e978 and tests/sampleZKPInputs.json.

set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/zklogin/artifacts/zkLogin-016e978"

GRAPH="${GRAPH:-$CACHE/zkLogin-016e978.bin}"
R1CS="${R1CS:-$CACHE/zkLogin-016e978.r1cs}"
INPUTS="${INPUTS:-$REPO/tests/sampleZKPInputs.json}"

if [[ ! -f "$GRAPH" ]]; then echo "missing graph: $GRAPH" >&2; exit 1; fi
if [[ ! -f "$INPUTS" ]]; then echo "missing inputs: $INPUTS" >&2; exit 1; fi

LIB_DIR="$REPO/depends/circom-witnesscalc/target/release"
if [[ ! -f "$LIB_DIR/libcircom_witnesscalc.a" ]]; then
    echo "circom-witnesscalc not built; run: npx task buildWitnesscalc" >&2
    exit 1
fi

mkdir -p "$REPO/build"
BIN="$REPO/build/witness_test"

PLATFORM_LINK=""
if [[ "$(uname -s)" == "Darwin" ]]; then
    PLATFORM_LINK="-framework Security -framework CoreFoundation"
fi

echo "==> Building witness_test..."
g++ -std=c++20 -O2 \
    -I"$REPO/src" \
    -I"$REPO/depends/circom-witnesscalc/include" \
    "$REPO/tests/witness_test.cpp" \
    "$REPO/src/binfile_utils.cpp" \
    -L"$LIB_DIR" -lcircom_witnesscalc \
    $PLATFORM_LINK \
    -o "$BIN"

WTNS="$(mktemp /tmp/witness_test.XXXXXX.wtns)"
trap 'rm -f "$WTNS"' EXIT

echo "==> Running witness_test..."
"$BIN" "$GRAPH" "$INPUTS" "$WTNS"

echo "==> Verifying witness against R1CS with snarkjs..."
npx --yes snarkjs wtns check "$R1CS" "$WTNS"
echo "    ✓ witness is valid"
