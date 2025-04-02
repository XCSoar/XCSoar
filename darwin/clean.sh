#!/bin/sh
cd "$WORKSPACE_PATH/../../.." || exit 1
echo "Executing gmake clean in $(pwd)..."
export PATH=/opt/homebrew/bin:"$PATH" 
gmake clean
