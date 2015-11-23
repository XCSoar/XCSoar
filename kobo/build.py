#!/usr/bin/env python3

import os, os.path
import sys

if len(sys.argv) != 7:
    print("Usage: build.py TARGET_OUTPUT_DIR HOST_TRIPLET CC CXX AR STRIP", file=sys.stderr)
    sys.exit(1)

target_output_dir, host_triplet, cc, cxx, ar, strip = sys.argv[1:]

# the path to the XCSoar sources
xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')

# output directories
from build.dirs import tarball_path, src_path

target_output_dir = os.path.abspath(target_output_dir)

lib_path = os.path.join(target_output_dir, 'lib')
arch_path = os.path.join(lib_path, host_triplet)
build_path = os.path.join(arch_path, 'build')
install_prefix = os.path.join(arch_path, 'root')

if 'MAKEFLAGS' in os.environ:
    # build/make.mk adds "--no-builtin-rules --no-builtin-variables",
    # which breaks the zlib Makefile (and maybe others)
    del os.environ['MAKEFLAGS']

class KoboToolchain:
    def __init__(self, tarball_path, src_path, build_path, install_prefix, arch, cc, cxx, ar, strip):
        self.tarball_path = tarball_path
        self.src_path = src_path
        self.build_path = build_path
        self.install_prefix = install_prefix
        self.arch = arch

        self.cc = cc
        self.cxx = cxx
        self.ar = ar
        self.strip = strip

        arch_flags = '-march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard'
        common_flags = '-Os -g -ffunction-sections -fdata-sections -fvisibility=hidden ' + arch_flags
        self.cflags = common_flags
        self.cxxflags = common_flags
        self.cppflags = '-isystem ' + os.path.join(install_prefix, 'include') + ' -DNDEBUG'
        self.ldflags = '-L' + os.path.join(install_prefix, 'lib')
        self.libs = ''

        self.env = dict(os.environ)

        # redirect pkg-config to use our root directory instead of the
        # default one on the build host
        self.env['PKG_CONFIG_LIBDIR'] = os.path.join(install_prefix, 'lib/pkgconfig')

# a list of third-party libraries to be used by XCSoar
from build.libs import *
thirdparty_libs = [
    zlib,
    freetype,
    curl,
    libpng,
    libjpeg,
    lua,
]

# build the third-party libraries
toolchain = KoboToolchain(tarball_path, src_path, build_path, install_prefix,
                          host_triplet, cc, cxx, ar, strip)
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
