#!/bin/sh

set -e

# set ANDROID variables
ANDROID_SDK_TOOLS_VERSION=6858069_latest
ANDROID_BUILD_TOOLS_VERSION=29.0.3
ANDROID_PLATFORM_VERSION=29
ANDROID_NDK_VERSION=r22b
ANDROID_REPO_URL=https://dl.google.com/android/repository
ANDROID_SDK_DIR=~/opt/android-sdk-linux
ANDROID_NDK_DIR=~/opt/android-ndk-${ANDROID_NDK_VERSION}

if [ -d ${ANDROID_SDK_DIR}\build-tools\${ANDROID_BUILD_TOOLS_VERSION} ]
then
  echo Not installing Android SDK, because ${ANDROID_SDK_DIR}\build-tools\${ANDROID_BUILD_TOOLS_VERSION} exists already
else
  if [ -d ${ANDROID_SDK_DIR} ]
  then
    echo Remove old Android SDK  '${ANDROID_SDK_DIR}'...
    rm -r ${ANDROID_SDK_DIR}
  fi
  echo Installing Android SDK to ${ANDROID_SDK_DIR}...

  android_sdk_tmp_zip=$(mktemp)
  wget --progress=bar:force:noscroll \
      ${ANDROID_REPO_URL}/commandlinetools-linux-${ANDROID_SDK_TOOLS_VERSION}.zip \
      -O ${android_sdk_tmp_zip}

  mkdir -p ${ANDROID_SDK_DIR}/licenses
  cd ${ANDROID_SDK_DIR}
  unzip ${android_sdk_tmp_zip}

  rm ${android_sdk_tmp_zip}

  echo 24333f8a63b6825ea9c5514f83c2829b004d1fee > licenses/android-sdk-license
fi
echo

cd ${ANDROID_SDK_DIR}
echo Installing Android SDK packages...
echo cmdline-tools/bin/sdkmanager --sdk_root=${ANDROID_SDK_DIR} \
    "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" "platforms;android-${ANDROID_PLATFORM_VERSION}"
cmdline-tools/bin/sdkmanager --sdk_root=${ANDROID_SDK_DIR} \
    "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" "platforms;android-${ANDROID_PLATFORM_VERSION}"
echo

if [ -d ${ANDROID_NDK_DIR} ]
then
  echo Not installing Android NDK, because ${ANDROID_NDK_DIR} exists already
else
  echo Installing Android NDK to ${ANDROID_NDK_DIR}...

  android_ndk_tmp_zip=$(mktemp)
  wget --progress=bar:force:noscroll \
      ${ANDROID_REPO_URL}/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.zip \
      -O ${android_ndk_tmp_zip}

  cd ~/opt
  unzip ${android_ndk_tmp_zip}

  rm ${android_ndk_tmp_zip}
fi
echo
