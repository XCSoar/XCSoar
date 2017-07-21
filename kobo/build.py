#!/usr/bin/env python3

import os, os.path
import sys

if len(sys.argv) != 9:
    print("Usage: build.py TARGET_OUTPUT_DIR HOST_TRIPLET CC CXX AR ARFLAGS RANLIB STRIP", file=sys.stderr)
    sys.exit(1)

target_output_dir, host_triplet, cc, cxx, ar, arflags, ranlib, strip = sys.argv[1:]
arch_ldflags = ''

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

target_arch = '-march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard'
cppflags = ''

if 'MAKEFLAGS' in os.environ:
    # build/make.mk adds "--no-builtin-rules --no-builtin-variables",
    # which breaks the zlib Makefile (and maybe others)
    del os.environ['MAKEFLAGS']

class Toolchain:
    def __init__(self, tarball_path, src_path, build_path, install_prefix,
                 arch, arch_cflags, cppflags, arch_ldflags,
                 cc, cxx, ar, arflags, ranlib, strip):
        self.tarball_path = tarball_path
        self.src_path = src_path
        self.build_path = build_path
        self.install_prefix = install_prefix
        self.arch = arch

        self.cc = cc
        self.cxx = cxx
        self.ar = ar
        self.arflags = arflags
        self.ranlib = ranlib
        self.strip = strip

        common_flags = '-Os -g -ffunction-sections -fdata-sections -fvisibility=hidden ' + arch_cflags
        self.cflags = common_flags
        self.cxxflags = common_flags
        self.cppflags = '-isystem ' + os.path.join(install_prefix, 'include') + ' -DNDEBUG ' + cppflags
        self.ldflags = '-L' + os.path.join(install_prefix, 'lib') + ' ' + arch_ldflags
        self.libs = ''

        self.env = dict(os.environ)

        # redirect pkg-config to use our root directory instead of the
        # default one on the build host
        self.env['PKG_CONFIG_LIBDIR'] = os.path.join(install_prefix, 'lib/pkgconfig')

        # WORKAROUND: Under some circumstances, if QEMU User Emulation is
        # installed on the build system, and enabled for binfmt_misc, it can
        # break detection of cross compiling (at least autoconf's cross
        # compiling detection), which can lead to undesired behaviour.
        # Setting the following nonsense environment variable values should
        # always circumvent QEMU User Emulation.
        self.env['QEMU_CPU'] = 'Deep Thought'
        self.env['QEMU_GUEST_BASE'] = '42'

# a list of third-party libraries to be used by XCSoar
from build.libs import *
thirdparty_libs = [
    zlib,
    freetype,
    curl,
    libpng,
    libjpeg,
]

# build the third-party libraries
toolchain = Toolchain(tarball_path, src_path, build_path, install_prefix,
                      host_triplet, target_arch, cppflags, arch_ldflags,
                      cc, cxx, ar, arflags, ranlib, strip)
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
