#!/bin/bash

set -e

# set ANDROID variables
ANDROID_SDK_TOOLS_VERSION=9477386_latest
ANDROID_BUILD_TOOLS_VERSION=33.0.2
ANDROID_PLATFORM_VERSION=33
ANDROID_NDK_VERSION=r26d
ANDROID_REPO_URL=https://dl.google.com/android/repository
ANDROID_SDK_DIR=~/opt/android-sdk-linux
ANDROID_NDK_DIR=~/opt/android-ndk-${ANDROID_NDK_VERSION}

if [ "$#" -eq 0 ]; then
  sections_to_install=("SDK" "NDK")
else
  for arg in "$@"
  do
     sections_to_install+=("$arg")
  done
fi


install_sdk() {
if [ -d "${ANDROID_SDK_DIR}"/build-tools/"${ANDROID_BUILD_TOOLS_VERSION}" ]
then
  echo "Not installing Android SDK, because ${ANDROID_SDK_DIR}/build-tools/${ANDROID_BUILD_TOOLS_VERSION} exists already"
else
  echo "Installing Android SDK to ${ANDROID_SDK_DIR}..."

  ANDROID_SDK_TMP_ZIP="$(mktemp)"
  wget --progress=bar:force:noscroll \
      ${ANDROID_REPO_URL}/commandlinetools-linux-${ANDROID_SDK_TOOLS_VERSION}.zip \
      -O "${ANDROID_SDK_TMP_ZIP}"

  mkdir -p "${ANDROID_SDK_DIR}"/licenses
  cd "${ANDROID_SDK_DIR}"
  unzip "${ANDROID_SDK_TMP_ZIP}"

  rm "${ANDROID_SDK_TMP_ZIP}"

  echo 24333f8a63b6825ea9c5514f83c2829b004d1fee > licenses/android-sdk-license
fi
echo

cd "${ANDROID_SDK_DIR}"
echo Installing Android SDK packages...
echo cmdline-tools/bin/sdkmanager --sdk_root="${ANDROID_SDK_DIR}" \
    "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" "platforms;android-${ANDROID_PLATFORM_VERSION}"
cmdline-tools/bin/sdkmanager --sdk_root=${ANDROID_SDK_DIR} \
    "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" "platforms;android-${ANDROID_PLATFORM_VERSION}"
echo
}

install_ndk() {
if [ -d "${ANDROID_NDK_DIR}" ]
then
  echo Not installing Android NDK, because ${ANDROID_NDK_DIR} exists already
else
  echo Installing Android NDK to ${ANDROID_NDK_DIR}...

  ANDROID_NDK_TMP_ZIP=$(mktemp)
  wget --progress=bar:force:noscroll \
      ${ANDROID_REPO_URL}/android-ndk-${ANDROID_NDK_VERSION}-linux.zip \
      -O "${ANDROID_NDK_TMP_ZIP}"

  mkdir -p ~/opt
  cd ~/opt || exit 1
  unzip "${ANDROID_NDK_TMP_ZIP}"

  rm "${ANDROID_NDK_TMP_ZIP}"
fi
echo
}

for section in "${sections_to_install[@]}"; do
  case $section in
    SDK)
     install_sdk
     ;;
    NDK)
     install_ndk
    ;;
    *)
    echo "Unkown section: $section"
    ;;
  esac
done
