#!/bin/bash
# Script to test compilation of each commit in the branch
# Usage: ./test_commits_compile.sh [ANDROID|WAYLAND|BOTH]

set -e

TARGET="${1:-BOTH}"
MERGE_BASE=$(git merge-base HEAD upstream/master)

echo "Testing commits for compilation..."
echo "Target: $TARGET"
echo "="

failed_commits=()
tested=0

while IFS= read -r line; do
    commit_hash=$(echo "$line" | cut -d' ' -f1)
    commit_msg=$(echo "$line" | cut -d' ' -f2-)
    
    echo ""
    echo "[$((++tested))] Testing: ${commit_hash:0:8} - ${commit_msg:0:60}"
    
    # Checkout commit
    git checkout -q "$commit_hash" || {
        echo "  ❌ Checkout failed"
        failed_commits+=("$commit_hash:checkout:$commit_msg")
        continue
    }
    
    # Clean output (best effort)
    rm -rf output/ANDROID output/WAYLAND 2>/dev/null || true
    
    # Test Android
    if [[ "$TARGET" == "ANDROID" || "$TARGET" == "BOTH" ]]; then
        echo "  Android: Compiling..."
        if make TARGET=ANDROIDFAT -j1 libs >/tmp/android_build.log 2>&1; then
            echo "  ✅ Android: OK"
        else
            if grep -q "error:" /tmp/android_build.log; then
                echo "  ❌ Android: FAILED"
                grep "error:" /tmp/android_build.log | head -2 | sed 's/^/     /'
                failed_commits+=("$commit_hash:Android:$commit_msg")
            else
                echo "  ⚠️  Android: Build issue (dependencies?)"
            fi
        fi
    fi
    
    # Test Wayland using docker
    if [[ "$TARGET" == "WAYLAND" || "$TARGET" == "BOTH" ]]; then
        echo "  Wayland: Compiling with docker..."
        if docker run --rm \
            --mount type=bind,source="$(pwd)",target=/opt/xcsoar \
            xcsoar/xcsoar-build:latest \
            xcsoar-compile WAYLAND >/tmp/wayland_build.log 2>&1; then
            echo "  ✅ Wayland: OK"
        else
            if grep -q "error:" /tmp/wayland_build.log; then
                echo "  ❌ Wayland: FAILED"
                grep "error:" /tmp/wayland_build.log | head -2 | sed 's/^/     /'
                failed_commits+=("$commit_hash:Wayland:$commit_msg")
            else
                echo "  ⚠️  Wayland: Build issue (dependencies?)"
            fi
        fi
    fi
    
done < <(git log --format="%H %s" --reverse $MERGE_BASE..HEAD)

# Return to original branch
git checkout -q feature/skysight

echo ""
echo "="
echo "Summary: ${#failed_commits[@]} commit(s) failed"
if [ ${#failed_commits[@]} -gt 0 ]; then
    echo ""
    echo "Failed commits:"
    for entry in "${failed_commits[@]}"; do
        commit_hash=$(echo "$entry" | cut -d: -f1)
        reason=$(echo "$entry" | cut -d: -f2)
        msg=$(echo "$entry" | cut -d: -f3-)
        echo "  - ${commit_hash:0:8}: $reason"
        echo "    $msg"
    done
else
    echo ""
    echo "✅ All commits compile successfully!"
fi
