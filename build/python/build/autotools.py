import os.path, subprocess, sys

from build.makeproject import MakeProject

class AutotoolsProject(MakeProject):
    def __init__(self, url, alternative_url, md5, installed, configure_args=[],
                 autogen=False,
                 cppflags='',
                 ldflags='',
                 libs='',
                 install_prefix=None,
                 use_destdir=False,
                 use_actual_arch=False,
                 subdirs=None,
                 **kwargs):
        MakeProject.__init__(self, url, alternative_url, md5, installed, **kwargs)
        self.configure_args = configure_args
        self.autogen = autogen
        self.cppflags = cppflags
        self.ldflags = ldflags
        self.libs = libs
        self.install_prefix = install_prefix
        self.use_destdir = use_destdir
        self.use_actual_arch = use_actual_arch
        self.subdirs = subdirs

    def configure(self, toolchain, src=None, build=None, target_toolchain=None):
        if src is None:
            src = self.unpack(toolchain)

        if build is None:
            build = self.make_build_path(toolchain)

        if self.autogen:
            if sys.platform == 'darwin':
                subprocess.check_call(['glibtoolize', '--force'], cwd=src)
            else:
                subprocess.check_call(['libtoolize', '--force'], cwd=src)
            subprocess.check_call(['aclocal'], cwd=src)
            subprocess.check_call(['automake', '--add-missing', '--force-missing', '--foreign'], cwd=src)
            subprocess.check_call(['autoconf'], cwd=src)

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
            'CFLAGS=' + toolchain.cflags,
            'CXXFLAGS=' + toolchain.cxxflags,
            'CPPFLAGS=' + cppflags + ' ' + self.cppflags,
            'LDFLAGS=' + toolchain.ldflags + ' ' + self.ldflags,
            'LIBS=' + toolchain.libs + ' ' + self.libs,
            'AR=' + toolchain.ar,
            'ARFLAGS=' + toolchain.arflags,
            'RANLIB=' + toolchain.ranlib,
            'STRIP=' + toolchain.strip,
            '--prefix=' + install_prefix,
            '--enable-silent-rules',
        ]

        arch = toolchain.actual_arch if self.use_actual_arch else toolchain.toolchain_arch
        if arch is not None:
            configure.append('--host=' + arch)

        if target_toolchain is not None:
            arch = target_toolchain.actual_arch if self.use_actual_arch else target_toolchain.toolchain_arch
            if arch is not None:
                configure.append('--target=' + arch)

        configure += self.configure_args

        try:
            subprocess.check_call(configure, cwd=build, env=toolchain.env)
        except subprocess.CalledProcessError:
            # dump config.log after a failed configure run
            try:
                with open(os.path.join(build, 'config.log')) as f:
                    sys.stdout.write(f.read())
            except:
                pass
            # re-raise the exception
            raise

        return build

    def get_make_args(self, toolchain):
        return MakeProject.get_make_args(self, toolchain)

    def get_make_install_args(self, toolchain):
        args = MakeProject.get_make_install_args(self, toolchain)
        if self.use_destdir:
            args += ['DESTDIR=' + toolchain.install_prefix]
        return args

    def _build(self, toolchain, target_toolchain=None):
        build = self.configure(toolchain, target_toolchain=target_toolchain)
        if self.subdirs is not None:
            for subdir in self.subdirs:
                self.build_make(toolchain, os.path.join(build, subdir))
        else:
            self.build_make(toolchain, build)
