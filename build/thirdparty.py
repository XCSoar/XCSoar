#!/usr/bin/env python3

import os
import sys

def usage():
    print("Usage: thirdparty.py TARGET_OUTPUT_DIR TARGET HOST_TRIPLET ACTUAL_HOST_TRIPLET ARCH_CFLAGS CPPFLAGS ARCH_LDFLAGS CC CXX AR ARFLAGS RANLIB STRIP", file=sys.stderr)
    print("   or: thirdparty.py --unpack-shared-sources TARGET_OUTPUT_DIR TARGET", file=sys.stderr)

unpack_shared_source_only = False

if len(sys.argv) < 2:
    usage()
    sys.exit(1)


if sys.argv[1] == '--unpack-shared-sources':
    if len(sys.argv) != 4:
        usage()
        sys.exit(1)
    target_output_dir, target = sys.argv[2:]
    # Initialize the remainder Toolchain initializers which are not required for unpacking
    toolchain_host_triplet = 'deepthought-hitchhiker-klingon'
    actual_host_triplet = toolchain_host_triplet
    arch_cflags = ' '
    cppflags = ' '
    arch_ldflags = ' '
    cc = 'false'
    cxx = 'false'
    ar = 'false'
    arflags = ' '
    ranlib = 'false'
    strip = 'false'

    unpack_shared_source_only = True
else:
    if len(sys.argv) != 14:
        usage()
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

# Create the common download and (shared) source
toolchain.make_unpack_paths()
# Create the build base paths
if not unpack_shared_source_only:
    toolchain.make_build_paths()

libs = Libs(toolchain)

thirdparty_libs = libs_for_target(libs, toolchain, target)

for x in thirdparty_libs:
    if not unpack_shared_source_only:
        if not x.check_installed():
            # Delete the build directory if it existed, and (re-)create it empty
            x.make_clean_build_path()

            if not x.check_downloaded():
                x.download()

            if not x.check_unpacked():
                x.unpack()

            x.build()
    else: # if unpack_shared_source_only
        # unpack shared sources only
        if x.out_of_tree:
            if not x.check_downloaded():
                x.download()

            if not x.check_unpacked():
                x.unpack()
