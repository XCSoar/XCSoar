from build.autotools import AutotoolsProject

class CurlProject(AutotoolsProject):
    def configure(self, toolchain, *args, **kwargs):
        # disable usage of pthreads for Win32 builds
        if 'mingw' in toolchain.actual_arch:
            self.configure_args.append('--enable-pthreads=no')

        return AutotoolsProject.configure(self, toolchain, *args, **kwargs)
