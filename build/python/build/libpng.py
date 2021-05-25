from build.autotools import AutotoolsProject

class LibPNGProject(AutotoolsProject):
    def configure(self, toolchain, *args, **kwargs):
        # append argument --enable-arm-neon for targets supporting NEON
        if toolchain.actual_arch.startswith('aarch64') or '-mfpu=neon' in toolchain.cflags:
            self.configure_args.append('--enable-arm-neon')

        return AutotoolsProject.configure(self, toolchain, *args, **kwargs)
