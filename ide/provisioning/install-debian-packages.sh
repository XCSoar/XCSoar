#!/bin/sh

set -e

echo Installing base dependencies...
apt-get --assume-yes install make \
  librsvg2-bin xsltproc \
  imagemagick gettext ffmpeg \
  git quilt zip \
  m4 automake wget \
  ttf-bitstream-vera fakeroot
echo

echo Installing Manual dependencies...
apt-get --assume-yes install texlive \
  texlive-latex-extra \
  texlive-luatex \
  texlive-lang-french \
  texlive-lang-polish \
  texlive-lang-portuguese \
  texlive-lang-german \
  liblocale-po-perl
echo

echo Installing dependencies for the Linux target...
apt-get --assume-yes install make g++ \
  zlib1g-dev \
  libfreetype6-dev \
  libpng-dev libjpeg-dev \
  libtiff5-dev libgeotiff-dev \
  libcurl4-openssl-dev \
  liblua5.2-dev lua5.2-dev \
  libxml-parser-perl \
  libasound2-dev \
  librsvg2-bin xsltproc \
  imagemagick gettext \
  mesa-common-dev libgl1-mesa-dev libegl1-mesa-dev \
  fonts-dejavu
echo

echo Installing dependencies for compiling with LLVM / Clang...
apt-get --assume-yes install llvm clang libc++-dev
echo

echo Installing dependencies for compiling targets which need libinput or GBM...
apt-get --assume-yes install libinput-dev libgbm-dev
echo

echo Installing dependencies for ARM Linux targets...
apt-get --assume-yes install g++-arm-linux-gnueabihf
echo

echo Installing PC/WIN64 dependencies...
apt-get --assume-yes install g++-mingw-w64
echo

echo Installing dependencies for the Android target, not including SDK / NDK...
apt-get --assume-yes install default-jdk-headless vorbis-tools adb
echo
