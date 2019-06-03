import os.path, fileinput, re, shutil, subprocess

from build.autotools import AutotoolsProject

class SDL2Project(AutotoolsProject):
    def configure(self):
        if re.match('(arm.*|aarch64)-apple-darwin', self.toolchain.actual_arch) is not None:
            # for building SDL2 with autotools, several workarounds are
            # required:
            # * out-of-tree build is not supported
            # * needs CFLAGS adjustment to enable Objective-C and ARC
            # * SDL_config.h needs to be replaced with SDL_config_iphoneos.h
            #   after running "configure"

            src = self.unpack(out_of_tree=False)

            configure = [
                os.path.join(src, 'configure'),
                'CC=' + self.toolchain.cc,
                'CFLAGS=' + self.toolchain.cflags + ' -x objective-c -fobjc-arc',
                'CPPFLAGS=' + self.toolchain.cppflags + ' ' + self.cppflags,
                'LDFLAGS=' + self.toolchain.ldflags,
                'LIBS=' + self.toolchain.libs,
                'AR=' + self.toolchain.ar,
                'STRIP=' + self.toolchain.strip,
                '--host=' + self.toolchain.toolchain_arch,
                '--prefix=' + self.toolchain.install_prefix,
            ] + self.configure_args

            subprocess.check_call(configure, cwd=src, env=self.toolchain.env)

            shutil.copyfile(os.path.join(src, 'include/SDL_config_iphoneos.h'),
                            os.path.join(src, 'include/SDL_config.h'))

            return src

        else:
            return AutotoolsProject.configure(self)
