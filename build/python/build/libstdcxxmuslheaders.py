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
        build = self.configure(toolchain)
        incdir_param = 'gxx_include_dir=' + toolchain.install_prefix + '/include/libstdc++'
        self.make(toolchain, os.path.join(build, 'libsupc++'), [incdir_param, 'install-bitsHEADERS', 'install-stdHEADERS'])
        self.make(toolchain, os.path.join(build, 'include'), [incdir_param, 'install-headers'])
