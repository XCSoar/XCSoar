import os
from build.autotools import AutotoolsProject
from build.makeproject import MakeProject

# Installer for musl compatible libstdc++ headers.
# The static libstdc++ (and libsupc++) libraries from GNU toolchains are
# musl comptatible, but the headers are not musl compatible.
# So we need to install a set of libstd++ headers, which are included in the gcc
# source tarball.
class LibstdcxxMuslHeadersProject(AutotoolsProject):
    def build(self, toolchain):
        src = self.unpack(toolchain)
        top_build = self.make_build_path(toolchain)
        build = os.path.join(top_build, 'libstdc++-v3')
        os.mkdir(build)

        # libstdc++'s configure requires a configured libgcc or it
        # will disable threading support; but instead of configuring
        # libgcc, this kludge just creates the "gthr-default.h"
        # symlink, which is enough to enable multi-threading
        libgcc = os.path.join(top_build, 'libgcc')
        os.mkdir(libgcc)
        os.symlink(os.path.join(src, 'libgcc', 'gthr-posix.h'),
                   os.path.join(libgcc, 'gthr-default.h'))

        build = self.configure(toolchain, src=src, build=build)

        incdir_param = 'gxx_include_dir=' + toolchain.install_prefix + '/include/libstdc++'
        self.make(toolchain, os.path.join(build, 'libsupc++'), [incdir_param, 'install-bitsHEADERS', 'install-stdHEADERS'])
        self.make(toolchain, os.path.join(build, 'include'), [incdir_param, 'install-headers'])
