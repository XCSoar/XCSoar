from os.path import abspath

from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.meson import MesonProject
from build.cmake import CmakeProject
from build.openssl import OpenSSLProject
from build.gcc import BinutilsProject, GccProject, GccBootstrapProject
from build.linux import SabotageLinuxHeadersProject
from build.lua import LuaProject
from .musl import MuslProject

binutils = BinutilsProject(
    'https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz',
    'https://fossies.org/linux/misc/binutils-2.41.tar.xz',
    'ae9a5789e23459e59606e6714723f2d3ffc31c03174191ef0d015bdf06007450',
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
    'https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz',
    'https://fossies.org/linux/misc/gcc-13.2.0.tar.xz',
    'e275e76442a6067341a27f04c5c6b83d8613144004c0413528863dc6b5c743da',
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
    'https://www.openssl.org/source/openssl-3.1.2.tar.gz',
    'https://artfiles.org/openssl.org/source/openssl-3.1.2.tar.gz',
    'a0ce69b8b97ea6a35b96875235aa453b966ba3cba8af2de23657d8b6767d6539',
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
    'https://download.libsodium.org/libsodium/releases/libsodium-1.0.19.tar.gz',
    'https://github.com/jedisct1/libsodium/releases/download/1.0.19-RELEASE/libsodium-1.0.19.tar.gz',
    '018d79fe0a045cca07331d37bd0cb57b2e838c51bc48fd837a1472e50068bbea',
    'include/sodium/crypto_hash_sha256.h',
    [
        '--disable-shared', '--enable-static',
    ],

    # libsodium-1.0.19.tar.gz contains a weirdly named top-level directory
    base='libsodium-stable',
    name='libsodium',
    version='1.0.19',

    # suppress "visibility default" from sodium/export.h
    cppflags='-DSODIUM_STATIC'
)

zlib = ZlibProject(
    'http://zlib.net/zlib-1.3.tar.xz',
    'https://github.com/madler/zlib/releases/download/v1.3/zlib-1.3.tar.xz',
    '8a9ba2898e1d0d774eca6ba5b4627a11e5588ba85c8851336eb38de4683050a7',
    'lib/libz.a',
)

freetype = MesonProject(
    'http://download.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.xz',
    'http://downloads.sourceforge.net/project/freetype/freetype2/2.13.2/freetype-2.13.2.tar.xz',
    '2d8d5917a1983ebd04921f2993a88858d6f72dec',
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
    'https://curl.se/download/curl-8.2.1.tar.xz',
    'https://github.com/curl/curl/releases/download/curl-8_2_1/curl-8.2.1.tar.xz',
    'dd322f6bd0a20e6cebdfd388f69e98c3d183bed792cf4713c8a7ef498cba4894',
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
    'https://www.sqlite.org/2023/sqlite-autoconf-3420000.tar.gz',
    'https://fossies.org/linux/misc/sqlite-autoconf-3420000.tar.gz',
    '7abcfd161c6e2742ca5c6c0895d1f853c940f203304a0b49da4e1eca5d088ca6',
    'lib/libsqlite3.a',
    [
        '--disable-shared', '--enable-static',
        '--disable-fts5',
    ],
    patches=abspath('lib/sqlite/patches'),
    autogen=True,
)

proj = CmakeProject(
    'http://download.osgeo.org/proj/proj-9.3.0.tar.gz',
    'https://fossies.org/linux/privat/proj-9.3.0.tar.gz',
    'f1d70cb8873bb4429a03c437c65c41c4',
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
    env={
        # suppress "visibility default" from geodesic.h
        'CFLAGS': '-DGEOD_DLL=',
        'CXXFLAGS': '-DGEOD_DLL=',
    },
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
    'http://downloads.sourceforge.net/project/libjpeg-turbo/3.0.0/libjpeg-turbo-3.0.0.tar.gz',
    'https://netcologne.dl.sourceforge.net/project/libjpeg-turbo/3.0.0/libjpeg-turbo-3.0.0.tar.gz',
    'c77c65fcce3d33417b2e90432e7a0eb05f59a7fff884022a9d931775d583bfaa',
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

sdl2 = CmakeProject(
    'http://www.libsdl.org/release/SDL2-2.28.2.tar.gz',
    'https://fossies.org/linux/misc/SDL2-2.28.2.tar.gz',
    '64b1102fa22093515b02ef33dd8739dee1ba57e9dbba6a092942b8bbed1a1c5e',
    'lib/libSDL2.a',
    [
        '-DBUILD_SHARED_LIBS=OFF',

        '-DSDL_TEST=OFF',

        # subsystems
        '-DSDL_RENDER=OFF',
        '-DSDL_JOYSTICK=OFF',
        '-DSDL_HAPTIC=OFF',
        '-DSDL_HIDAPI=OFF',
        '-DSDL_POWER=OFF',
        '-DSDL_TIMERS=OFF',
        '-DSDL_FILE=OFF',
        '-DSDL_LOADSO=OFF',
        '-DSDL_CPUINFO=OFF',
        '-DSDL_FILESYSTEM=OFF',
        '-DSDL_SENSOR=OFF',
        '-DSDL_LOCALE=OFF',
        '-DSDL_MISC=OFF',

        '-DSDL2_DISABLE_SDL2MAIN=OFF',

        '-DSDL_DISKAUDIO=OFF',
        '-DSDL_DUMMYAUDIO=OFF',
        '-DSDL_DUMMYVIDEO=OFF',
        '-DSDL_OPENGL=OFF',
        '-DSDL_OPENGLES=ON',
        '-DSDL_OSS=OFF',
        '-DSDL_JACK=OFF',
        '-DSDL_ESD=OFF',
        '-DSDL_ARTS=OFF',
        '-DSDL_NAS=OFF',
        '-DSDL_SNDIO=OFF',
        '-DSDL_LIBSAMPLERATE=OFF',
        '-DSDL_COCOA=OFF',
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
