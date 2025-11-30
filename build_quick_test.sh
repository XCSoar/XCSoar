#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Quick test script - builds a subset of common configurations

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

echo "XCSoar Quick Build Test"
echo "======================"
echo ""

# Common test targets
TEST_TARGETS=(
    "UNIX"
    "PC"
)

# Common test flag combinations
TEST_FLAGS=(
    "DEBUG=y"
    "DEBUG=n"
    "DEBUG=n LTO=y"
)

SUCCESS=0
FAILED=0

for target in "${TEST_TARGETS[@]}"; do
    echo "Building target: $target"
    
    # Default build
    if make TARGET="$target" clean all > /dev/null 2>&1; then
        echo "  ✓ Default: SUCCESS"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "  ✗ Default: FAILED"
        FAILED=$((FAILED + 1))
    fi
    
    # With flags
    for flags in "${TEST_FLAGS[@]}"; do
        if make TARGET="$target" $flags clean all > /dev/null 2>&1; then
            echo "  ✓ $flags: SUCCESS"
            SUCCESS=$((SUCCESS + 1))
        else
            echo "  ✗ $flags: FAILED"
            FAILED=$((FAILED + 1))
        fi
    done
    echo ""
done

echo "Summary: $SUCCESS successful, $FAILED failed"



