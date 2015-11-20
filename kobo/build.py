#!/usr/bin/env python3

import os, os.path
import sys, shutil, subprocess
import urllib.request
import hashlib
import re

if len(sys.argv) != 7:
    print("Usage: build.py TARGET_OUTPUT_DIR HOST_ARCH CC CXX AR STRIP", file=sys.stderr)
    sys.exit(1)

target_output_dir, host_arch, cc, cxx, ar, strip = sys.argv[1:]

# the path to the XCSoar sources
xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')

# output directories
output_path = os.path.abspath('output')
tarball_path = os.path.join(output_path, 'download')
src_path = os.path.join(output_path, 'src')
target_output_dir = os.path.abspath(target_output_dir)

lib_path = os.path.join(target_output_dir, 'lib')
arch_path = os.path.join(lib_path, host_arch)
build_path = os.path.join(arch_path, 'build')
install_prefix = os.path.join(arch_path, 'root')

if 'MAKEFLAGS' in os.environ:
    # build/make.mk adds "--no-builtin-rules --no-builtin-variables",
    # which breaks the zlib Makefile (and maybe others)
    del os.environ['MAKEFLAGS']

class KoboToolchain:
    def __init__(self, install_prefix, arch, cc, cxx, ar, strip):
        self.arch = arch
        self.install_prefix = install_prefix

        self.cc = cc
        self.cxx = cxx
        self.ar = ar
        self.strip = strip

        arch_flags = '-march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard'
        common_flags = '-Os -g -ffunction-sections -fdata-sections -fvisibility=hidden ' + arch_flags
        self.cflags = common_flags
        self.cxxflags = common_flags
        self.cppflags = '-isystem ' + os.path.join(install_prefix, 'include') + ' -DNDEBUG'
        self.ldflags = '-L' + os.path.join(install_prefix, 'lib')
        self.libs = ''

        self.env = dict(os.environ)

        # redirect pkg-config to use our root directory instead of the
        # default one on the build host
        self.env['PKG_CONFIG_LIBDIR'] = os.path.join(install_prefix, 'lib/pkgconfig')

from build.download import download_and_verify
from build.tar import untar

class Project:
    def __init__(self, url, md5, installed, name=None, version=None,
                 base=None,
                 use_cxx=False, use_clang=False):
        if base is None:
            basename = os.path.basename(url)
            m = re.match(r'^(.+)\.(tar(\.(gz|bz2|xz|lzma))?|zip)$', basename)
            if not m: raise
            self.base = m.group(1)
        else:
            self.base = base

        if name is None or version is None:
            m = re.match(r'^([-\w]+)-(\d[\d.]*[a-z]?)$', self.base)
            if name is None: name = m.group(1)
            if version is None: version = m.group(2)

        self.name = name
        self.version = version

        self.url = url
        self.md5 = md5
        self.installed = installed

        self.use_cxx = use_cxx
        self.use_clang = use_clang

    def download(self):
        return download_and_verify(self.url, self.md5, tarball_path)

    def is_installed(self, toolchain):
        tarball = self.download()
        installed = os.path.join(toolchain.install_prefix, self.installed)
        tarball_mtime = os.path.getmtime(tarball)
        try:
            return os.path.getmtime(installed) >= tarball_mtime
        except FileNotFoundError:
            return False

    def unpack(self):
        global src_path
        return untar(self.download(), src_path, self.base)

    def make_build_path(self):
        path = os.path.join(build_path, self.base)
        try:
            shutil.rmtree(path)
        except FileNotFoundError:
            pass
        os.makedirs(path, exist_ok=True)
        return path

class ZlibProject(Project):
    def __init__(self, url, md5, installed,
                 **kwargs):
        Project.__init__(self, url, md5, installed, **kwargs)

    def build(self, toolchain):
        src = self.unpack(out_of_tree=False)

        subprocess.check_call(['./configure', '--prefix=' + toolchain.install_prefix, '--static'],
                              cwd=src, env=toolchain.env)
        subprocess.check_call(['/usr/bin/make', '--quiet',
                               'CC=' + toolchain.cc,
                               'CPP=' + toolchain.cc + ' -E',
                               'AR=' + toolchain.ar,
                               'LDSHARED=' + toolchain.cc + ' -shared',
                               'install'],
                              cwd=src, env=toolchain.env)

