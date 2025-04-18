import os.path, subprocess, sys
from typing import Collection, Iterable, Optional, Sequence, Union
from collections.abc import Mapping

from build.makeproject import MakeProject
from .toolchain import AnyToolchain

class AutotoolsProject(MakeProject):
    def __init__(self, url: Union[str, Sequence[str]], md5: str, installed: str,
                 configure_args: Iterable[str]=[],
                 autogen: bool=False,
                 autoreconf: bool=False,
                 per_arch_cflags: Optional[Mapping[str, str]]=None,
                 cppflags: str='',
                 ldflags: str='',
                 libs: str='',
                 install_prefix: Optional[str]=None,
                 use_destdir: bool=False,
                 subdirs: Optional[Collection[str]]=None,
                 **kwargs):
        MakeProject.__init__(self, url, md5, installed, **kwargs)
        self.configure_args = configure_args
        self.autogen = autogen
        self.autoreconf = autoreconf
        self.per_arch_cflags = per_arch_cflags
        self.cppflags = cppflags
        self.ldflags = ldflags
        self.libs = libs
        self.install_prefix = install_prefix
        self.use_destdir = use_destdir
        self.subdirs = subdirs

    def configure(self, toolchain: AnyToolchain, src: Optional[str]=None, build: Optional[str]=None, target_toolchain: Optional[AnyToolchain]=None) -> str:
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
        if self.autoreconf:
            subprocess.check_call(['autoreconf', '-vif'], cwd=src)

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

        arch_cflags = ''
        if self.per_arch_cflags is not None and toolchain.host_triplet is not None:
            arch_cflags = self.per_arch_cflags.get(toolchain.host_triplet, '')

        configure = [
            os.path.join(src, 'configure'),
            'CC=' + toolchain.cc,
            'CXX=' + toolchain.cxx,
            'CFLAGS=' + toolchain.cflags + ' ' + arch_cflags,
            'CXXFLAGS=' + toolchain.cxxflags + ' ' + arch_cflags,
            'CPPFLAGS=' + cppflags + ' ' + self.cppflags,
            'LDFLAGS=' + toolchain.ldflags + ' ' + self.ldflags,
            'LIBS=' + toolchain.libs + ' ' + self.libs,
            'AR=' + toolchain.ar,
            'ARFLAGS=' + toolchain.arflags,
            'RANLIB=' + toolchain.ranlib,
            'STRIP=' + toolchain.strip,
            '--prefix=' + install_prefix,
            '--disable-silent-rules',
        ]

        if toolchain.host_triplet is not None:
            configure.append('--host=' + toolchain.host_triplet)

        if target_toolchain is not None:
            if target_toolchain.host_triplet is not None:
                configure.append('--target=' + target_toolchain.host_triplet)

        configure.extend(self.configure_args)

        try:
            print(configure)
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

    def get_make_args(self, toolchain: AnyToolchain) -> list[str]:
        return MakeProject.get_make_args(self, toolchain)

    def get_make_install_args(self, toolchain: AnyToolchain) -> list[str]:
        args = MakeProject.get_make_install_args(self, toolchain)
        if self.use_destdir:
            args += ['DESTDIR=' + toolchain.install_prefix]
        return args

    def _build(self, toolchain: AnyToolchain, target_toolchain: Optional[AnyToolchain]=None) -> None:
        build = self.configure(toolchain, target_toolchain=target_toolchain)
        if self.subdirs is not None:
            for subdir in self.subdirs:
                self.build_make(toolchain, os.path.join(build, subdir))
        else:
            self.build_make(toolchain, build)
