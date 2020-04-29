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

target_output_dir = os.path.abspath(target_output_dir)

lib_path = os.path.join(target_output_dir, 'lib')
arch_path = os.path.join(lib_path, actual_host_triplet)
build_path = os.path.join(arch_path, 'build')
install_prefix = os.path.join(arch_path, 'root')

if 'MAKEFLAGS' in os.environ:
    # build/make.mk adds "--no-builtin-rules --no-builtin-variables",
    # which breaks the zlib Makefile (and maybe others)
    del os.environ['MAKEFLAGS']

class Toolchain:
    def __init__(self, tarball_path, src_path, build_path, install_prefix,
                 toolchain_arch, actual_arch, arch_cflags, cppflags,
                 arch_ldflags, cc, cxx, ar, arflags, ranlib, strip):
        self.tarball_path = tarball_path
        self.src_path = src_path
        self.build_path = build_path
        self.install_prefix = install_prefix
        self.toolchain_arch = toolchain_arch
        self.actual_arch = actual_arch

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

if 'mingw32' in actual_host_triplet:
    thirdparty_libs = [
        zlib,
        libsodium,
        openssl,
        curl,
        lua,
    ]
elif re.match('(arm.*|aarch64)-apple-darwin', actual_host_triplet) is not None:
    thirdparty_libs = [
        libsodium,
        openssl,
        curl,
        lua,
        proj,
        libtiff,
        libgeotiff,
        sdl2
    ]
elif 'apple-darwin' in actual_host_triplet:
    thirdparty_libs = [
        libsodium,
        lua,
        proj,
        libtiff,
        libgeotiff,
        sdl2
    ]
elif target == 'ANDROID':
    thirdparty_libs = [
        libsodium,
        openssl,
        netcdf,
        netcdfcxx,
        curl,
        lua,
        proj,
        libtiff,
        libgeotiff,
    ]
elif toolchain_host_triplet.endswith('-musleabihf'):
    thirdparty_libs = [
        zlib,
        libsodium,
        freetype,
        openssl,
        curl,
        libpng,
        libjpeg,
        lua,
        libsalsa,
        libusb,
        simple_usbmodeswitch,
    ]
else:
    thirdparty_libs = [
        musl,
        libstdcxx_musl_headers,
        zlib,
        libsodium,
        freetype,
        openssl,
        curl,
        libpng,
        libjpeg,
        lua,
        libsalsa,
        libusb,
        simple_usbmodeswitch,
    ]

# build the third-party libraries
toolchain = Toolchain(tarball_path, src_path, build_path, install_prefix,
                      toolchain_host_triplet, actual_host_triplet,
                      arch_cflags, cppflags, arch_ldflags, cc, cxx, ar, arflags,
                      ranlib, strip)
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
