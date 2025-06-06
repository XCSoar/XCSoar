#!/bin/sh

# Note: We could let Xcode pass through the build settings in the environment, but this currently at least breaks host compiling of tools (GenerateSineTable will then be build for target) and maybe has other unwanted side effects ... so we pass individual settings as script arguments.

TARGET_PLATFORM_NAME=$1
CONFIGURATION=$2

export DEBUG="n"
if [ "${CONFIGURATION}" = "Debug" ]; then
  export DEBUG="y"
fi

case "$TARGET_PLATFORM_NAME" in
  iphonesimulator)
    export TARGET="IOS64SIM"
    rm "$(pwd)"/darwin/XCSoarExecutable
    ln -s "$(pwd)"/output/IOS64SIM/ipa/Payload/XCSoar.app "$(pwd)"/darwin/XCSoarExecutable
    ;;
  iphoneos)
    export TARGET="IOS64"
    rm "$(pwd)"/darwin/XCSoarExecutable
    ln -s "$(pwd)"/output/IOS64/ipa/Payload/XCSoar.app "$(pwd)"/darwin/XCSoarExecutable
    ;;
  macosx)
    export TARGET="MACOS"
    rm "$(pwd)"/darwin/XCSoarExecutable
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
export PATH=/opt/homebrew/bin:$PATH # for gmake to be in path
gmake -j"$(nproc)" USE_CCACHE=y V=2 OPTIMIZE='-O0' DEBUG=$DEBUG TARGET=$TARGET $IPA_TARGET
