from os.path import abspath

from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.openssl import OpenSSLProject
from build.freetype import FreeTypeProject
from build.sdl2 import SDL2Project
from build.lua import LuaProject

glibc = AutotoolsProject(
    'http://mirror.netcologne.de/gnu/libc/glibc-2.23.tar.xz',
    '456995968f3acadbed39f5eba31678df',
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
    'https://www.openssl.org/source/openssl-1.0.2k.tar.gz',
    '6b3977c61f2aedf0f96367dcfb5c6e578cf37e7b8d913b4ecb6643c3cb88d8c0',
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
    'http://zlib.net/zlib-1.2.11.tar.xz',
    '4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066',
    'lib/libz.a',
)

freetype = FreeTypeProject(
    'http://download.savannah.gnu.org/releases/freetype/freetype-2.7.1.tar.bz2',
    '3a3bb2c4e15ffb433f2032f50a5b5a92558206822e22bfe8cbe339af4aa82f88',
    'lib/libfreetype.a',
    [
        '--disable-shared', '--enable-static',
        '--without-bzip2', '--without-png',
        '--without-harfbuzz',
    ],
)

curl = AutotoolsProject(
    'http://curl.haxx.se/download/curl-7.53.1.tar.lzma',
    '4b124ff5984f2b537790a8f50dbf3d44da89e57d0505ba567128535a2426f5e2',
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
    'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.29.tar.xz',
    '4245b684e8fe829ebb76186327bb37ce5a639938b219882b53d64bd3cfc5f239',
    'lib/libpng.a',
    [
        '--disable-shared', '--enable-static',
        '--enable-arm-neon',
    ]
)

libjpeg = AutotoolsProject(
    'http://downloads.sourceforge.net/project/libjpeg-turbo/1.5.1/libjpeg-turbo-1.5.1.tar.gz',
    '41429d3d253017433f66e3d472b8c7d998491d2f41caa7306b8d9a6f2a2c666c',
    'lib/libjpeg.a',
    [
        '--disable-shared', '--enable-static',
    ]
)

libtiff = AutotoolsProject(
    'http://download.osgeo.org/libtiff/tiff-4.0.7.tar.gz',
    '9f43a2cfb9589e5cecaa66e16bf87f814c945f22df7ba600d63aac4632c4f019',
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
    base='tiff-4.0.7',
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
    'http://www.libsdl.org/release/SDL2-2.0.5.tar.gz',
    'd4055424d556b4a908aa76fad63abd3c',
    'lib/libSDL2.a',
    [
        '--disable-shared', '--enable-static',
    ]
)

lua = LuaProject(
    'http://www.lua.org/ftp/lua-5.3.4.tar.gz',
    '79790cfd40e09ba796b01a571d4d63b52b1cd950',
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
