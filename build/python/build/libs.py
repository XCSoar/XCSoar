from os.path import abspath

from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.cmake import CmakeProject
from build.openssl import OpenSSLProject
from build.freetype import FreeTypeProject
from build.curl import CurlProject
from build.libpng import LibPNGProject
from build.gcc import BinutilsProject, GccProject, GccBootstrapProject
from build.linux import SabotageLinuxHeadersProject
from build.sdl2 import SDL2Project
from build.lua import LuaProject

binutils = BinutilsProject(
    'https://ftp.gnu.org/gnu/binutils/binutils-2.36.1.tar.xz',
    'http://mirrors.ibiblio.org/gnu/ftp/gnu/binutils/binutils-2.36.1.tar.xz',
    'e81d9edf373f193af428a0f256674aea62a9d74dfe93f65192d4eae030b0f3b0',
    'bin/as',
    [
        '--with-system-zlib',
        '--enable-gold',
        '--disable-ld',
        '--disable-libquadmath',
        '--disable-lto',
    ],
    use_actual_arch=True,
)

linux_headers = SabotageLinuxHeadersProject(
    'http://ftp.barfooze.de/pub/sabotage/tarballs/linux-headers-4.19.88.tar.xz',
    'http://foss.aueb.gr/mirrors/linux/sabotage/tarballs/linux-headers-4.19.88.tar.xz',
    '5a975ba49b577869f2338aa80f44efd4e94f76e5b4bda11a6a1761a6d646848fdeaad7c820339b2c1c20d55f9bbf0e686121d621ac1cfa1dfc6cd71a166ade3a',
    'include/linux/input.h',
)

gcc = GccProject(
    'https://ftp.gnu.org/gnu/gcc/gcc-11.1.0/gcc-11.1.0.tar.xz',
    'http://mirrors.ibiblio.org/gnu/ftp/gnu/gcc/gcc-11.1.0/gcc-11.1.0.tar.xz',
    '4c4a6fb8a8396059241c2e674b85b351c26a5d678274007f076957afa1cc9ddf',
    'lib/libstdc++.a',
    [
        # GCC fails to build if we disable the shared libstdc++
        #'--disable-shared', '--enable-static',

        '--with-gcc',
        '--with-gnu-ld',
        '--with-gnu-as',
        '--disable-nls',
        '--disable-lto',
        '--enable-clocale=generic',
        '--enable-languages=c,c++',
        '--disable-bootstrap',
        '--disable-multilib',
        '--without-newlib',

        '--disable-wchar_t',
        '--disable-symvers',

        '--disable-decimal-float',
        '--disable-libatomic',
        '--disable-libgomp', '--disable-libmpx', '--disable-libquadmath',
        '--disable-libssp', '--disable-libvtv',
        '--disable-libsanitizer',

        '--disable-libstdcxx-verbose',
        '--disable-libstdcxx-dual-abi',
        '--disable-libstdcxx-filesystem-ts',

        # TODO: don't hard-code Kobo settings
        '--with-arch=armv7-a+vfpv3',
        '--with-fpu=neon',
        '--with-float=hard',
    ],
    use_actual_arch=True,
)

gcc_bootstrap = GccBootstrapProject(
    gcc.url,
    gcc.alternative_url,
    gcc.md5,
    '../bin/armv7a-a8neon-linux-musleabihf-g++',
    gcc.configure_args,
    install_target='install-gcc',
    use_actual_arch=True,
)

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

openssl = OpenSSLProject(
    'https://www.openssl.org/source/openssl-3.0.0-alpha16.tar.gz',
    'ftp://ftp.cert.dfn.de/pub/tools/net/openssl/source/openssl-3.0.0-alpha16.tar.gz',
    '08ce8244b59d75f40f91170dfcb012bf25309cdcb1fef9502e39d694f883d1d1',
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
    'http://download.savannah.gnu.org/releases/freetype/freetype-2.10.4.tar.xz',
    'http://downloads.sourceforge.net/project/freetype/freetype2/2.10.4/freetype-2.10.4.tar.xz',
    '86a854d8905b19698bbc8f23b860bc104246ce4854dcea8e3b0fb21284f75784',
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
    'https://curl.se/download/curl-7.76.1.tar.xz',
    'https://github.com/curl/curl/releases/download/curl-7_76_1/curl-7.76.1.tar.xz',
    '64bb5288c39f0840c07d077e30d9052e1cbb9fa6c2dc52523824cc859e679145',
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
