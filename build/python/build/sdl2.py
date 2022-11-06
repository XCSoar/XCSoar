import os.path, fileinput, re, shutil, subprocess

from build.autotools import AutotoolsProject

class SDL2Project(AutotoolsProject):
    def configure(self, toolchain, *args, **kwargs):
        if re.match('(arm.*|aarch64)-apple-darwin', toolchain.actual_arch) is not None:
            # for building SDL2 with autotools, several workarounds are
            # required:
            # * out-of-tree build is not supported
            # * needs CFLAGS adjustment to enable Objective-C and ARC

            src = self.unpack(toolchain, out_of_tree=False)

            # the SDL2 configure script expects "*-ios-*" and not "*-apple-*"
            host = toolchain.toolchain_arch.replace('-apple-', '-ios-')

            configure = [
                os.path.join(src, 'configure'),
                'CC=' + toolchain.cc,
                'CFLAGS=' + toolchain.cflags + ' -x objective-c -fobjc-arc',
                'CPPFLAGS=' + toolchain.cppflags + ' ' + self.cppflags,
                'LDFLAGS=' + toolchain.ldflags,
                'LIBS=' + toolchain.libs,
                'AR=' + toolchain.ar,
                'STRIP=' + toolchain.strip,
                '--host=' + host,
                '--prefix=' + toolchain.install_prefix,
                '--disable-silent-rules',
            ] + self.configure_args

            print(configure)
            subprocess.check_call(configure, cwd=src, env=toolchain.env)

            return src

        else:
            return AutotoolsProject.configure(self, toolchain, *args, **kwargs)
