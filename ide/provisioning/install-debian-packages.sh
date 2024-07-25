#!/bin/bash

set -e

export DEBIAN_FRONTEND=noninteractive
declare -A APTOPTS
APTOPTS[1]="--assume-yes"
APTOPTS[2]="--no-install-recommends"

sections_to_install=()

# Parse arguments
if [ "$#" -eq 0 ]; then
  sections_to_install=("UPDATE" "BASE" "MANUAL" "LINUX" "WAYLAND" "DEBIAN" "LLVM" "LIBINPUT_GBM" "ARM" "WIN" "KOBO" "ANDROID")
else
  for arg in "$@"
  do
    sections_to_install+=("$arg")
  done
fi

update_pkg() {
  echo Updating Repositories
  apt-get update
}

install_base() {
  echo Installing base dependencies...
  apt-get install ${APTOPTS[*]} make \
    librsvg2-bin xsltproc \
    imagemagick gettext sox \
    git quilt zip \
    m4 automake wget \
    pkg-config cmake ninja-build ccache \
    ca-certificates
  echo
}

install_manual() {
  echo Installing Manual dependencies...
  apt-get install ${APTOPTS[*]} texlive \
    texlive-latex-extra \
    texlive-luatex \
    texlive-lang-french \
    texlive-lang-polish \
    texlive-lang-portuguese \
    texlive-lang-german \
    liblocale-po-perl
  echo
}

install_linux() {
  echo Installing dependencies for the Linux target...
  apt-get install ${APTOPTS[*]} make g++ \
    zlib1g-dev \
    libfmt-dev \
    libdbus-1-dev \
    libsodium-dev \
    libfreetype6-dev \
    libpng-dev libjpeg-dev \
    libtiff5-dev libgeotiff-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libc-ares-dev \
    liblua5.4-dev \
    libxml-parser-perl \
    libasound2-dev \
    libsdl2-dev \
    librsvg2-bin xsltproc \
    imagemagick gettext \
    mesa-common-dev libgl1-mesa-dev libegl1-mesa-dev \
    fonts-dejavu \
    xz-utils
  echo
}

install_wayland() {
  echo Installing dependencies for the Wayland target...
  apt-get install ${APTOPTS[*]} wayland-protocols \
    libwayland-bin
  echo
}

install_debian() {
  echo Installing dependencies for creating Debian package
  apt-get install ${APTOPTS[*]} dpkg-dev \
    debhelper \
    texlive-lang-english \
    libio-captureoutput-perl \
    build-essential
  echo
}

install_llvm() {
  echo Installing dependencies for compiling with LLVM / Clang...
  apt-get install ${APTOPTS[*]} llvm clang libc++-dev libc++abi-dev
  echo
}

install_libinput_gbm() {
  echo Installing dependencies for compiling targets which need libinput or GBM...
  apt-get install ${APTOPTS[*]} libinput-dev libgbm-dev
  echo
}

install_arm() {
  echo Installing dependencies for ARM Linux targets...
  apt-get install ${APTOPTS[*]} g++-arm-linux-gnueabihf \
    libmpc-dev \
    meson
  echo
}

install_win() {
  echo Installing PC/WIN64 dependencies...
  apt-get install ${APTOPTS[*]} g++-mingw-w64
  echo
}

install_kobo() {
  echo Installing Kobo dependencies...
  apt-get install ${APTOPTS[*]} \
      texinfo \
      fakeroot \
      python3-setuptools \
      ttf-bitstream-vera
  echo
}

install_android() {
  echo Installing dependencies for the Android target, not including SDK / NDK...
  apt-get install ${APTOPTS[*]} default-jdk-headless vorbis-tools adb libtool \
      unzip
  echo
}

for section in "${sections_to_install[@]}"; do
  case $section in
    BASE)
      install_base
      ;;
    MANUAL)
      install_manual
      ;;
    LINUX)
      install_linux
      ;;
    WAYLAND)
      install_wayland
      ;;
    DEBIAN)
      install_debian
      ;;
    LLVM)
      install_llvm
      ;;
    LIBINPUT_GBM)
      install_libinput_gbm
      ;;
    ARM)
      install_arm
      ;;
    WIN)
      install_win
      ;;
    KOBO)
      install_kobo
      ;;
    ANDROID)
      install_android
      ;;
    UPDATE)
      update_pkg
      ;;
    clean)
      clean_pkg
      ;;
    *)
      echo "Unknown section: $section"
      ;;
  esac
done

clean_pkg() {
  echo Clean up downloaded resources in order to free space
  apt-get clean
  echo
}
