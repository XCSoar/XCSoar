
import os


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

    # create the base directories where the tarballs are being downloaded, where shared sources are being unpacked,
    # and where the libs are being built.
    def make_paths(self):
        os.makedirs(self.tarball_path, exist_ok=True)
        os.makedirs(self.src_path, exist_ok=True)
        os.makedirs(self.build_path, exist_ok=True)
