import os.path, subprocess, sys

from build.project import Project

class AutotoolsProject(Project):
    def __init__(self, url, md5, installed, configure_args=[],
                 autogen=False,
                 cppflags='',
                 ldflags='',
                 libs='',
                 shared=False,
                 install_prefix=None,
                 install_target='install',
                 use_destdir=False,
                 **kwargs):
        Project.__init__(self, url, md5, installed, **kwargs)
        self.configure_args = configure_args
        self.autogen = autogen
        self.cppflags = cppflags
        self.ldflags = ldflags
        self.libs = libs
        self.shared = shared
        self.install_prefix = install_prefix
        self.install_target = install_target
        self.use_destdir = use_destdir

    def _filter_cflags(self, flags):
        if self.shared:
            # filter out certain flags which are only useful with
            # static linking
            for f in ('-fvisibility=hidden', '-fdata-sections', '-ffunction-sections'):
                flags = flags.replace(' ' + f + ' ', ' ')
        return flags

    def configure(self, toolchain):
        src = self.unpack(toolchain)
        if self.autogen:
            if sys.platform == 'darwin':
                subprocess.check_call(['glibtoolize', '--force'], cwd=src)
            else:
                subprocess.check_call(['libtoolize', '--force'], cwd=src)
            subprocess.check_call(['aclocal'], cwd=src)
            subprocess.check_call(['automake', '--add-missing', '--force-missing', '--foreign'], cwd=src)
            subprocess.check_call(['autoconf'], cwd=src)

        build = self.make_build_path(toolchain)

        install_prefix = self.install_prefix
        if install_prefix is None:
            install_prefix = toolchain.install_prefix

        configure = [
            os.path.join(src, 'configure'),
            'CC=' + toolchain.cc,
            'CXX=' + toolchain.cxx,
            'CFLAGS=' + self._filter_cflags(toolchain.cflags),
            'CXXFLAGS=' + self._filter_cflags(toolchain.cxxflags),
            'CPPFLAGS=' + toolchain.cppflags + ' ' + self.cppflags,
            'LDFLAGS=' + toolchain.ldflags + ' ' + self.ldflags,
            'LIBS=' + toolchain.libs + ' ' + self.libs,
            'AR=' + toolchain.ar,
            'RANLIB=' + toolchain.ranlib,
            'STRIP=' + toolchain.strip,
            '--host=' + toolchain.arch,
            '--prefix=' + install_prefix,
            '--enable-silent-rules',
        ] + self.configure_args

        subprocess.check_call(configure, cwd=build, env=toolchain.env)
        return build

    def build(self, toolchain):
        build = self.configure(toolchain)

        destdir = []
        if self.use_destdir:
            destdir = ['DESTDIR=' + toolchain.install_prefix]

        subprocess.check_call(['/usr/bin/make', '--quiet', '-j12'],
                              cwd=build, env=toolchain.env)
        subprocess.check_call(['/usr/bin/make', '--quiet', self.install_target] + destdir,
                              cwd=build, env=toolchain.env)
