from os.path import abspath

from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.openssl import OpenSSLProject
from build.freetype import FreeTypeProject
from build.curl import CurlProject
from build.libpng import LibPNGProject
from build.sdl2 import SDL2Project
from build.lua import LuaProject

glibc = AutotoolsProject(
    'http://mirror.netcologne.de/gnu/libc/glibc-2.23.tar.xz',
    'http://ftp.gnu.org/gnu/glibc/glibc-2.23.tar.xz',
    '456995968f3acadbed39f5eba31678df',
    'include/unistd.h',
    [
        '--enable-kernel=2.6.35',
        '--disable-werror',
        '--disable-build-nscd',
        '--disable-nscd',
    ],
    patches=abspath('lib/glibc/patches'),
    shared=True,

    # This is needed so glibc can find its NSS modules
    make_args=['default-rpath=/opt/xcsoar/lib'],
)

musl = AutotoolsProject(
    'https://www.musl-libc.org/releases/musl-1.1.16.tar.gz',
    'https://fossies.org/linux/misc/musl-1.1.16.tar.gz',
    '937185a5e5d721050306cf106507a006c3f1f86d86cd550024ea7be909071011',
    'include/unistd.h',
    [
        '--disable-shared',
    ],
)

openssl = OpenSSLProject(
    'https://www.openssl.org/source/openssl-1.0.2k.tar.gz',
    'ftp://ftp.kfki.hu/pub/packages/security/openssl/openssl-1.0.2k.tar.gz',
    '6b3977c61f2aedf0f96367dcfb5c6e578cf37e7b8d913b4ecb6643c3cb88d8c0',
    'include/openssl/ossl_typ.h',
)

openssh = AutotoolsProject(
    'http://ftp.openbsd.org/pub/OpenBSD/OpenSSH/portable/openssh-7.2p2.tar.gz',
    'http://ftp.nluug.nl/security/OpenSSH/openssh-7.2p2.tar.gz',
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
    'http://downloads.sourceforge.net/project/libpng/zlib/1.2.11/zlib-1.2.11.tar.xz',
    '4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066',
    'lib/libz.a',
)

freetype = FreeTypeProject(
    'http://download.savannah.gnu.org/releases/freetype/freetype-2.8.tar.bz2',
    'http://downloads.sourceforge.net/project/freetype/freetype2/2.8/freetype-2.8.tar.bz2',
    'a3c603ed84c3c2495f9c9331fe6bba3bb0ee65e06ec331e0a0fb52158291b40b',
    'lib/libfreetype.a',
    [
        '--disable-shared', '--enable-static',
        '--without-bzip2', '--without-png',
        '--without-harfbuzz',
    ],
)

curl = CurlProject(
    'http://curl.haxx.se/download/curl-7.55.1.tar.xz',
    'https://github.com/curl/curl/releases/download/curl-7_55_1/curl-7.55.1.tar.bz2',
    '3eafca6e84ecb4af5f35795dee84e643d5428287e88c041122bb8dac18676bb7',
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
)

proj = AutotoolsProject(
    'http://download.osgeo.org/proj/proj-4.9.3.tar.gz',
    'https://fossies.org/linux/privat/proj-4.9.3.tar.gz',
    'd598336ca834742735137c5674b214a1',
    'lib/libproj.a',
    [
        '--disable-shared', '--enable-static',
        '--without-mutex',
    ],
    patches=abspath('lib/proj/patches'),
    autogen=True,
)

libpng = LibPNGProject(
    'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.32.tar.xz',
    'http://downloads.sourceforge.net/project/libpng/libpng16/1.6.32/libpng-1.6.32.tar.xz',
    'c918c3113de74a692f0a1526ce881dc26067763eb3915c57ef3a0f7b6886f59b',
    'lib/libpng.a',
    [
        '--disable-shared', '--enable-static',
    ]
)

libjpeg = AutotoolsProject(
    'http://downloads.sourceforge.net/project/libjpeg-turbo/1.5.2/libjpeg-turbo-1.5.2.tar.gz',
    'http://sourceforge.mirrorservice.org/l/li/libjpeg-turbo/1.5.2/libjpeg-turbo-1.5.2.tar.gz',
    '9098943b270388727ae61de82adec73cf9f0dbb240b3bc8b172595ebf405b528',
    'lib/libjpeg.a',
    [
        '--disable-shared', '--enable-static',
    ]
)

libusb = AutotoolsProject(
    'https://github.com//libusb/libusb/releases/download/v1.0.21/libusb-1.0.21.tar.bz2',
    'http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.21/libusb-1.0.21.tar.bz2',
    '7dce9cce9a81194b7065ee912bcd55eeffebab694ea403ffb91b67db66b1824b',
    'lib/libusb-1.0.a',
    [
        '--disable-shared', '--enable-static',
        '--disable-udev',
    ]
)

simple_usbmodeswitch = AutotoolsProject(
    'https://github.com/felixhaedicke/simple_usbmodeswitch/releases/download/v1.0/simple_usbmodeswitch-1.0.tar.bz2',
    'http://s15356785.onlinehome-server.info/~felix/simple_usbmodeswitch/simple_usbmodeswitch-1.0.tar.bz2',
    '35e8a6ed8551ef419baf7310e54d6d1a81e18bf44e111b07d74285001f18e98d',
    'bin/simple_usbmodeswitch',
    ldflags='-pthread',
)

libtiff = AutotoolsProject(
    'http://download.osgeo.org/libtiff/tiff-4.0.8.tar.gz',
    'http://ftp.lfs-matrix.net/pub/blfs/conglomeration/tiff/tiff-4.0.8.tar.gz',
    '59d7a5a8ccd92059913f246877db95a2918e6c04fb9d43fd74e5c3390dac2910',
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
    patches=abspath('lib/libtiff/patches'),
    autogen=True,
)

libgeotiff = AutotoolsProject(
    'http://download.osgeo.org/geotiff/libgeotiff/libgeotiff-1.4.2.tar.gz',
    'https://fossies.org/linux/privat/libgeotiff-1.4.2.tar.gz',
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
    'http://downloads.sourceforge.net/project/libsdl/SDL/2.0.5/SDL2-2.0.5.tar.gz',
    'd4055424d556b4a908aa76fad63abd3c',
    'lib/libSDL2.a',
    [
        '--disable-shared', '--enable-static',
    ],
    patches=abspath('lib/sdl2/patches'),
)

lua = LuaProject(
    'http://www.lua.org/ftp/lua-5.3.4.tar.gz',
    'https://github.com/lua/lua/releases/download/v5-3-4/lua-5.3.4.tar.gz',
    '79790cfd40e09ba796b01a571d4d63b52b1cd950',
    'lib/liblua.a',
    patches=abspath('lib/lua/patches'),
)

libsalsa = AutotoolsProject(
    'ftp://ftp.suse.com/pub/people/tiwai/salsa-lib/salsa-lib-0.1.6.tar.bz2',
    'http://vesta.informatik.rwth-aachen.de/ftp/pub/Linux/suse/people/tiwai/salsa-lib/salsa-lib-0.1.6.tar.bz2',
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
