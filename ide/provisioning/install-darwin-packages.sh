#!/bin/bash

set -e

sections_to_install=()

# Parse arguments
if [ "$#" -eq 0 ]; then
  sections_to_install=("BASE" "OSX64" "MACOS" "IOS")
else
  for arg in "$@"
  do
    sections_to_install+=("$arg")
  done
fi

install_base() {
  echo Installing base dependencies...
  brew install quilt \
    librsvg make \
    sox gettext \
    imagemagick
  echo
}

install_macOS() {
  echo Installing macOS dependencies...
  brew install fmt sdl2 \
    libsodium freetype \
    libpng libjpeg-turbo \
    libtiff libgeotiff \
    proj c-ares \
    curl lua
  echo
}

install_ios() {
  echo Installing dependencies for the iOS target...
  brew install automake autoconf \
    libtool \
    cmake ninja
  echo
}

for section in "${sections_to_install[@]}"; do
  case $section in
    BASE)
      install_base
      ;;
    OSX64)
      install_macOS # OSX64 and macOS have the same dependencies for now
      ;;
    MACOS)
      install_macOS
      ;;
    IOS)
      install_ios
      ;;
    *)
      echo "Unknown section: $section"
      ;;
  esac
done
