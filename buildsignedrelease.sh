#!/bin/bash

mkdir -p ~/.android
export ANDROID_KEYSTORE_PASS="U3gOlkVJbQbm"
make -j$(nproc) ANDROID_BUNDLE_BUILD="y" PLAY=y TARGET=ANDROIDFAT DEBUG=n USE_CCACHE=y \
ANDROID_SDK_PLATFORM=android-35 \
ANDROID_KEYSTORE=$(eval echo ~)/.android/signing-key.jks ANDROID_KEY_ALIAS=myalias \
output/ANDROID_BUNDLE/bin/XCSoar.aab
