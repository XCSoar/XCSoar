#!/bin/sh

set -e

# set ANDROID variables
ANDROID_SDK_TOOLS_VERSION=3859397
ANDROID_BUILD_TOOLS_VERSION=26.0.0
ANDROID_PLATFORM_VERSION=22
ANDROID_NDK_VERSION=r19c
ANDROID_REPO_URL=https://dl.google.com/android/repository
ANDROID_SDK_DIR=~/opt/android-sdk-linux
ANDROID_NDK_DIR=~/opt/android-ndk-${ANDROID_NDK_VERSION}

if [ -d ${ANDROID_SDK_DIR} ]
then
  echo Not installing Android NDK, because ${ANDROID_SDK_DIR} exists already
else
  echo Installing Android SDK to ${ANDROID_SDK_DIR}...

  android_sdk_tmp_zip=$(mktemp)
  wget --progress=bar:force:noscroll \
      ${ANDROID_REPO_URL}/sdk-tools-linux-${ANDROID_SDK_TOOLS_VERSION}.zip \
      -O ${android_sdk_tmp_zip}

  mkdir -p ${ANDROID_SDK_DIR}/licenses
  cd ${ANDROID_SDK_DIR}
  unzip ${android_sdk_tmp_zip}

  rm ${android_sdk_tmp_zip}

  echo 8933bad161af4178b1185d1a37fbf41ea5269c55 > licenses/android-sdk-license
  echo d56f5187479451eabf01fb78af6dfcb131a6481e >> licenses/android-sdk-license
fi
echo

cd ~/opt/android-sdk-linux
echo Installing Android SDK packages...
tools/bin/sdkmanager "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" \
    "platforms;android-${ANDROID_PLATFORM_VERSION}"
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
