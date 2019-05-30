from build.autotools import AutotoolsProject

class NetcdfCxxProject(AutotoolsProject):
    def get_make_args(self, toolchain):
        # HELP TODO: Temp workaround. Will ANDROID_NDK alwats be set in env if in the default location?
        return AutotoolsProject.get_make_args(self, toolchain)  + ['CPPFLAGS=' + toolchain.cppflags + ' -I' + toolchain.env['ANDROID_NDK'] + '/sources/cxx-stl/llvm-libc++/include']
