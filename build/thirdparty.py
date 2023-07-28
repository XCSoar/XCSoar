#!/usr/bin/env -S python3 -u

import os, os.path
import re
import sys

if len(sys.argv) != 13:
    print("Usage: build.py LIB_PATH HOST_TRIPLET ARCH_CFLAGS CPPFLAGS ARCH_LDFLAGS CC CXX AR ARFLAGS RANLIB STRIP WINDRES", file=sys.stderr)
    sys.exit(1)

lib_path, host_triplet, arch_cflags, cppflags, arch_ldflags, cc, cxx, ar, arflags, ranlib, strip, windres = sys.argv[1:]

# the path to the XCSoar sources
xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')

# output directories
from build.dirs import tarball_path, src_path

lib_path = os.path.abspath(lib_path)
build_path = os.path.join(lib_path, 'build')
install_prefix = os.path.join(lib_path, host_triplet)

if 'MAKEFLAGS' in os.environ:
    # build/make.mk adds "--no-builtin-rules --no-builtin-variables",
    # which breaks the zlib Makefile (and maybe others)
    del os.environ['MAKEFLAGS']

# a list of third-party libraries to be used by XCSoar
from build.libs import *

if 'mingw32' in host_triplet:
    thirdparty_libs = [
        zlib,
        libfmt,
        libsodium,
        cares,
        curl,
        lua,
    ]

    # Some libraries (such as CURL) want to use the min()/max() macros
    cppflags = cppflags.replace('-DNOMINMAX', '')

    # Explicitly disable _FORTIFY_SOURCE because it is broken with
    # mingw.  This prevents some libraries such as libsodium to enable
    # it.
    cppflags += ' -D_FORTIFY_SOURCE=0'
elif 'apple-darwin' in host_triplet:
    thirdparty_libs = [
        zlib,
        libfmt,
        libsodium,
        openssl,
        cares,
        curl,
        lua,
        sqlite3,
        proj,
        libtiff,
        libgeotiff,
        sdl2
    ]
elif 'android' in host_triplet:
    thirdparty_libs = [
        libfmt,
        libsodium,
        openssl,
        cares,
        curl,
        lua,
        sqlite3,
        proj,
        libtiff,
        libgeotiff,
    ]
elif '-kobo-linux-' in host_triplet:
    thirdparty_libs = [
        binutils,
        linux_headers,
        gcc_bootstrap,
        musl,
        gcc,
        zlib,
        libfmt,
        libsodium,
        freetype,
        openssl,
        cares,
        curl,
        libpng,
        libjpeg,
        lua,
        libsalsa,
        libusb,
        simple_usbmodeswitch,
    ]
else:
    raise RuntimeError('Unrecognized target')

# build the third-party libraries
from build.toolchain import Toolchain, NativeToolchain
toolchain = Toolchain(xcsoar_path, lib_path,
                      tarball_path, src_path, build_path, install_prefix,
                      host_triplet,
                      arch_cflags, cppflags, arch_ldflags, cc, cxx, ar, arflags,
                      ranlib, strip, windres)
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
