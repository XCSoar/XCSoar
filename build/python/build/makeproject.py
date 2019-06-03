import subprocess, multiprocessing

from build.project import Project

class MakeProject(Project):
    def __init__(self, toolchain, url, alternative_url, md5, installed,
                 install_target='install',
                 **kwargs):
        Project.__init__(self, toolchain, url, alternative_url, md5, installed, **kwargs)
        self.install_target = install_target

    def get_simultaneous_jobs(self):
        try:
            # use twice as many simultaneous jobs as we have CPU cores
            return multiprocessing.cpu_count() * 2
        except NotImplementedError:
            # default to 12, if multiprocessing.cpu_count() is not implemented
            return 12

    def get_make_args(self):
        return ['--quiet', '-j' + str(self.get_simultaneous_jobs())]

    def get_make_install_args(self):
        return ['--quiet', self.install_target]

    def make(self, wd, args):
        subprocess.check_call(['make'] + args,
                              cwd=wd, env=self.toolchain.env)

    def build(self, wd, install=True):
        self.make(wd, self.get_make_args())
        if install:
            self.make(wd, self.get_make_install_args())
