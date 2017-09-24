from build.autotools import AutotoolsProject

class CurlProject(AutotoolsProject):
    def configure(self, toolchain):
        # append argument --enable-arm-neon for targets supporting NEON
        if 'mingw' in toolchain.arch:
            self.configure_args.append('--enable-pthreads=no')

        return AutotoolsProject.configure(self, toolchain)
