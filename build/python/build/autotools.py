import os.path, subprocess, sys

from build.makeproject import MakeProject

class AutotoolsProject(MakeProject):
    def __init__(self, url, alternative_url, md5, installed, configure_args=[],
                 autogen=False,
                 cppflags='',
                 ldflags='',
                 libs='',
                 shared=False,
                 install_prefix=None,
                 install_target='install',
                 use_destdir=False,
                 make_args=[],
                 **kwargs):
        MakeProject.__init__(self, url, alternative_url, md5, installed, **kwargs)
        self.configure_args = configure_args
        self.autogen = autogen
        self.cppflags = cppflags
        self.ldflags = ldflags
        self.libs = libs
        self.shared = shared
        self.install_prefix = install_prefix
        self.use_destdir = use_destdir
        self.make_args = make_args

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

        cppflags = toolchain.cppflags
        if self.name == 'glibc':
            # glibc's Makefile goes into an endless re-execution loop
            # if it is already installed; this kludge works around
            # this problem by removing all "-isystem" directives (some
            # of which probably point to the installation directory)
            import re
            cppflags = re.sub(r'\s*-isystem\s+\S+\s*', ' ', cppflags)

        install_prefix = self.install_prefix
        if install_prefix is None:
            install_prefix = toolchain.install_prefix

        configure = [
            os.path.join(src, 'configure'),
            'CC=' + toolchain.cc,
            'CXX=' + toolchain.cxx,
            'CFLAGS=' + self._filter_cflags(toolchain.cflags),
            'CXXFLAGS=' + self._filter_cflags(toolchain.cxxflags),
            'CPPFLAGS=' + cppflags + ' ' + self.cppflags,
            'LDFLAGS=' + toolchain.ldflags + ' ' + self.ldflags,
            'LIBS=' + toolchain.libs + ' ' + self.libs,
            'AR=' + toolchain.ar,
            'ARFLAGS=' + toolchain.arflags,
            'RANLIB=' + toolchain.ranlib,
            'STRIP=' + toolchain.strip,
            '--host=' + toolchain.arch,
            '--prefix=' + install_prefix,
            '--enable-silent-rules',
        ] + self.configure_args

        subprocess.check_call(configure, cwd=build, env=toolchain.env)
        return build

    def get_make_args(self, toolchain):
        return MakeProject.get_make_args(self, toolchain) + self.make_args

    def get_make_install_args(self, toolchain):
        args = MakeProject.get_make_install_args(self, toolchain)
        if self.use_destdir:
            args += ['DESTDIR=' + toolchain.install_prefix]
        return args

    def build(self, toolchain):
        build = self.configure(toolchain)
        MakeProject.build(self, toolchain, build)
