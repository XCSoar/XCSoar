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


# Test if running on a Mac
if [ "`uname -s`" != "Darwin" ]; 
then
    echo "Not running on a Mac.  Nothing to do for us."
    exit 0
fi


# Current OS X distributions come with GCC 4.2, which is too old.

# look for possibly newer GCC versions in common locations
# gcc-4.6 - in PATH
# gcc-mp-4.6 - Macports in PATH
# /usr/local/bin/gcc - typical binary location

echo $0

DEST=`dirname $0`/local-config.mk

for i in \$a-4.7 \$a-mp-4.7 \$a-4.6 \$a-mp-4.6 \$a-4.5 \$a-mp-4.5 \$a-4.4 \$a-mp-4.4 /usr/local/bin/\$a-4.6 /usr/local/bin/\$a \$a;
do
    a=gcc
    eval name=$i
    location=`which $name`

    if [ -e "$location" ]; then

	CC=$location

	a=g++
	eval name=$i
	CXX=`which $name`

	a=cpp
	eval name=$i
	HOSTCPP=`which $name`

	HOSTCC=$CC
	HOSTCXX=$CXX

	break
	fi

done

# Test to ensure compiler is new enough.
(echo "main(){}" | ${CXX} -std=gnu++0x -x c++ -o /dev/null -) || { echo "C compiler fails, or does not understand -std=gnu++0x.  GCC 4.3 or later needed!"; exit 1; }

# Output local-config.mk
# local-config is loaded twice
# if TARGET is not UNIX, do not override compiler

# we must set TARGET_LDFLAGS, as  -Wl,--gc-sections seems to be unknown to the linker.

cat >${DEST} <<FINISHED || exit 1;

ifeq (\$(TARGET),)
TARGET=UNIX
endif

ifeq (\$(TARGET),UNIX)
TARGET_LDFLAGS = -static-libgcc
CC = "$CC"
CXX = "$CXX"
HOSTCC = "$HOSTCC"
HOSTCPP = "$HOSTCPP"
HOSTCXX = "$HOSTCXX"
LINK = "$CXX"
endif
FINISHED

echo "Using ${CXX} as compiler."
echo "Ready to run make."
