#!/bin/bash

if [ "$COMPLETE" == "" ]; then COMPLETE=y ; fi
# rm -rv output/data/temp/graphics/*

#delete all old Android packages:

BIN_NAME=output/ANDROID/bin/OpenSoar-unsigned.apk
rm -rv output/ANDROID/opt
rm -v  $BIN_NAME

echo "============================================================================="
echo "Make Android v8a (64 bit):"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/arm64-v8a/opt/src ; fi
# else rm -rv output/ANDROID/arm64-v8a/opt/src/Version.* ; fi
# rm -rv output/include/ProgramVersion.*

make DEBUG=n TARGET=ANDROIDAARCH64

if [ -e $BIN_NAME ]; then    # exit ; fi
cp -v $BIN_NAME output/ANDROID/bin/OpenSoar-${PROGRAM_VERSION}-64.apk 

if [ ! "$ANDROID_ARM64_ONLY" == "y" ]; then
echo "============================================================================="
echo "Make Android v7a (32 bit):"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/armeabi-v7a/opt/src ; fi
make DEBUG=n TARGET=ANDROID

echo "============================================================================="
echo "Make Android x64:"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/x86_64/opt/src ; fi
make DEBUG=n TARGET=ANDROIDX64

echo "============================================================================="
echo "Make Android x86:"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/x86/opt/src ; fi
make DEBUG=n TARGET=ANDROID86

cp -v $BIN_NAME output/ANDROID/bin/OpenSoar-${PROGRAM_VERSION}.apk 

if [ ! "$ANDROID_ONLY" == "y" ]
then 
echo "============================================================================="
echo "Make Win64:"
if [ "$COMPLETE" == "y" ]; then rm -rv output/WIN64/opt/src ; fi
make DEBUG=n TARGET=WIN64
cp -v output/WIN64/bin/OpenSoar.exe output/WIN64/bin/OpenSoar-${PROGRAM_VERSION}.exe 
echo "============================================================================="
echo "Make Linux:"
if [ "$COMPLETE" == "y" ]; then rm -rv output/UNIX/opt/src ; fi
make DEBUG=n TARGET=UNIX
cp -v output/UNIX/bin/OpenSoar output/UNIX/bin/OpenSoar-${PROGRAM_VERSION} 

# ANDROID_ONLY:
fi

else
echo "'OpenSoar-unsigned.apk' not available!" 
fi

# ANDROID_ARM64_ONLY:
fi
