from os.path import abspath

from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.cmake import CmakeProject
from build.openssl import OpenSSLProject
from build.freetype import FreeTypeProject
from build.curl import CurlProject
from build.libpng import LibPNGProject
from build.libstdcxxmuslheaders import LibstdcxxMuslHeadersProject
from build.sdl2 import SDL2Project
from build.lua import LuaProject

musl = AutotoolsProject(
    'https://www.musl-libc.org/releases/musl-1.1.18.tar.gz',
    'https://fossies.org/linux/misc/musl-1.1.18.tar.gz',
    'd017ee5d01aec0c522a1330fdff06b1e428cb409e1db819cc4935d5da4a5a118',
    'include/unistd.h',
    [
        '--disable-shared',
    ],
    patches=abspath('lib/musl/patches'),
)

libstdcxx_musl_headers = LibstdcxxMuslHeadersProject(
    'https://ftp.gnu.org/gnu/gcc/gcc-10.2.0/gcc-10.2.0.tar.xz',
    'http://mirrors.ibiblio.org/gnu/ftp/gnu/gcc/gcc-10.2.0/gcc-10.2.0.tar.xz',
    'b8dd4368bb9c7f0b98188317ee0254dd8cc99d1e3a18d0ff146c855fe16c1d8c',
    'include/libstdc++/algorithm',
    [
        '--enable-clocale=generic',
        '--disable-shared',
        '--disable-multilib',
    ],
    config_script='libstdc++-v3/configure',
    use_actual_arch=True,
)

openssl = OpenSSLProject(
    'https://www.openssl.org/source/openssl-3.0.0-alpha13.tar.gz',
    'ftp://ftp.cert.dfn.de/pub/tools/net/openssl/source/openssl-3.0.0-alpha13.tar.gz',
    'c88cbb9d330b4daa3dbb5af1ed511d5062253291a56e09fd17e9ac013a20f8a3',
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

libsodium = AutotoolsProject(
    'https://download.libsodium.org/libsodium/releases/libsodium-1.0.18.tar.gz',
    "https://github.com/jedisct1/libsodium/releases/download/1.0.18-RELEASE/libsodium-1.0.18.tar.gz",
    '6f504490b342a4f8a4c4a02fc9b866cbef8622d5df4e5452b46be121e46636c1',
    'include/sodium/crypto_hash_sha256.h',
    [
        '--disable-shared', '--enable-static',
    ],
)

zlib = ZlibProject(
    'http://zlib.net/zlib-1.2.11.tar.xz',
    'http://downloads.sourceforge.net/project/libpng/zlib/1.2.11/zlib-1.2.11.tar.xz',
    '4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066',
    'lib/libz.a',
)

freetype = FreeTypeProject(
    'http://download.savannah.gnu.org/releases/freetype/freetype-2.10.2.tar.xz',
    'http://downloads.sourceforge.net/project/freetype/freetype2/2.10.2/freetype-2.10.2.tar.xz',
    '1543d61025d2e6312e0a1c563652555f17378a204a61e99928c9fcef030a2d8b',
    'lib/libfreetype.a',
    [
        '--disable-shared', '--enable-static',
        '--without-bzip2', '--without-png',
        '--without-harfbuzz',
    ],
)

cares = CmakeProject(
    'https://c-ares.haxx.se/download/c-ares-1.17.1.tar.gz',
    'https://c-ares.haxx.se/download/c-ares-1.17.1.tar.gz',
    'd73dd0f6de824afd407ce10750ea081af47eba52b8a6cb307d220131ad93fc40',
    'lib/libcares.a',
    [
        '-DCARES_STATIC=ON',
        '-DCARES_SHARED=OFF',
        '-DCARES_STATIC_PIC=ON',
        '-DCARES_BUILD_TOOLS=OFF',
    ],
    patches=abspath('lib/c-ares/patches'),
    #autogen=True,
    #subdirs=['include', 'src/lib'],
)

curl = CmakeProject(
    'https://curl.se/download/curl-7.75.0.tar.xz',
    'https://github.com/curl/curl/releases/download/curl-7_75_0/curl-7.75.0.tar.xz',
    'fe0c49d8468249000bda75bcfdf9e30ff7e9a86d35f1a21f428d79c389d55675',
    'lib/libcurl.a',
    [
        '-DBUILD_CURL_EXE=OFF',
        '-DBUILD_SHARED_LIBS=OFF',
        '-DENABLE_ARES=ON',
        '-DCURL_DISABLE_LDAP=ON',
        '-DCURL_DISABLE_TELNET=ON',
        '-DCURL_DISABLE_DICT=ON',
        '-DCURL_DISABLE_FILE=ON',
        '-DCURL_DISABLE_TFTP=ON',
        '-DCURL_DISABLE_LDAPS=ON',
        '-DCURL_DISABLE_RTSP=ON',
        '-DCURL_DISABLE_PROXY=ON',
        '-DCURL_DISABLE_POP3=ON',
        '-DCURL_DISABLE_IMAP=ON',
        '-DCURL_DISABLE_SMTP=ON',
        '-DCURL_DISABLE_GOPHER=ON',
        '-DCURL_DISABLE_COOKIES=ON',
        '-DCURL_DISABLE_CRYPTO_AUTH=ON',
        '-DCURL_DISABLE_IMAP=ON',
        '-DCMAKE_USE_LIBSSH2=OFF',
        '-DBUILD_TESTING=OFF',
    ],
    windows_configure_args=[
        '-DCMAKE_USE_SCHANNEL=ON',
    ],
    patches=abspath('lib/curl/patches'),
)

proj = AutotoolsProject(
    'http://download.osgeo.org/proj/proj-5.1.0.tar.gz',
    'https://fossies.org/linux/privat/proj-5.1.0.tar.gz',
    '6b1379a53317d9b5b8c723c1dc7bf2e3a8eb22ceb46b8807a1ce48ef65685bb3',
    'lib/libproj.a',
    [
        '--disable-shared', '--enable-static',
        '--without-mutex',
    ],
    patches=abspath('lib/proj/patches'),
    autogen=True,
)

libpng = AutotoolsProject(
    'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.37.tar.xz',
    'http://downloads.sourceforge.net/project/libpng/libpng16/1.6.37/libpng-1.6.37.tar.xz',
    '505e70834d35383537b6491e7ae8641f1a4bed1876dbfe361201fc80868d88ca',
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
    'http://download.osgeo.org/libtiff/tiff-4.0.10.tar.gz',
    'http://ftp.lfs-matrix.net/pub/blfs/conglomeration/tiff/tiff-4.0.10.tar.gz',
    '2c52d11ccaf767457db0c46795d9c7d1a8d8f76f68b0b800a3dfe45786b996e4',
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
        '--disable-zstd',
        '--disable-webp',
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
    'http://www.libsdl.org/release/SDL2-2.0.12.tar.gz',
    'https://fossies.org/linux/misc/SDL2-2.0.12.tar.gz',
    '349268f695c02efbc9b9148a70b85e58cefbbf704abd3e91be654db7f1e2c863',
    'lib/libSDL2.a',
    [
        '--disable-shared', '--enable-static',
    ],
    patches=abspath('lib/sdl2/patches'),
)

lua = LuaProject(
    'http://www.lua.org/ftp/lua-5.3.5.tar.gz',
    'https://github.com/lua/lua/releases/download/v5-3-5/lua-5.3.5.tar.gz',
    '0c2eed3f960446e1a3e4b9a1ca2f3ff893b6ce41942cf54d5dd59ab4b3b058ac',
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
