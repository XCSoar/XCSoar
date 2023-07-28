import os
from typing import Optional

from .makeproject import MakeProject
from build.autotools import AutotoolsProject
from .toolchain import AnyToolchain

class BinutilsProject(AutotoolsProject):
    def _build(self, toolchain: AnyToolchain, target_toolchain: Optional[AnyToolchain]=None) -> None:
        AutotoolsProject._build(self, toolchain.native, target_toolchain=toolchain)

class GccBootstrapProject(AutotoolsProject):
    def _build(self, toolchain: AnyToolchain, target_toolchain: Optional[AnyToolchain]=None) -> None:
        build = self.configure(toolchain=toolchain.native,
                               target_toolchain=toolchain)
        self.make(toolchain, build, self.get_make_args(toolchain) + ['all-gcc'])
        self.make(toolchain, build, ['configure-target-libgcc'])
        self.make(toolchain, build, self.get_make_install_args(toolchain))

class GccProject(AutotoolsProject):
    def _build(self, toolchain: AnyToolchain, target_toolchain: Optional[AnyToolchain]=None) -> None:
        build = self.make_build_path(toolchain.native, lazy=True)
        if not os.path.isfile(os.path.join(build, 'Makefile')):
            build = self.configure(toolchain=toolchain.native,
                                   target_toolchain=toolchain,
                                   build=build)
        self.build_make(toolchain.native, build)
