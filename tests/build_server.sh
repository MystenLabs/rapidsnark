#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "$SCRIPT_DIR"

echo "=== Building prover server ==="
echo ""

echo "Step 1/5: Installing npm dependencies..."
npm install

echo ""
echo "Step 2/5: Initializing git submodules..."
git submodule init
git submodule update

echo ""
echo "Step 3/5: Creating field sources..."
npx task createFieldSources

echo ""
echo "Step 4/5: Building Pistache library..."
npx task buildPistache

echo ""
echo "Step 5/5: Building prover server..."
npx task buildProverServer

echo ""
echo "✓ Build complete!"
echo "  Binary: $SCRIPT_DIR/build/proverServer"
