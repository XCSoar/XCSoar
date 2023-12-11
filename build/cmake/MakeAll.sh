#!/bin/bash

if [ "$COMPLETE" == "" ]; then COMPLETE=y; fi

BIN_NAME=output/ANDROID/bin/OpenSoar-unsigned.apk

#delete all old Android packages:
rm -rv output/ANDROID/opt
rm -v  $BIN_NAME

echo "============================================================================="
echo "Make Android v8a (64 bit):"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/arm64-v8a/opt/src; fi

make DEBUG=n TARGET=ANDROIDAARCH64
echo "Android arm64-v8a ready!"

if [ -e $BIN_NAME ]; then
if [ "$EXPORT_ANDROID_ARM64" == "y" ]; then cp -v $BIN_NAME output/ANDROID/bin/OpenSoar-${PROGRAM_VERSION}-64.apk; fi 

if [ ! "$ANDROID_ARM64_ONLY" == "y" ]; then
echo "============================================================================="
echo "Make Android v7a (32 bit):"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/armeabi-v7a/opt/src; fi
make DEBUG=n TARGET=ANDROID
echo "Android armeabi_v7a ready!"

echo "============================================================================="
echo "Make Android x64:"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/x86_64/opt/src; fi
make DEBUG=n TARGET=ANDROIDX64
echo "Android x86_64 ready!"

echo "============================================================================="
echo "Make Android x86:"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/x86/opt/src; fi
make DEBUG=n TARGET=ANDROID86
echo "Android x86 ready!"

cp -v $BIN_NAME output/ANDROID/bin/OpenSoar-${PROGRAM_VERSION}.apk 

if [ ! "$ANDROID_ONLY" == "y" ]; then 
echo "============================================================================="
echo "Make Win64:"
if [ "$COMPLETE" == "y" ]; then rm -rv output/WIN64/opt/src; fi
make DEBUG=n TARGET=WIN64
echo "Win64 ready!"
cp -v output/WIN64/bin/OpenSoar.exe output/WIN64/bin/OpenSoar-${PROGRAM_VERSION}.exe 
echo "============================================================================="
echo "Make Linux:"
if [ "$COMPLETE" == "y" ]; then rm -rv output/UNIX/opt/src; fi
make DEBUG=n TARGET=UNIX
echo "UNIX ready!"
cp -v output/UNIX/bin/OpenSoar output/UNIX/bin/OpenSoar-${PROGRAM_VERSION} 

# ANDROID_ONLY:
else
echo "ANDROID_ONLY = $ANDROID_ONLY"
fi

else
echo "ANDROID_ARM64_ONLY = $ANDROID_ARM64_ONLY"
fi

else
echo "'$BIN_NAME' not available!" 
fi
