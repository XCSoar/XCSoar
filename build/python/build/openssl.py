import subprocess
import os

from build.makeproject import MakeProject

class OpenSSLProject(MakeProject):
    def __init__(self, toolchain, url, alternative_url, md5, installed,
                 **kwargs):
        MakeProject.__init__(self, toolchain, url, alternative_url, md5, installed, install_target='install_sw', **kwargs)

    def get_simultaneous_jobs(self):
        return 1

    def get_make_args(self):
        return MakeProject.get_make_args(self) + [
            'CC=' + self.toolchain.cc,
            'AR=' + self.toolchain.ar + ' r',
            'RANLIB=' + self.toolchain.ranlib
        ]

    def build(self):
        subprocess.check_call(['/bin/sh', './Configure',
                               'dist', 'no-dso', 'no-krb5',
                               '--prefix=' + self.toolchain.install_prefix],
                              cwd=self.build_dir, env=self.toolchain.env)
        MakeProject.build(self, self.build_dir)
