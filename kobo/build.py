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
common_flags = '-Os -g -ffunction-sections -fdata-sections -fvisibility=hidden ' + target_arch
cflags = common_flags
cxxflags = common_flags
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

def file_md5(path):
    """Calculate the MD5 checksum of a file and return it in hexadecimal notation."""

    with open(path, 'rb') as f:
        m = hashlib.md5()
        while True:
            data = f.read(65536)
            if len(data) == 0:
                # end of file
                return m.hexdigest()
            m.update(data)

def download_tarball(url, md5):
    """Download a tarball, verify its MD5 checksum and return the local path."""

    global tarball_path
    os.makedirs(tarball_path, exist_ok=True)
    path = os.path.join(tarball_path, os.path.basename(url))

    try:
        calculated_md5 = file_md5(path)
        if md5 == calculated_md5: return path
        os.unlink(path)
    except FileNotFoundError:
        pass

    tmp_path = path + '.tmp'

    print("download", url)
    urllib.request.urlretrieve(url, tmp_path)
    calculated_md5 = file_md5(tmp_path)
    if calculated_md5 != md5:
        os.unlink(tmp_path)
        raise "MD5 mismatch"

    os.rename(tmp_path, path)
    return path

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
        return download_tarball(self.url, self.md5)

    def is_installed(self):
        global root_path
        tarball = self.download()
        installed = os.path.join(root_path, self.installed)
        tarball_mtime = os.path.getmtime(tarball)
        try:
            return os.path.getmtime(installed) >= tarball_mtime
        except FileNotFoundError:
            return False

    def unpack(self, out_of_tree=True):
        global src_path, build_path
        tarball = self.download()
        if out_of_tree:
            parent_path = src_path
        else:
            parent_path = build_path
        path = os.path.join(parent_path, self.base)
        try:
            shutil.rmtree(path)
        except FileNotFoundError:
            pass
        os.makedirs(parent_path, exist_ok=True)
        subprocess.check_call(['/bin/tar', 'xfC', tarball, parent_path])
        return path

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

    def build(self):
        src = self.unpack(out_of_tree=False)

        subprocess.check_call(['./configure', '--prefix=' + root_path, '--static'], cwd=src)
        subprocess.check_call(['/usr/bin/make', '--quiet',
                               'CC=' + cc,
                               'CPP=' + cc + ' -E',
                               'AR=' + ar,
                               'LDSHARED=' + cc + ' -shared',
                               'install'],
                              cwd=src)

class AutotoolsProject(Project):
    def __init__(self, url, md5, installed, configure_args=[],
                 autogen=False,
                 cppflags='',
                 **kwargs):
        Project.__init__(self, url, md5, installed, **kwargs)
        self.configure_args = configure_args
        self.autogen = autogen
        self.cppflags = cppflags

    def configure(self):
        src = self.unpack()
        if self.autogen:
            subprocess.check_call(['/usr/bin/aclocal'], cwd=src)
            subprocess.check_call(['/usr/bin/automake', '--add-missing', '--force-missing', '--foreign'], cwd=src)
            subprocess.check_call(['/usr/bin/autoconf'], cwd=src)
            subprocess.check_call(['/usr/bin/libtoolize', '--force'], cwd=src)

        build = self.make_build_path()

        configure = [
            os.path.join(src, 'configure'),
            'CC=' + cc,
            'CXX=' + cxx,
            'CFLAGS=' + cflags,
            'CXXFLAGS=' + cxxflags,
            'CPPFLAGS=' + cppflags + ' ' + self.cppflags,
            'LDFLAGS=' + ldflags,
            'LIBS=' + libs,
            'AR=' + ar,
            'STRIP=' + strip,
            '--host=' + host_arch,
            '--prefix=' + root_path,
            '--enable-silent-rules',
        ] + self.configure_args

        subprocess.check_call(configure, cwd=build)
        return build

    def build(self):
        build = self.configure()

        subprocess.check_call(['/usr/bin/make', '--quiet', '-j12'], cwd=build)
        subprocess.check_call(['/usr/bin/make', '--quiet', 'install'], cwd=build)

class FreeTypeProject(AutotoolsProject):
    def configure(self):
        build = AutotoolsProject.configure(self)

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
        'http://download.savannah.gnu.org/releases/freetype/freetype-2.6.2.tar.bz2',
        '86109d0c998787d81ac582bad9adf82e',
        'lib/libfreetype.a',
        [
            '--disable-shared', '--enable-static',
            '--without-bzip2', '--without-png',
            '--without-harfbuzz',
        ],
    ),

    AutotoolsProject(
        'http://curl.haxx.se/download/curl-7.46.0.tar.lzma',
        'f845c513830d38c1b7ac39a98c1c2b11',
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
        'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.20.tar.xz',
        '3968acb7c66ef81a9dab867f35d0eb4b',
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
for x in thirdparty_libs:
    if not x.is_installed():
        x.build()
