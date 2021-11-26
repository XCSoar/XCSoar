from os.path import abspath

from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.meson import MesonProject
from build.cmake import CmakeProject
from build.openssl import OpenSSLProject
from build.gcc import BinutilsProject, GccProject, GccBootstrapProject
from build.linux import SabotageLinuxHeadersProject
from build.sdl2 import SDL2Project
from build.lua import LuaProject
from .musl import MuslProject

binutils = BinutilsProject(
    'https://ftp.gnu.org/gnu/binutils/binutils-2.37.tar.xz',
    'http://mirrors.ibiblio.org/gnu/ftp/gnu/binutils/binutils-2.37.tar.xz',
    '820d9724f020a3e69cb337893a0b63c2db161dadcb0e06fc11dc29eb1e84a32c',
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
    'https://ftp.gnu.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz',
    'http://mirrors.ibiblio.org/gnu/ftp/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz',
    'd08edc536b54c372a1010ff6619dd274c0f1603aa49212ba20f7aa2cda36fa8b',
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

musl = MuslProject(
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
    'https://www.openssl.org/source/openssl-3.0.0.tar.gz',
    'ftp://ftp.cert.dfn.de/pub/tools/net/openssl/source/openssl-3.0.0.tar.gz',
    '59eedfcb46c25214c9bd37ed6078297b4df01d012267fe9e9eee31f61bc70536',
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

freetype = MesonProject(
    'http://download.savannah.gnu.org/releases/freetype/freetype-2.11.0.tar.xz',
    'http://downloads.sourceforge.net/project/freetype/freetype2/2.11.0/freetype-2.11.0.tar.xz',
    '8bee39bd3968c4804b70614a0a3ad597299ad0e824bc8aad5ce8aaf48067bde7',
    'lib/libfreetype.a',
    [
        '-Dbrotli=disabled',
        '-Dbzip2=disabled',
        '-Dharfbuzz=disabled',
        '-Dpng=disabled',
        '-Dzlib=enabled',
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
)

curl = CmakeProject(
    'https://curl.se/download/curl-7.80.0.tar.xz',
    'https://github.com/curl/curl/releases/download/curl-7_80_0/curl-7.80.0.tar.xz',
    'a132bd93188b938771135ac7c1f3ac1d3ce507c1fcbef8c471397639214ae2ab',
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

proj = CmakeProject(
    'http://download.osgeo.org/proj/proj-5.2.0.tar.gz',
    'https://fossies.org/linux/privat/proj-5.2.0.tar.gz',
    'ef919499ffbc62a4aae2659a55e2b25ff09cccbbe230656ba71c6224056c7e60',
    'lib/libproj.a',
    [
        '-DPROJ_TESTS=OFF',
        '-DBUILD_CCT=OFF',
        '-DBUILD_CS2CS=OFF',
        '-DBUILD_GEOD=OFF',
        '-DBUILD_GIE=OFF',
        '-DBUILD_NAD2BIN=OFF',
        '-DBUILD_PROJ=OFF',
        '-DBUILD_LIBPROJ_SHARED=OFF',
        '-DUSE_THREAD=OFF',
    ],
    patches=abspath('lib/proj/patches'),
)

libpng = CmakeProject(
    'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.37.tar.xz',
    'http://downloads.sourceforge.net/project/libpng/libpng16/1.6.37/libpng-1.6.37.tar.xz',
    '505e70834d35383537b6491e7ae8641f1a4bed1876dbfe361201fc80868d88ca',
    'lib/libpng.a',
    [
        '-DPNG_SHARED=OFF',
        '-DPNG_TESTS=OFF',
    ]
)

libjpeg = AutotoolsProject(
    'http://downloads.sourceforge.net/project/libjpeg-turbo/1.5.3/libjpeg-turbo-1.5.3.tar.gz',
    'http://sourceforge.mirrorservice.org/l/li/libjpeg-turbo/1.5.3/libjpeg-turbo-1.5.3.tar.gz',
    'b24890e2bb46e12e72a79f7e965f409f4e16466d00e1dd15d93d73ee6b592523',
    'lib/libjpeg.a',
    [
        '--disable-shared', '--enable-static',
        '--without-turbojpeg',
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

libtiff = CmakeProject(
    'http://download.osgeo.org/libtiff/tiff-4.3.0.tar.gz',
    'https://fossies.org/linux/misc/tiff-4.3.0.tar.gz',
    '0e46e5acb087ce7d1ac53cf4f56a09b221537fc86dfc5daaad1c2e89e1b37ac8',
    'lib/libtiff.a',
    [
        '-DBUILD_SHARED_LIBS=OFF',
        '-Dld-version-script=OFF',
        '-Dccitt=OFF',
        '-Dpackbits=OFF',
        '-Dlzw=OFF',
        '-Dthunder=OFF',
        '-Dnext=OFF',
        '-Dlogluv=OFF',
        '-Dmdi=OFF',
        '-Dpixarlog=OFF',
        '-Djpeg=OFF',
        '-Dold-jpeg=OFF',
        '-Djbig=OFF',
        '-Dlzma=OFF',
        '-Dzstd=OFF',
        '-Dlerc=OFF',
        '-Dwebp=OFF',
        '-Dcxx=OFF',
        '-Dstrip-chopping=OFF',
        '-Dextrasample-as-alpha=OFF',

        # workaround for build failure with -Dstrip-chopping=OFF
        '-DSTRIP_SIZE_DEFAULT=8192',

        '-DCMAKE_EXE_LINKER_FLAGS=-lm',
    ],
    patches=abspath('lib/libtiff/patches'),
)

libgeotiff = CmakeProject(
    'http://download.osgeo.org/geotiff/libgeotiff/libgeotiff-1.4.3.tar.gz',
    'https://fossies.org/linux/privat/libgeotiff-1.4.3.tar.gz',
    'b8510d9b968b5ee899282cdd5bef13fd02d5a4c19f664553f81e31127bc47265',
    'lib/libgeotiff.a',
    [
        '-DWITH_UTILITIES=OFF',
        '-DBUILD_SHARED_LIBS=OFF',
    ],
    patches=abspath('lib/libgeotiff/patches'),
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
    'http://www.lua.org/ftp/lua-5.4.3.tar.gz',
    'https://github.com/lua/lua/releases/download/v5-3-5/lua-5.4.3.tar.gz',
    '1dda2ef23a9828492b4595c0197766de6e784bc7',
    'lib/liblua.a',
    patches=abspath('lib/lua/patches'),
)

libsalsa = AutotoolsProject(
    'ftp://ftp.suse.com/pub/people/tiwai/salsa-lib/salsa-lib-0.1.6.tar.bz2',
    'https://mirror.linux-ia64.org/ftp_suse_com/people/tiwai/salsa-lib/salsa-lib-0.1.6.tar.bz2',
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