class AutotoolsProject(Project):
    def __init__(self, url, md5, installed, configure_args=[],
                 autogen=False,
                 cppflags='',
                 **kwargs):
        Project.__init__(self, url, md5, installed, **kwargs)
        self.configure_args = configure_args
        self.autogen = autogen
        self.cppflags = cppflags

    def configure(self, toolchain):
        src = self.unpack()
        if self.autogen:
            subprocess.check_call(['/usr/bin/aclocal'], cwd=src)
            subprocess.check_call(['/usr/bin/automake', '--add-missing', '--force-missing', '--foreign'], cwd=src)
            subprocess.check_call(['/usr/bin/autoconf'], cwd=src)
            subprocess.check_call(['/usr/bin/libtoolize', '--force'], cwd=src)

        build = self.make_build_path()

        configure = [
            os.path.join(src, 'configure'),
            'CC=' + toolchain.cc,
            'CXX=' + toolchain.cxx,
            'CFLAGS=' + toolchain.cflags,
            'CXXFLAGS=' + toolchain.cxxflags,
            'CPPFLAGS=' + toolchain.cppflags + ' ' + self.cppflags,
            'LDFLAGS=' + toolchain.ldflags,
            'LIBS=' + toolchain.libs,
            'AR=' + toolchain.ar,
            'STRIP=' + toolchain.strip,
            '--host=' + toolchain.arch,
            '--prefix=' + toolchain.install_prefix,
            '--enable-silent-rules',
        ] + self.configure_args

        subprocess.check_call(configure, cwd=build, env=toolchain.env)
        return build

    def build(self, toolchain):
        build = self.configure(toolchain)

        subprocess.check_call(['/usr/bin/make', '--quiet', '-j12'],
                              cwd=build, env=toolchain.env)
        subprocess.check_call(['/usr/bin/make', '--quiet', 'install'],
                              cwd=build, env=toolchain.env)

class FreeTypeProject(AutotoolsProject):
    def configure(self, toolchain):
        build = AutotoolsProject.configure(self, toolchain)

        comment_re = re.compile(r'^\w+_MODULES\s*\+=\s*(?:type1|cff|cid|pfr|type42|winfonts|pcf|bdf|lzw|bzip2|psaux|psnames)\s*$')
        modules_cfg = os.path.join(build, 'modules.cfg')
        tmp = modules_cfg + '.tmp'
        with open(modules_cfg) as src:
            with open(tmp, 'w') as dest:
                for line in src:
                    if comment_re.match(line):
                        line = '# ' + line
                    dest.write(line)
        os.rename(tmp, modules_cfg)

        return build

# a list of third-party libraries to be used by XCSoar
thirdparty_libs = [
    ZlibProject(
        'http://zlib.net/zlib-1.2.8.tar.xz',
        '28f1205d8dd2001f26fec1e8c2cebe37',
        'lib/libz.a',
    ),

    FreeTypeProject(
        'http://download.savannah.gnu.org/releases/freetype/freetype-2.6.1.tar.bz2',
        '35cb8f4d9e5906847901bb39324c2f80',
        'lib/libfreetype.a',
        [
            '--disable-shared', '--enable-static',
            '--without-bzip2', '--without-png',
            '--without-harfbuzz',
        ],
    ),

    AutotoolsProject(
        'http://curl.haxx.se/download/curl-7.44.0.tar.lzma',
        '2f924c80bb7124dff1b39f54ffda3781',
        'lib/libcurl.a',
        [
            '--disable-shared', '--enable-static',
            '--disable-debug',
            '--enable-http',
            '--enable-ipv6',
            '--disable-ftp', '--disable-file',
            '--disable-ldap', '--disable-ldaps',
            '--disable-rtsp', '--disable-proxy', '--disable-dict', '--disable-telnet',
            '--disable-tftp', '--disable-pop3', '--disable-imap', '--disable-smb',
            '--disable-smtp',
            '--disable-gopher',
            '--disable-manual',
            '--disable-threaded-resolver', '--disable-verbose', '--disable-sspi',
            '--disable-crypto-auth', '--disable-ntlm-wb', '--disable-tls-srp', '--disable-cookies',
            '--without-ssl', '--without-gnutls', '--without-nss', '--without-libssh2',
        ],
        use_clang=True,
    ),

    AutotoolsProject(
        'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.19.tar.xz',
        '1e6a458429e850fc93c1f3b6dc00a48f',
        'lib/libpng.a',
        [
            '--disable-shared', '--enable-static',
            '--enable-arm-neon',
        ]
    ),

    AutotoolsProject(
        'http://downloads.sourceforge.net/project/libjpeg-turbo/1.4.2/libjpeg-turbo-1.4.2.tar.gz',
        '86b0d5f7507c2e6c21c00219162c3c44',
        'lib/libjpeg.a',
        [
            '--disable-shared', '--enable-static',
            '--enable-arm-neon',
        ]
    ),
]

# build the third-party libraries
toolchain = KoboToolchain(install_prefix, host_arch, cc, cxx, ar, strip)
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
