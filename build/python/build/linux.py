from .makeproject import MakeProject

class SabotageLinuxHeadersProject(MakeProject):
    def get_make_install_args(self, toolchain):
        return MakeProject.get_make_install_args(self, toolchain) + [
            'ARCH=arm', # TODO: don't hard-code Kobo settings
            'prefix=' + toolchain.install_prefix,
        ]

    def build(self, toolchain):
        src = self.unpack(toolchain)
        self.make(toolchain, src, self.get_make_install_args(toolchain))
