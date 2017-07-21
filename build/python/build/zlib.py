import subprocess

from build.project import Project

class ZlibProject(Project):
    def __init__(self, url, alternative_url, md5, installed,
                 **kwargs):
        Project.__init__(self, url, alternative_url, md5, installed, **kwargs)

    def build(self, toolchain):
        src = self.unpack(toolchain, out_of_tree=False)

        subprocess.check_call(['./configure', '--prefix=' + toolchain.install_prefix, '--static'],
                              cwd=src, env=toolchain.env)
        subprocess.check_call(['/usr/bin/make', '--quiet',
                               'CC=' + toolchain.cc + ' ' + toolchain.cppflags + ' ' + toolchain.cflags,
                               'CPP=' + toolchain.cc + ' -E ' + toolchain.cppflags,
                               'AR=' + toolchain.ar,
                               'ARFLAGS=' + toolchain.arflags,
                               'RANLIB=' + toolchain.ranlib,
                               'LDSHARED=' + toolchain.cc + ' -shared',
                               'install'],
                              cwd=src, env=toolchain.env)
