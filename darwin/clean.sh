#!/bin/sh
set -xe

cd "$WORKSPACE_PATH/../../.." || exit 1
echo "Executing gmake clean in $(pwd)..."
export PATH="$(brew --prefix)"/bin:"$PATH"
gmake clean
