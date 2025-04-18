from .autotools import AutotoolsProject
from .toolchain import AnyToolchain

class MuslProject(AutotoolsProject):
    def configure(self, toolchain: AnyToolchain, *args, **kwargs) -> str:
        # Musl's configure script ignores the commonly used "AR"
        # etc. variables, but instead uses CROSS_COMPILE to build the
        # paths to all build tools.  This kludge takes strips "ar"
        # from the "ar" path and uses it to build the CROSS_COMPILE
        # prefix.
        cross_compile = toolchain.ar.removesuffix('ar')
        if cross_compile:
            self.configure_args.append('CROSS_COMPILE=' + cross_compile)

        return AutotoolsProject.configure(self, toolchain, *args, **kwargs)
