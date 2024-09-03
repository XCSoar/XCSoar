import sys
import os
import platform
from typing import Union

class NativeToolchain:
    """A toolchain for building native binaries, e.g. to be run on the
    build host."""

    def __init__(self, lib_path: str, other: 'Toolchain'):
        self.native = self

        self.tarball_path = other.tarball_path
        self.src_path = other.src_path
        self.build_path = other.build_path
        self.install_prefix = lib_path
        self.host_triplet = None

        machine = platform.machine()
        self.is_arm = machine.startswith('arm')
        self.is_armv7 = machine.startswith('armv7')
        self.is_aarch64 = machine.startswith('aarch64')
        self.is_windows = False
        self.is_android = False
        self.is_darwin = sys.platform == 'darwin'

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
    def __init__(self, top_path: str, lib_path: str,
                 tarball_path: str, src_path: str, build_path: str, install_prefix: str,
                 host_triplet: str, target_is_ios: bool,
                 arch_cflags: str, cppflags: str, arch_ldflags: str, 
                 cc: str, cxx: str, ar: str, arflags: str,
                 ranlib: str, strip: str, windres: str):
        self.tarball_path = tarball_path
        self.src_path = src_path
        self.build_path = build_path
        self.install_prefix = install_prefix
        self.host_triplet = host_triplet

        self.is_arm = host_triplet.startswith('arm')
        self.is_armv7 = host_triplet.startswith('armv7')
        self.is_aarch64 = host_triplet.startswith('aarch64')
        self.is_windows = 'mingw32' in host_triplet
        self.is_android = '-android' in host_triplet
        self.is_darwin = '-darwin' in host_triplet
        
        self.is_target_ios = target_is_ios

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
        self.pkg_config = shutil.copy(os.path.join(top_path, 'build', 'pkg-config.sh'),
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

        self.native = NativeToolchain(lib_path, self)

AnyToolchain = Union[Toolchain, NativeToolchain]
