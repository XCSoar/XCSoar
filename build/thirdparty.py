#!/usr/bin/env python3

import os, os.path
import re
import sys


if len(sys.argv) != 14:
    print("Usage: build.py TARGET_OUTPUT_DIR TARGET HOST_TRIPLET ACTUAL_HOST_TRIPLET ARCH_CFLAGS CPPFLAGS ARCH_LDFLAGS CC CXX AR ARFLAGS RANLIB STRIP", file=sys.stderr)
    sys.exit(1)

target_output_dir, target, toolchain_host_triplet, actual_host_triplet, arch_cflags, cppflags, arch_ldflags, cc, cxx, ar, arflags, ranlib, strip = sys.argv[1:]

# the path to the XCSoar sources
xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')

# output directories
from build.dirs import tarball_path, src_path

# other imports
from build.toolchain import Toolchain
from build.libs import Libs
from build.libsfortarget import libs_for_target


target_output_dir = os.path.abspath(target_output_dir)

lib_path = os.path.join(target_output_dir, 'lib')
arch_path = os.path.join(lib_path, actual_host_triplet)
build_path = os.path.join(arch_path, 'build')
install_prefix = os.path.join(arch_path, 'root')

if 'MAKEFLAGS' in os.environ:
    # build/make.mk adds "--no-builtin-rules --no-builtin-variables",
    # which breaks the zlib Makefile (and maybe others)
    del os.environ['MAKEFLAGS']

# build the third-party libraries
toolchain = Toolchain(tarball_path, src_path, build_path, install_prefix,
                      toolchain_host_triplet, actual_host_triplet,
                      arch_cflags, cppflags, arch_ldflags, cc, cxx, ar, arflags,
                      ranlib, strip)

libs = Libs(toolchain)

thirdparty_libs = libs_for_target(libs, toolchain, target)

for x in thirdparty_libs:
    if not x.is_installed():
        x.build()
