#!/bin/bash

if [ "$COMPLETE" == "" ]; then COMPLETE=y ; fi
rm -rv output/data/temp/graphics/*

#delete all old Android packages:
rm -rv output/ANDROID/opt

echo "============================================================================="
echo "Make Android v8a (64 bit):"
if [ "$COMPLETE" == "y" ]; then rm -rv output/ANDROID/arm64-v8a/opt/src ; fi
make DEBUG=n TARGET=ANDROIDAARCH64
cp -v output/ANDROID/bin/OpenSoar-unsigned.apk output/ANDROID/bin/OpenSoar-${PROGRAM_VERSION}-64.apk 

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

cp -v output/ANDROID/bin/OpenSoar-unsigned.apk output/ANDROID/bin/OpenSoar-${PROGRAM_VERSION}.apk 

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

fi
