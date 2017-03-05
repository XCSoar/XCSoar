#!/usr/bin/env python3

import os, os.path
import sys, shutil, subprocess
import re

if len(sys.argv) != 8:
    print("Usage: build.py TARGET_OUTPUT_DIR HOST_ARCH CC CXX AR RANLIB STRIP", file=sys.stderr)
    sys.exit(1)

target_output_dir, host_arch, cc, cxx, ar, ranlib, strip = sys.argv[1:]
host_triplet = 'arm-linux-gnueabihf'
arch_ldflags = ''

# the path to the XCSoar sources
xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')
from build.download import download_and_verify
from build.tar import untar

output_path = os.path.abspath('output')
tarball_path = os.path.join(output_path, 'download')
src_path = os.path.join(output_path, 'src')
target_output_dir = os.path.abspath(target_output_dir)
target_root = os.path.join(target_output_dir, 'root')

lib_path = os.path.join(target_output_dir, 'lib')
arch_path = os.path.join(lib_path, host_arch)
build_path = os.path.join(arch_path, 'build')
root_path = os.path.join(arch_path, 'root')

target_arch = '-march=armv7-a -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard'
cppflags = '-isystem ' + os.path.join(root_path, 'include') + ' -DNDEBUG'
ldflags = '-L' + os.path.join(root_path, 'lib')
libs = ''

# redirect pkg-config to use our root directory instead of the default
# one on the build host
os.environ['PKG_CONFIG_LIBDIR'] = os.path.join(root_path, 'lib/pkgconfig')

if 'MAKEFLAGS' in os.environ:
    # build/make.mk adds "--no-builtin-rules --no-builtin-variables",
    # which breaks the zlib Makefile (and maybe others)
    del os.environ['MAKEFLAGS']

class Toolchain:
    def __init__(self, tarball_path, src_path, build_path, install_prefix,
                 arch, arch_cflags, cppflags, arch_ldflags,
                 cc, cxx, ar, ranlib, strip):
        self.tarball_path = tarball_path
        self.src_path = src_path
        self.build_path = build_path
        self.install_prefix = install_prefix
        self.arch = arch

        self.cc = cc
        self.cxx = cxx
        self.ar = ar
        self.ranlib = ranlib
        self.strip = strip

        common_flags = '-Os -g -ffunction-sections -fdata-sections -fvisibility=hidden ' + arch_cflags
        self.cflags = common_flags
        self.cxxflags = common_flags
        self.cppflags = '-isystem ' + os.path.join(install_prefix, 'include') + ' -DNDEBUG ' + cppflags
        self.ldflags = '-L' + os.path.join(install_prefix, 'lib') + ' ' + arch_ldflags
        self.libs = ''

        self.env = dict(os.environ)

        # redirect pkg-config to use our root directory instead of the
        # default one on the build host
        self.env['PKG_CONFIG_LIBDIR'] = os.path.join(install_prefix, 'lib/pkgconfig')

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

    def download(self, toolchain):
        return download_and_verify(self.url, self.md5, toolchain.tarball_path)

    def is_installed(self, toolchain):
        tarball = self.download(toolchain)
        installed = os.path.join(toolchain.install_prefix, self.installed)
        tarball_mtime = os.path.getmtime(tarball)
        try:
            return os.path.getmtime(installed) >= tarball_mtime
        except FileNotFoundError:
            return False

    def unpack(self, toolchain, out_of_tree=True):
        if out_of_tree:
            parent_path = toolchain.src_path
        else:
            parent_path = toolchain.build_path
        path = untar(self.download(toolchain), parent_path, self.base)
        return path

    def make_build_path(self, toolchain):
        path = os.path.join(toolchain.build_path, self.base)
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
        src = self.unpack(toolchain)
        if self.autogen:
            subprocess.check_call(['/usr/bin/aclocal'], cwd=src)
            subprocess.check_call(['/usr/bin/automake', '--add-missing', '--force-missing', '--foreign'], cwd=src)
            subprocess.check_call(['/usr/bin/autoconf'], cwd=src)
            subprocess.check_call(['/usr/bin/libtoolize', '--force'], cwd=src)

        build = self.make_build_path(toolchain)

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
            'RANLIB=' + toolchain.ranlib,
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
        'http://download.savannah.gnu.org/releases/freetype/freetype-2.6.3.tar.bz2',
        '0037b25a8c090bc8a1218e867b32beb1',
        'lib/libfreetype.a',
        [
            '--disable-shared', '--enable-static',
            '--without-bzip2', '--without-png',
            '--without-harfbuzz',
        ],
    ),

    AutotoolsProject(
        'http://curl.haxx.se/download/curl-7.49.1.tar.lzma',
        'ae5e5e395da413d1fa0864e1d0a3fa57',
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
        'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.28.tar.xz',
        '425354f86c392318d31aedca71019372',
        'lib/libpng.a',
        [
            '--disable-shared', '--enable-static',
            '--enable-arm-neon',
        ]
    ),

    AutotoolsProject(
        'http://downloads.sourceforge.net/project/libjpeg-turbo/1.5.0/libjpeg-turbo-1.5.0.tar.gz',
        '3fc5d9b6a8bce96161659ae7a9939257',
        'lib/libjpeg.a',
        [
            '--disable-shared', '--enable-static',
            '--enable-arm-neon',
        ]
    ),
]

# build the third-party libraries
toolchain = Toolchain(tarball_path, src_path, build_path, root_path,
                      host_triplet, target_arch, cppflags, arch_ldflags,
                      cc, cxx, ar, ranlib, strip)
for x in thirdparty_libs:
    if not x.is_installed(toolchain):
        x.build(toolchain)
