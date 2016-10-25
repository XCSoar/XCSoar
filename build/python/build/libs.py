from os.path import abspath

from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.openssl import OpenSSLProject
from build.freetype import FreeTypeProject
from build.sdl2 import SDL2Project
from build.lua import LuaProject

glibc = AutotoolsProject(
    'http://mirror.netcologne.de/gnu/libc/glibc-2.24.tar.xz',
    '97dc5517f92016f3d70d83e3162ad318',
    'include/unistd.h',
    [
        '--enable-static-nss',
        '--enable-kernel=2.6.35',
        '--disable-werror',
        '--disable-build-nscd',
        '--disable-nscd',
    ],
    shared=True,
)

openssl = OpenSSLProject(
    'https://www.openssl.org/source/openssl-1.0.2j.tar.gz',
    '96322138f0b69e61b7212bc53d5e912b',
    'include/openssl/ossl_typ.h',
)

openssh = AutotoolsProject(
    'http://ftp.openbsd.org/pub/OpenBSD/OpenSSH/portable/openssh-7.2p2.tar.gz',
    '13009a9156510d8f27e752659075cced',
    'opt/openssh/sbin/sshd',
    [
        '--disable-etc-default-login',
        '--disable-lastlog',
        '--disable-utmp',
        '--disable-utmpx',
        '--disable-wtmp',
        '--disable-wtmpx',
        '--disable-libutil',
        '--disable-pututline',
        '--disable-pututxline',
        '--without-openssl',
        '--without-ssh1',
        '--without-stackprotect',
        '--without-hardening',
        '--without-shadow',
        '--without-sandbox',
        '--without-selinux',
    ],
    ldflags='-static',
    install_prefix='/opt/openssh',
    install_target='install-nosysconf',
    use_destdir=True,
)

zlib = ZlibProject(
    'http://zlib.net/zlib-1.2.8.tar.xz',
    '28f1205d8dd2001f26fec1e8c2cebe37',
    'lib/libz.a',
)

freetype = FreeTypeProject(
    'http://download.savannah.gnu.org/releases/freetype/freetype-2.7.tar.bz2',
    'be4601619827b7935e1d861745923a68',
    'lib/libfreetype.a',
    [
        '--disable-shared', '--enable-static',
        '--without-bzip2', '--without-png',
        '--without-harfbuzz',
    ],
)

curl = AutotoolsProject(
    'http://curl.haxx.se/download/curl-7.50.3.tar.lzma',
    '6080c1eb3e72d5da6c892ba72a074ad2',
    'lib/libcurl.a',
    [
        '--disable-shared', '--enable-static',
        '--disable-debug',
        '--enable-http',
        '--enable-ipv6',
        '--enable-ftp', '--disable-file',
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
    patches=abspath('lib/curl/patches'),
    use_clang=True,
)

proj = AutotoolsProject(
    'http://download.osgeo.org/proj/proj-4.9.3.tar.gz',
    'd598336ca834742735137c5674b214a1',
    'lib/libproj.a',
    [
        '--disable-shared', '--enable-static',
        '--without-mutex',
    ],
    patches=abspath('lib/proj/patches'),
    autogen=True,
)

libpng = AutotoolsProject(
    'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.26.tar.xz',
    'faed9bb495d2e12dd0c9ec561ca60cd8',
    'lib/libpng.a',
    [
        '--disable-shared', '--enable-static',
        '--enable-arm-neon',
    ]
)

libjpeg = AutotoolsProject(
    'http://downloads.sourceforge.net/project/libjpeg-turbo/1.5.1/libjpeg-turbo-1.5.1.tar.gz',
    '55deb139b0cac3c8200b75d485fc13f3',
    'lib/libjpeg.a',
    [
        '--disable-shared', '--enable-static',
    ]
)

libtiff = AutotoolsProject(
    'http://download.osgeo.org/libtiff/tiff-4.0.6.tar.gz',
    'd1d2e940dea0b5ad435f21f03d96dd72',
    'lib/libtiff.a',
    [
        '--disable-shared', '--enable-static',
        '--disable-largefile',
        '--disable-cxx',
        '--disable-ccitt',
        '--disable-packbits',
        '--disable-lzw',
        '--disable-thunder',
        '--disable-next',
        '--disable-logluv',
        '--disable-mdi',
        '--disable-pixarlog',
        '--disable-jpeg',
        '--disable-old-jpeg',
        '--disable-jbig',
        '--disable-lzma',
        '--disable-strip-chopping',
        '--disable-extrasample-as-alpha',
    ],
    base='tiff-4.0.6',
    patches=abspath('lib/libtiff/patches'),
    autogen=True,
)

libgeotiff = AutotoolsProject(
    'http://download.osgeo.org/geotiff/libgeotiff/libgeotiff-1.4.2.tar.gz',
    '96ab80e0d4eff7820579957245d844f8',
    'lib/libgeotiff.a',
    [
        '--disable-shared', '--enable-static',
        '--disable-doxygen-doc',
        '--disable-doxygen-dot',
        '--disable-doxygen-man',
        '--disable-doxygen-html',
    ],
    patches=abspath('lib/libgeotiff/patches'),
    autogen=True,
    libs='-lz',
)

sdl2 = SDL2Project(
    'http://www.libsdl.org/release/SDL2-2.0.4.tar.gz',
    '44fc4a023349933e7f5d7a582f7b886e',
    'lib/libSDL2.a',
    [
        '--disable-shared', '--enable-static',
    ]
)

lua = LuaProject(
    'http://www.lua.org/ftp/lua-5.3.3.tar.gz',
    '703f75caa4fdf4a911c1a72e67a27498',
    'lib/liblua.a',
    patches=abspath('lib/lua/patches'),
)

libsalsa = AutotoolsProject(
    'ftp://ftp.suse.com/pub/people/tiwai/salsa-lib/salsa-lib-0.1.6.tar.bz2',
    '08a6481cdbf4c79e05a9cba3b6c48375',
    'lib/libsalsa.a',
    [
        '--disable-4bit',
        '--disable-user-elem',
        '--enable-shared=no',
        '--enable-tlv'
    ],
    patches=abspath('lib/salsa-lib/patches')
)
