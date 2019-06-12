import subprocess

from build.makeproject import MakeProject

class ZlibProject(MakeProject):
    def __init__(self, toolchain, url, alternative_url, md5, installed,
                 **kwargs):
        MakeProject.__init__(self, toolchain, url, alternative_url, md5, installed, **kwargs)

    def get_make_args(self):
        return MakeProject.get_make_args(self) + [
            'CC=' + self.toolchain.cc + ' ' + self.toolchain.cppflags + ' ' + self.toolchain.cflags,
            'CPP=' + self.toolchain.cc + ' -E ' + self.toolchain.cppflags,
            'AR=' + self.toolchain.ar,
            'ARFLAGS=' + self.toolchain.arflags,
            'RANLIB=' + self.toolchain.ranlib,
            'LDSHARED=' + self.toolchain.cc + ' -shared',
            'libz.a'
        ]

    def get_make_install_args(self):
        return [
            'RANLIB=' + self.toolchain.ranlib,
            self.install_target
        ]

    def build(self):
        subprocess.check_call(['./configure', '--prefix=' + self.toolchain.install_prefix, '--static'],
                              cwd=self.src_dir, env=self.toolchain.env)
        MakeProject.build(self, self.src_dir)
