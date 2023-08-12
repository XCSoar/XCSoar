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

class NativeToolchain:
    """A toolchain for building native binaries, e.g. to be run on the
    build host."""

    def __init__(self, other):
        self.native = self

        self.tarball_path = other.tarball_path
        self.src_path = other.src_path
        self.build_path = other.build_path
        self.install_prefix = lib_path
        self.host_triplet = None
        self.is_windows = None

        self.cc = 'ccache gcc'
        self.cxx = 'ccache g++'
        self.ar = 'ar'
        self.arflags = ''
        self.ranlib = 'ranlib'
        self.strip = 'strip'
        self.windres = None

        common_flags = '-Os -ffunction-sections -fdata-sections -fvisibility=hidden'
        self.cflags = common_flags
        self.cxxflags = common_flags
        self.cppflags = '-DNDEBUG'
        self.ldflags = ''
        self.libs = ''

        self.env = dict(os.environ)

class Toolchain:
    def __init__(self, tarball_path, src_path, build_path, install_prefix,
                 host_triplet, arch_cflags, cppflags,
                 arch_ldflags, cc, cxx, ar, arflags, ranlib, strip, windres):
        self.tarball_path = tarball_path
        self.src_path = src_path
        self.build_path = build_path
        self.install_prefix = install_prefix
        self.host_triplet = host_triplet
        self.is_windows = 'mingw32' in host_triplet

        self.cc = cc
        self.cxx = cxx
        self.ar = ar
        self.arflags = arflags
        self.ranlib = ranlib
        self.strip = strip
        self.windres = windres

        common_flags = '-Os -g -ffunction-sections -fdata-sections -fvisibility=hidden ' + arch_cflags
        self.cflags = common_flags
        self.cxxflags = common_flags
        self.cppflags = '-isystem ' + os.path.join(install_prefix, 'include') + ' -DNDEBUG ' + cppflags
        self.ldflags = '-L' + os.path.join(install_prefix, 'lib') + ' ' + arch_ldflags
        self.libs = ''

        self.env = dict(os.environ)

        # redirect pkg-config to use our root directory instead of the
        # default one on the build host
        import shutil
        bin_dir = os.path.join(install_prefix, 'bin')
        os.makedirs(bin_dir, exist_ok=True)
        self.pkg_config = shutil.copy(os.path.join(xcsoar_path, 'build', 'pkg-config.sh'),
                                      os.path.join(bin_dir, 'pkg-config'))
        self.env['PKG_CONFIG'] = self.pkg_config

        # WORKAROUND: Under some circumstances, if QEMU User Emulation is
        # installed on the build system, and enabled for binfmt_misc, it can
        # break detection of cross compiling (at least autoconf's cross
        # compiling detection), which can lead to undesired behaviour.
        # Setting the following nonsense environment variable values should
        # always circumvent QEMU User Emulation.
        self.env['QEMU_CPU'] = 'Deep Thought'
        self.env['QEMU_GUEST_BASE'] = '42'

        self.native = NativeToolchain(self)

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
toolchain = Toolchain(tarball_path, src_path, build_path, install_prefix,
                      host_triplet,
                      arch_cflags, cppflags, arch_ldflags, cc, cxx, ar, arflags,
                      ranlib, strip, windres)
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
