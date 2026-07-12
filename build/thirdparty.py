#!/usr/bin/env -S python3 -u

import os, os.path
import re
import sys

if len(sys.argv) < 14 or len(sys.argv) > 15:
    print("Usage: build.py LIB_PATH HOST_TRIPLET TARGET_IS_IOS ARCH_CFLAGS CPPFLAGS ARCH_LDFLAGS CC CXX AR ARFLAGS RANLIB STRIP WINDRES [ENABLE_SDL]", file=sys.stderr)
    sys.exit(1)

lib_path, host_triplet, target_is_ios, arch_cflags, cppflags, arch_ldflags, cc, cxx, ar, arflags, ranlib, strip, windres = sys.argv[1:14]
enable_sdl = sys.argv[14] if len(sys.argv) > 14 else ''
target_is_ios = (target_is_ios == 'y') # convert to boolean
enable_sdl = (enable_sdl == 'y') # convert to boolean

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

from build.toolchain import Toolchain, NativeToolchain
toolchain = Toolchain(xcsoar_path, lib_path,
                      tarball_path, src_path, build_path, install_prefix,
                      host_triplet, target_is_ios,
                      arch_cflags, cppflags, arch_ldflags, cc, cxx, ar, arflags,
                      ranlib, strip, windres)

# a list of third-party libraries to be used by XCSoar
from build.libs import *

geotiff_enabled = os.environ.get('GEOTIFF', 'n') == 'y'
use_angle_env = os.environ.get('USE_ANGLE', 'auto')
if use_angle_env == 'auto':
    use_angle = toolchain.is_darwin and not toolchain.is_target_ios
else:
    use_angle = (use_angle_env == 'y')

thirdparty_projects = {
    'binutils': binutils,
    'linux-headers': linux_headers,
    'gcc-bootstrap': gcc_bootstrap,
    'musl': musl,
    'gcc': gcc,
    'zlib': zlib,
    'fmt': libfmt,
    'libsodium': libsodium,
    'freetype': freetype,
    'openssl': openssl,
    'c-ares': cares,
    'curl': curl,
    'libpng': libpng,
    'libjpeg': libjpeg,
    'lua': lua,
    'sqlite': sqlite3,
    'proj': proj,
    'libtiff': libtiff,
    'libgeotiff': libgeotiff,
    'libsalsa': libsalsa,
    'libusb': libusb,
    'simple-usbmodeswitch': simple_usbmodeswitch,
    'sdl2': sdl2,
    'angle': angle,
}

if toolchain.is_windows:
    thirdparty_libs = [
        zlib,
        libfmt,
        libsodium,
        cares,
        curl,
        lua,
    ]
    if use_angle:
        thirdparty_libs += [
            angle,
        ]

    # Add SDL2, sqlite3, and their dependencies for OpenGL/ANGLE builds
    if enable_sdl:
        thirdparty_libs.extend([
            sqlite3,
            freetype,
            libpng,
            libjpeg,
            sdl2,
        ])

    # Some libraries (such as CURL) want to use the min()/max() macros
    toolchain.cppflags = cppflags.replace('-DNOMINMAX', '')

    # Explicitly disable _FORTIFY_SOURCE because it is broken with
    # mingw.  This prevents some libraries such as libsodium to enable
    # it.
    toolchain.cppflags += ' -D_FORTIFY_SOURCE=0'
elif toolchain.is_darwin:
    thirdparty_libs = [
        zlib,
        libfmt,
        libsodium,
        cares,
        curl,
        lua,
    ]
    if use_angle:
        thirdparty_libs += [
            angle,
        ]
    thirdparty_libs += [
        sdl2
    ]
    if geotiff_enabled:
        thirdparty_libs += [
            sqlite3,
            proj,
            libtiff,
            libgeotiff,
        ]
elif toolchain.is_android:
    thirdparty_libs = [
        zlib,
        libfmt,
        libsodium,
        openssl,
        cares,
        curl,
        libpng,
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

package_selection = os.environ.get('THIRDPARTY_PACKAGES', 'auto')
if package_selection != 'auto':
    selected_names = {
        name.strip()
        for name in package_selection.split(',')
        if name.strip()
    }
    unknown_names = selected_names - thirdparty_projects.keys()
    if unknown_names:
        raise RuntimeError(
            'Unknown third-party package(s): ' + ', '.join(sorted(unknown_names))
        )

    available_projects = set(thirdparty_libs)
    unsupported_names = {
        name for name in selected_names
        if thirdparty_projects[name] not in available_projects
    }
    if unsupported_names:
        raise RuntimeError(
            'Third-party package(s) unavailable for this target: ' +
            ', '.join(sorted(unsupported_names))
        )

    selected_projects = {thirdparty_projects[name] for name in selected_names}
    thirdparty_libs = [
        project for project in thirdparty_libs if project in selected_projects
    ]

# build the third-party libraries
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
