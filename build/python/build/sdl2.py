import os.path, fileinput, re, shutil, subprocess

from build.autotools import AutotoolsProject

class SDL2Project(AutotoolsProject):
    def configure(self, toolchain):
        if re.match('(arm.*|aarch64)-apple-darwin', toolchain.arch) is not None:
            # for building SDL2 with autotools, several workarounds are
            # required:
            # * out-of-tree build is not supported
            # * needs CFLAGS adjustment to enable Objective-C and ARC
            # * SDL_config.h needs to be replaced with SDL_config_iphoneos.h
            #   after running "configure"

            src = self.unpack(toolchain, out_of_tree=False)

            configure = [
                os.path.join(src, 'configure'),
                'CC=' + toolchain.cc,
                'CFLAGS=' + toolchain.cflags + ' -x objective-c -fobjc-arc',
                'CPPFLAGS=' + toolchain.cppflags + ' ' + self.cppflags,
                'LDFLAGS=' + toolchain.ldflags,
                'LIBS=' + toolchain.libs,
                'AR=' + toolchain.ar,
                'STRIP=' + toolchain.strip,
                '--host=' + toolchain.arch,
                '--prefix=' + toolchain.install_prefix,
            ] + self.configure_args

            subprocess.check_call(configure, cwd=src, env=toolchain.env)

            shutil.copyfile(os.path.join(src, 'include/SDL_config_iphoneos.h'),
                            os.path.join(src, 'include/SDL_config.h'))

            return src

        else:
            return AutotoolsProject.configure(self, toolchain)
