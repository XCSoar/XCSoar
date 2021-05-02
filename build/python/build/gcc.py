import os

from .makeproject import MakeProject
from build.autotools import AutotoolsProject

class BinutilsProject(AutotoolsProject):
    def build(self, toolchain):
        AutotoolsProject.build(self, toolchain.native, target_toolchain=toolchain)

class GccBootstrapProject(AutotoolsProject):
    def build(self, toolchain):
        build = self.configure(toolchain=toolchain.native,
                               target_toolchain=toolchain)
        self.make(toolchain, build, self.get_make_args(toolchain) + ['all-gcc'])
        self.make(toolchain, build, ['configure-target-libgcc'])
        self.make(toolchain, build, self.get_make_install_args(toolchain))

class GccProject(AutotoolsProject):
    def build(self, toolchain):
        build = self.make_build_path(toolchain.native, lazy=True)
        if not os.path.isfile(os.path.join(build, 'Makefile')):
            build = self.configure(toolchain=toolchain.native,
                                   target_toolchain=toolchain,
                                   build=build)
        MakeProject.build(self, toolchain.native, build)
