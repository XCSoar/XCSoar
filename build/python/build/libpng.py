from build.autotools import AutotoolsProject

class LibPNGProject(AutotoolsProject):
    def configure(self):
        # append argument --enable-arm-neon for targets supporting NEON
        if self.toolchain.actual_arch.startswith('aarch64') or '-mfpu=neon' in self.toolchain.cflags:
            self.configure_args.append('--enable-arm-neon')

        return AutotoolsProject.configure(self)
