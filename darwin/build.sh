#!/bin/bash
set -euo pipefail

# This script is meant for execution by Xcode.
# Note: We could let Xcode pass through the build settings in the environment, but this currently at least breaks host compiling of tools (GenerateSineTable will then be build for target) and maybe has other unwanted side effects ... so we pass individual settings as script arguments.

# Input validation
if [ $# -lt 2 ]; then
    echo "This script is meant to be executed by Xcode." >&2
    echo "Usage: $0 <target_platform_name> <configuration>" >&2
    echo "  target_platform_name: iphonesimulator, iphoneos, or macosx" >&2
    echo "  configuration: Debug or Release" >&2
    exit 1
fi

readonly TARGET_PLATFORM_NAME="$1"
readonly CONFIGURATION="$2"

# Set debug flag based on configuration
DEBUG="n"
if [ "${CONFIGURATION}" = "Debug" ]; then
    DEBUG="y"
fi
export DEBUG

# Function to safely remove and create symlink
create_symlink() {
    local target="$1"
    local link_name="$2"
    rm -f "$link_name"
    ln -s "$target" "$link_name"
}

# Set the make TARGET based on selected device
# and linking all executables to the same path, so we can use one build scheme
EXECUTABLE_PATH="$(pwd)/darwin/XCSoarExecutable"
case "$TARGET_PLATFORM_NAME" in
    iphonesimulator)
        TARGET="IOS64SIM"
        create_symlink "$(pwd)/output/IOS64SIM/ipa/Payload/XCSoar.app" "$EXECUTABLE_PATH"
        ;;
    iphoneos)
        TARGET="IOS64"
        # Clean up previous build artifacts to ensure .ipa is fresh
        rm -rf "$(pwd)/output/IOS64/ipa" "$(pwd)/output/IOS64/xcsoar.ipa" "$(pwd)/output/IOS64/xcsoar-signed.ipa" "$(pwd)"/output/IOS64/Info.plist*
        create_symlink "$(pwd)/output/IOS64/xcsoar-signed.ipa" "$EXECUTABLE_PATH"
        ;;
    macosx)
        TARGET="MACOS"
        create_symlink "$(pwd)/output/MACOS/bin/xcsoar" "$EXECUTABLE_PATH"
        ;;
    *)
        echo "Error: Unsupported platform: $TARGET_PLATFORM_NAME" >&2
        echo "Supported platforms: iphonesimulator, iphoneos, macosx" >&2
        exit 1
        ;;
esac

export TARGET

echo "build.sh: Executing gmake for target $TARGET"

# Set IPA target based on platform
IPA_TARGET="ipa"
if [ "$TARGET" = "MACOS" ]; then
    IPA_TARGET=""
fi

# Ensure homebrew gmake is in PATH (by default not the case within Xcode sandbox)
export PATH="/opt/homebrew/bin:$PATH"

# Get number of CPUs for parallel compilation
NUM_CPUS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "4")

echo "Building with $NUM_CPUS parallel jobs..."

# Execute make with error checking
if ! gmake -j"${NUM_CPUS}" USE_CCACHE=y V=2 OPTIMIZE="-O0" DEBUG="$DEBUG" TARGET="$TARGET" $IPA_TARGET; then
    echo "Error: Build failed" >&2
    exit 1
fi

# Sign the iOS build if needed
if [ "$TARGET" = "IOS64" ]; then
    echo "Signing iOS build..."
    if [ -f "$(pwd)/darwin/sign.sh" ]; then
        if ! "$(pwd)/darwin/sign.sh"; then
            echo "Warning: Signing failed" >&2
        fi
    else
        echo "Warning: sign.sh not found" >&2
    fi
fi

echo "Build completed successfully for target $TARGET"
