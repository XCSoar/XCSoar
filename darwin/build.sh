#!/bin/sh
set -xe

# Note: We could let Xcode pass through the build settings in the environment, but this currently at least breaks host compiling of tools (GenerateSineTable will then be build for target) and maybe has other unwanted side effects ... so we pass individual settings as script arguments.

TARGET_PLATFORM_NAME=$1
CONFIGURATION=$2

export DEBUG="n"
if [ "${CONFIGURATION}" = "Debug" ]; then
  export DEBUG="y"
fi

# setting the make TARGET based on selected device
# and linking all executables to the same path, so we can use one build scheme
case "$TARGET_PLATFORM_NAME" in
  iphonesimulator)
    export TARGET="IOS64SIM"
    rm -f "$(pwd)"/darwin/XCSoarExecutable
    ln -s "$(pwd)"/output/IOS64SIM/ipa/Payload/XCSoar.app "$(pwd)"/darwin/XCSoarExecutable
    ;;
  iphoneos)
    export TARGET="IOS64"
    rm -rf "$(pwd)"/output/IOS64/ipa "$(pwd)"/output/IOS64/xcsoar.ipa "$(pwd)"/output/IOS64/Info.plist*
    rm -f "$(pwd)"/darwin/XCSoarExecutable
    ln -s "$(pwd)"/output/IOS64/xcsoar.ipa "$(pwd)"/darwin/XCSoarExecutable
    ;;
  macosx)
    export TARGET="MACOS"
    rm -f "$(pwd)"/darwin/XCSoarExecutable
    ln -s "$(pwd)"/output/MACOS/bin/xcsoar "$(pwd)"/darwin/XCSoarExecutable
    ;;
  *)
    echo "Unsupported platform: $TARGET_PLATFORM_NAME"
    exit 1
    ;;
esac

echo "ios/build.sh: Executing gmake for target $TARGET"
IPA_TARGET="ipa"
if [ "$TARGET" = "MACOS" ]; then
  IPA_TARGET=""
fi

export PATH=/opt/homebrew/bin:"$PATH" # for gmake to be in path
NUM_CPUS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu)

gmake -j"${NUM_CPUS}" USE_CCACHE=y V=2 OPTIMIZE='-O0' DEBUG=$DEBUG TARGET=$TARGET $IPA_TARGET

if [ "$TARGET" = "IOS64" ]; then
  "$(pwd)"/darwin/sign.sh
fi
