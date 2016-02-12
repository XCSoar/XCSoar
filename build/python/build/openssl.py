import subprocess

from build.project import Project

class OpenSSLProject(Project):
    def __init__(self, url, md5, installed,
                 **kwargs):
        Project.__init__(self, url, md5, installed, **kwargs)

    def build(self, toolchain):
        src = self.unpack(toolchain, out_of_tree=False)

        subprocess.check_call(['/bin/sh', './Configure',
                               'dist', 'no-dso', 'no-krb5',
                               '--prefix=' + toolchain.install_prefix],
                              cwd=src, env=toolchain.env)
        subprocess.check_call(['/usr/bin/make', '--quiet', '-j12',
                               'CC=' + toolchain.cc,
                               'AR=' + toolchain.ar + ' r',
                               'RANLIB=arm-linux-gnueabihf-ranlib'],
                              cwd=src, env=toolchain.env)
        subprocess.check_call(['/usr/bin/make', '--quiet', 'install_sw',
                               'CC=' + toolchain.cc,
                               'AR=' + toolchain.ar + ' r',
                               'RANLIB=arm-linux-gnueabihf-ranlib'],
                              cwd=src, env=toolchain.env)
