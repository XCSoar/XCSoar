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
    'https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.xz',
    'https://fossies.org/linux/misc/binutils-2.40.tar.xz',
    '0f8a4c272d7f17f369ded10a4aca28b8e304828e95526da482b0ccc4dfc9d8e1',
    'bin/as',
    [
        '--with-system-zlib',
        '--enable-gold',
        '--disable-ld',
        '--disable-libquadmath',
        '--disable-lto',
    ],
)

linux_headers = SabotageLinuxHeadersProject(
    'http://ftp.barfooze.de/pub/sabotage/tarballs/linux-headers-4.19.88.tar.xz',
    'http://foss.aueb.gr/mirrors/linux/sabotage/tarballs/linux-headers-4.19.88.tar.xz',
    '5a975ba49b577869f2338aa80f44efd4e94f76e5b4bda11a6a1761a6d646848fdeaad7c820339b2c1c20d55f9bbf0e686121d621ac1cfa1dfc6cd71a166ade3a',
    'include/linux/input.h',
)

gcc = GccProject(
    'https://ftp.gnu.org/gnu/gcc/gcc-13.1.0/gcc-13.1.0.tar.xz',
    'https://fossies.org/linux/misc/gcc-13.1.0.tar.xz',
    '61d684f0aa5e76ac6585ad8898a2427aade8979ed5e7f85492286c4dfc13ee86',
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
        '--disable-libstdcxx-backtrace',
        '--disable-libstdcxx-filesystem-ts',

        # TODO: don't hard-code Kobo settings
        '--with-arch=armv7-a+vfpv3',
        '--with-fpu=neon',
        '--with-float=hard',
    ],
)

gcc_bootstrap = GccBootstrapProject(
    gcc.url,
    gcc.alternative_url,
    gcc.md5,
    '../bin/armv7a-kobo-linux-musleabihf-g++',
    gcc.configure_args,
    install_target='install-gcc',
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
    'https://www.openssl.org/source/openssl-3.1.1.tar.gz',
    'https://artfiles.org/openssl.org/source/openssl-3.1.1.tar.gz',
    'b3aa61334233b852b63ddb048df181177c2c659eb9d4376008118f9c08d07674',
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

libfmt = CmakeProject(
    'https://github.com/fmtlib/fmt/archive/10.0.0.tar.gz',
    'https://github.com/fmtlib/fmt/archive/10.0.0.tar.gz',
    'ede1b6b42188163a3f2e0f25ad5c0637eca564bd8df74d02e31a311dd6b37ad8',
    'lib/libfmt.a',
    [
        '-DBUILD_SHARED_LIBS=OFF',
        '-DFMT_DOC=OFF',
        '-DFMT_TEST=OFF',
    ],
    name='fmt',
    version='10.0.0',
    base='fmt-10.0.0',
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
    'http://zlib.net/zlib-1.2.13.tar.xz',
    'http://downloads.sourceforge.net/project/libpng/zlib/1.2.13/zlib-1.2.13.tar.xz',
    'd14c38e313afc35a9a8760dadf26042f51ea0f5d154b0630a31da0540107fb98',
    'lib/libz.a',
)

freetype = MesonProject(
    'http://download.savannah.gnu.org/releases/freetype/freetype-2.13.1.tar.xz',
    'http://downloads.sourceforge.net/project/freetype/freetype2/2.13.1/freetype-2.13.1.tar.xz',
    'ea67e3b019b1104d1667aa274f5dc307d8cbd606b399bc32df308a77f1a564bf',
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
    'https://c-ares.haxx.se/download/c-ares-1.18.1.tar.gz',
    'https://c-ares.haxx.se/download/c-ares-1.18.1.tar.gz',
    '1a7d52a8a84a9fbffb1be9133c0f6e17217d91ea5a6fa61f6b4729cda78ebbcf',
    'lib/libcares.a',
    [
        '-DCARES_STATIC=ON',
        '-DCARES_SHARED=OFF',
        '-DCARES_STATIC_PIC=ON',
        '-DCARES_BUILD_TOOLS=OFF',
    ],
)

curl = CmakeProject(
    'https://curl.se/download/curl-8.1.2.tar.xz',
    'https://github.com/curl/curl/releases/download/curl-8_1_2/curl-8.1.2.tar.xz',
    '31b1118eb8bfd43cd95d9a3f146f814ff874f6ed3999b29d94f4d1e7dbac5ef6',
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
        '-DCURL_USE_SCHANNEL=ON',
    ],
    patches=abspath('lib/curl/patches'),
)

# Needed by proj
sqlite3 = AutotoolsProject(
    'https://sqlite.org/2022/sqlite-autoconf-3390300.tar.gz',
    'https://fossies.org/linux/misc/sqlite-autoconf-3390300.tar.gz',
    '7868fb3082be3f2cf4491c6fba6de2bddcbc293a35fefb0624ee3c13f01422b9',
    'lib/libsqlite3.a',
    [
        '--disable-shared', '--enable-static',
    ],
    patches=abspath('lib/sqlite/patches'),
    autogen=True,
)

proj = CmakeProject(
    'http://download.osgeo.org/proj/proj-9.2.1.tar.gz',
    'https://fossies.org/linux/privat/proj-9.2.1.tar.gz',
    '15ebf4afa8744b9e6fccb5d571fc9f338dc3adcf99907d9e62d1af815d4971a1',
    'lib/libproj.a',
    [
        '-DBUILD_TESTING=OFF',
        '-DENABLE_TIFF=OFF',
        '-DENABLE_CURL=OFF',
        '-DBUILD_CCT=OFF',
        '-DBUILD_CS2CS=OFF',
        '-DBUILD_GEOD=OFF',
        '-DBUILD_GIE=OFF',
        '-DBUILD_PROJ=OFF',
        '-DBUILD_PROJINFO=OFF',
        '-DBUILD_PROJSYNC=OFF',
        '-DBUILD_SHARED_LIBS=OFF',
        '-DUSE_THREAD=OFF',
    ],
    patches=abspath('lib/proj/patches'),
)

