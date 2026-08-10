#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "$SCRIPT_DIR"

echo "=== Building prover server ==="
echo ""

echo "Step 1/6: Installing npm dependencies..."
npm install

echo ""
echo "Step 2/6: Initializing git submodules..."
git submodule init
git submodule update

echo ""
echo "Step 3/6: Creating field sources..."
npx task createFieldSources

echo ""
echo "Step 4/6: Building Pistache library..."
npx task buildPistache

echo ""
echo "Step 5/6: Building circom-witnesscalc library..."
npx task buildWitnesscalc

echo ""
echo "Step 6/6: Building prover server..."
npx task buildProverServer

echo ""
echo "✓ Build complete!"
echo "  Binary: $SCRIPT_DIR/build/proverServer"
