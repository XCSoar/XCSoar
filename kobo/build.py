#!/usr/bin/env python3

import os, os.path
import sys, subprocess

if len(sys.argv) != 8:
    print("Usage: build.py TARGET_OUTPUT_DIR HOST_ARCH CC CXX AR RANLIB STRIP", file=sys.stderr)
    sys.exit(1)

target_output_dir, host_arch, cc, cxx, ar, ranlib, strip = sys.argv[1:]
host_triplet = 'arm-linux-gnueabihf'
arch_ldflags = ''

# the path to the XCSoar sources
xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')
from build.project import Project
from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.freetype import FreeTypeProject

output_path = os.path.abspath('output')
tarball_path = os.path.join(output_path, 'download')
src_path = os.path.join(output_path, 'src')
target_output_dir = os.path.abspath(target_output_dir)
target_root = os.path.join(target_output_dir, 'root')

lib_path = os.path.join(target_output_dir, 'lib')
arch_path = os.path.join(lib_path, host_arch)
build_path = os.path.join(arch_path, 'build')
root_path = os.path.join(arch_path, 'root')

target_arch = '-march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard'
cppflags = '-isystem ' + os.path.join(root_path, 'include') + ' -DNDEBUG'
ldflags = '-L' + os.path.join(root_path, 'lib')
libs = ''

# redirect pkg-config to use our root directory instead of the default
# one on the build host
os.environ['PKG_CONFIG_LIBDIR'] = os.path.join(root_path, 'lib/pkgconfig')

if 'MAKEFLAGS' in os.environ:
    # build/make.mk adds "--no-builtin-rules --no-builtin-variables",
    # which breaks the zlib Makefile (and maybe others)
    del os.environ['MAKEFLAGS']

class Toolchain:
    def __init__(self, tarball_path, src_path, build_path, install_prefix,
                 arch, arch_cflags, cppflags, arch_ldflags,
                 cc, cxx, ar, ranlib, strip):
        self.tarball_path = tarball_path
        self.src_path = src_path
        self.build_path = build_path
        self.install_prefix = install_prefix
        self.arch = arch

        self.cc = cc
        self.cxx = cxx
        self.ar = ar
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
toolchain = Toolchain(tarball_path, src_path, build_path, root_path,
                      host_triplet, target_arch, cppflags, arch_ldflags,
                      cc, cxx, ar, ranlib, strip)
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
