import subprocess

from build.project import Project

class ZlibProject(Project):
    def __init__(self, url, md5, installed,
                 **kwargs):
        Project.__init__(self, url, md5, installed, **kwargs)

    def build(self, toolchain):
        src = self.unpack(toolchain, out_of_tree=False)

        subprocess.check_call(['./configure', '--prefix=' + toolchain.install_prefix, '--static'],
                              cwd=src, env=toolchain.env)
        subprocess.check_call(['/usr/bin/make', '--quiet',
                               'CC=' + toolchain.cc,
                               'CPP=' + toolchain.cc + ' -E',
                               'AR=' + toolchain.ar,
                               'LDSHARED=' + toolchain.cc + ' -shared',
                               'install'],
                              cwd=src, env=toolchain.env)
