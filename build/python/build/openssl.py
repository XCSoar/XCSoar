import subprocess

from build.makeproject import MakeProject

class OpenSSLProject(MakeProject):
    def __init__(self, url, alternative_url, md5, installed,
                 **kwargs):
        MakeProject.__init__(self, url, alternative_url, md5, installed, install_target='install_sw', **kwargs)

    def get_simultaneous_jobs(self):
        return 1

    def get_make_args(self, toolchain):
        return MakeProject.get_make_args(self, toolchain) + [
            'CC=' + toolchain.cc,
            'AR=' + toolchain.ar + ' r',
            'RANLIB=arm-linux-gnueabihf-ranlib'
        ]

    def build(self, toolchain):
        src = self.unpack(toolchain, out_of_tree=False)

        subprocess.check_call(['/bin/sh', './Configure',
                               'dist', 'no-dso', 'no-krb5',
                               '--prefix=' + toolchain.install_prefix],
                              cwd=src, env=toolchain.env)
        MakeProject.build(self, toolchain, wd)