libpng = CmakeProject(
    'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.40.tar.xz',
    'http://downloads.sourceforge.net/project/libpng/libpng16/1.6.40/libpng-1.6.40.tar.xz',
    '535b479b2467ff231a3ec6d92a525906fb8ef27978be4f66dbe05d3f3a01b3a1',
    'lib/libpng.a',
    [
        '-DPNG_SHARED=OFF',
        '-DPNG_TESTS=OFF',
    ],
    env={
        # unwind tables are needed for throwing C++ exceptions from C
        # error callbacks
        'CFLAGS': '-funwind-tables',
    },
)

libjpeg = CmakeProject(
    'http://downloads.sourceforge.net/project/libjpeg-turbo/2.1.5.1/libjpeg-turbo-2.1.5.1.tar.gz',
    'http://sourceforge.mirrorservice.org/l/li/libjpeg-turbo/2.1.5.1/libjpeg-turbo-2.1.5.1.tar.gz',
    '2fdc3feb6e9deb17adec9bafa3321419aa19f8f4e5dea7bf8486844ca22207bf',
    'lib/libjpeg.a',
    [
        '-DENABLE_STATIC=ON',
        '-DENABLE_SHARED=OFF',
    ],
    env={
        # unwind tables are needed for throwing C++ exceptions from C
        # error callbacks
        'CFLAGS': '-funwind-tables',
    },
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
    'http://download.osgeo.org/libtiff/tiff-4.5.1.tar.xz',
    'https://fossies.org/linux/misc/tiff-4.5.1.tar.xz',
    '3c080867114c26edab3129644a63b708028a90514b7fe3126e38e11d24f9f88a',
    'lib/libtiff.a',
    [
        '-DBUILD_SHARED_LIBS=OFF',
        '-Dtiff-tools=OFF',
        '-Dtiff-tests=OFF',
        '-Dtiff-contrib=OFF',
        '-Dtiff-docs=OFF',
        '-Dtiff-deprecated=OFF',
        '-Dtiff-install=ON',
        '-Dld-version-script=OFF',
        '-Dccitt=OFF',
        '-Dpackbits=OFF',
        '-Dlzw=OFF',
        '-Dthunder=OFF',
        '-Dnext=OFF',
        '-Dlogluv=OFF',
        '-Dmdi=OFF',
        '-Dpixarlog=OFF',
        '-DCMAKE_DISABLE_FIND_PACKAGE_Deflate=ON',
        '-DCMAKE_DISABLE_FIND_PACKAGE_JPEG=ON',
        '-DCMAKE_DISABLE_FIND_PACKAGE_JBIG=ON',
        '-DCMAKE_DISABLE_FIND_PACKAGE_liblzma=ON',
        '-DCMAKE_DISABLE_FIND_PACKAGE_ZSTD=ON',
        '-DCMAKE_DISABLE_FIND_PACKAGE_LERC=ON',
        '-DCMAKE_DISABLE_FIND_PACKAGE_WebP=ON',
        '-DCMAKE_DISABLE_FIND_PACKAGE_OpenGL=ON',
        '-DCMAKE_DISABLE_FIND_PACKAGE_GLUT=ON',
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
    'http://download.osgeo.org/geotiff/libgeotiff/libgeotiff-1.7.1.tar.gz',
    'https://fossies.org/linux/privat/libgeotiff-1.7.1.tar.gz',
    '05ab1347aaa471fc97347d8d4269ff0c00f30fa666d956baba37948ec87e55d6',
    'lib/libgeotiff.a',
    [
        '-DWITH_UTILITIES=OFF',
        '-DBUILD_SHARED_LIBS=OFF',
    ],
    patches=abspath('lib/libgeotiff/patches'),
)

sdl2 = SDL2Project(
    'http://www.libsdl.org/release/SDL2-2.0.22.tar.gz',
    'https://fossies.org/linux/misc/SDL2-2.0.22.tar.gz',
    'fe7cbf3127882e3fc7259a75a0cb585620272c51745d3852ab9dd87960697f2e',
    'lib/libSDL2.a',
    [
        '--disable-shared', '--enable-static',
        '--disable-joystick',
        '--disable-haptic',
        '--disable-hidapi',
        '--disable-sensor',
        '--disable-power',
        '--disable-filesystem',
        '--disable-threads',
        '--disable-timers',
        '--disable-file',
        '--disable-misc',
        '--disable-locale',
        '--disable-loadso',
        '--disable-cpuinfo',
        '--disable-oss',
        '--disable-jack',
        '--disable-esd',
        '--disable-arts',
        '--disable-nas',
        '--disable-sndio',
        '--disable-diskaudio',
        '--disable-dummyaudio',
        '--disable-libsamplerate',
        '--disable-video-dummy',
        '--disable-video-opengl',
        '--disable-video-opengles1',
        '--disable-sdl2-config',
    ],
    patches=abspath('lib/sdl2/patches'),
)

lua = LuaProject(
    'http://www.lua.org/ftp/lua-5.4.6.tar.gz',
    'https://fossies.org/linux/misc/lua-5.4.6.tar.gz',
    '7d5ea1b9cb6aa0b59ca3dde1c6adcb57ef83a1ba8e5432c0ecd06bf439b3ad88',
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
