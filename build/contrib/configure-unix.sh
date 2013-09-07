#!/bin/bash

# This script is optional, it is not an official part of the XCSoar
# build scripts.

# Configure script for XCSoar on Mac OS X.

# We're not using Autoconf
# but if we were, we're doing a poor man's variant of the following:

# AC_INIT(XCSoar, 1.0)
# #AC_PREREQ([2.59])
# #AM_INIT_AUTOMAKE([1.0 no-define])
# #AC_CONFIG_HEADERS([config.h])
# CXXFLAGS="-stdc=gnu++0x"
# AC_PROG_CC([gcc-4.6 gcc-mp-4.6 gcc-4.5 gcc-mp-4.5 gcc-4.4 gcc-mp-4.4 gcc cc])
# AC_PROG_CXX([g++-4.6 g++-mp-4.6 g++-4.5 g++-mp-4.5 g++-4.4 g++-mp-4.4 g++])
# AC_CONFIG_FILES([Makefile])
# AC_OUTPUT

DEST=`dirname $0`/../local-config.mk

# Test if running on a Mac
if [ "`uname -s`" != "Darwin" -a "`uname -s`" != "Linux" ]; 
then
    echo "Not running on Linux or Mac OS X.  Automatic configuration not available."
    rm ${DEST}
    exit 0
fi
if [ "`uname -s`" == "Darwin" ];
then
    LOCATOR="mdfind -name"
else
    LOCATOR="locate"
fi

# Current OS X distributions come with GCC 4.2, which is too old.

# look for possibly newer GCC versions in common locations
# gcc-4.6 - in PATH
# gcc-mp-4.6 - Macports in PATH
# /usr/local/bin/gcc-mp-4.7 - typical binary location

for prefix in "" /usr/local/bin/;
do
    for suffix in -4.7 -mp-4.7 -4.6 -mp-4.6 -4.5 -mp-4.5 -4.4 -mp-4.4 "";
    do
	CXX="${prefix}gcc${suffix}"
	location=`which $CXX`

	if [ -e "$location" ]; then
	    CXX=$location
	    LOCAL_TCPREFIX=`dirname "$location"`/
	    LOCAL_TCSUFFIX=${suffix}
	    break 2
	fi
    done
done


# Test to ensure compiler is new enough.
(echo "main(){}" | ${CXX} -std=gnu++0x -x c++ -o /dev/null -) || { echo "C compiler fails, or does not understand -std=gnu++0x.  GCC 4.3 or later needed!"; exit 1; }

echo "Using ${CXX} as compiler for UNIX targets."
echo "Using toolchain in ${LOCAL_TCPREFIX} for UNIX targets."

# Building for Android 

ANDROID_SDK=${ANDROID_SDK-`${LOCATOR} android-sdk-macosx | head -n1`}
if [ -d ${ANDROID_SDK} ]; then
  echo "Using Android SDK: ${ANDROID_SDK}"
  ANDROID_NDK=${ANDROID_NDK-${ANDROID_SDK}/../android-ndk-*}
  if [ ! -d ${ANDROID_NDK} ]; then
      ANDROID_NDK=${ANDROID_SDK}/android-ndk-*
  fi  
  if [ -d ${ANDROID_NDK} ]; then
      ANDROID_NDK=`cd ${ANDROID_NDK}; pwd`
      echo "Using Android NDK: ${ANDROID_NDK}"
  else
      echo "Android NDK not found (see http://developer.android.com/sdk/).  Cannot build for Android."
  fi

  ANDROID_PLATFORM=${ANDROID_PLATFORM-"android-8"}
  ANDROID_SDK_PLATFORM=${ANDROID_SDK}/platforms/${ANDROID_PLATFORM}
  ANDROID_NDK_PLATFORM=${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}

  if [ ! -d "${ANDROID_SDK_PLATFORM}" ]; then
      echo "Android SDK platform $ANDROID_PLATFORM not found.
  Install platform with ${ANDROID_SDK}/tools/android 
  or adapt targets.mk in order to build for Android."
  fi
  if [ ! -d "${ANDROID_NDK_PLATFORM}" ]; then
      echo "Android NDK platform ${ANDROID_PLATFORM} not found.  
  Install platform with ${ANDROID_SDK}/tools/android 
  or adapt targets.mk in order to build for Android.
  The NDK platform must match the SDK one.  It is set in targets.mk."
  fi

  if test `which oggenc`; then
      if [ -d "${ANDROID_SDK}" -a -d "${ANDROID_SDK_PLATFORM}" -a -d "${ANDROID_NDK_PLATFORM}" ]; then
	  echo "ANDROID7 and/or ANDROID targets seem available."
      fi
  else
      echo "oggenc is missing (try: brew install vorbis-tools, or port install vorbis-tools).  Cannot build for Android."
  fi

else
  echo "Android SDK not found (see http://developer.android.com/sdk/).  Cannot build for Android."
fi

# Output local-config.mk
# local-config is loaded twice
# if TARGET is not UNIX, do not override compiler

# we must set TARGET_LDFLAGS, as  -Wl,--gc-sections seems to be unknown to the linker.
# we're guaranteed that the host is unix, so we can set LOCAL_TCPREFIX.
# target.mk uses this for UNIX targets.

cat >${DEST} <<FINISHED || exit 1;

ANDROID_SDK = $ANDROID_SDK
ANDROID_NDK = $ANDROID_NDK

ifeq (\$(TARGET),)
TARGET=UNIX
endif

ifeq (\$(TARGET),UNIX)
TARGET_LDFLAGS = -static-libgcc
endif

# Best local toolchain:
LOCAL_TCPREFIX = $LOCAL_TCPREFIX
LOCAL_TCSUFFIX = $LOCAL_TCSUFFIX
FINISHED

echo "Ready to run make."
