#!/bin/bash
set -euo pipefail

# This script is meant for execution by Xcode.

# Ensure homebrew gmake is in PATH (by default not the case within Xcode sandbox)
export PATH="/opt/homebrew/bin:$PATH"

echo "Executing gmake clean in $(pwd)..."
gmake clean
