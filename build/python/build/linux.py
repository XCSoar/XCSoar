from typing import Optional

from .makeproject import MakeProject
from .toolchain import AnyToolchain

class SabotageLinuxHeadersProject(MakeProject):
    def get_make_install_args(self, toolchain: AnyToolchain) -> list[str]:
        return MakeProject.get_make_install_args(self, toolchain) + [
            'ARCH=arm', # TODO: don't hard-code Kobo settings
            'prefix=' + toolchain.install_prefix,
        ]

    def _build(self, toolchain: AnyToolchain, target_toolchain: Optional[AnyToolchain]=None) -> None:
        src = self.unpack(toolchain)
        self.make(toolchain, src, self.get_make_install_args(toolchain))
