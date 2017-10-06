import subprocess

from build.makeproject import MakeProject

class ZlibProject(MakeProject):
    def __init__(self, url, alternative_url, md5, installed,
                 **kwargs):
        MakeProject.__init__(self, url, alternative_url, md5, installed, **kwargs)

    def get_make_args(self, toolchain):
        return MakeProject.get_make_args(self, toolchain) + [
            'CC=' + toolchain.cc + ' ' + toolchain.cppflags + ' ' + toolchain.cflags,
            'CPP=' + toolchain.cc + ' -E ' + toolchain.cppflags,
            'AR=' + toolchain.ar,
            'ARFLAGS=' + toolchain.arflags,
            'RANLIB=' + toolchain.ranlib,
            'LDSHARED=' + toolchain.cc + ' -shared',
            'libz.a'
        ]

    def build(self, toolchain):
        src = self.unpack(toolchain, out_of_tree=False)

        subprocess.check_call(['./configure', '--prefix=' + toolchain.install_prefix, '--static'],
                              cwd=src, env=toolchain.env)
        MakeProject.build(self, toolchain, src)
