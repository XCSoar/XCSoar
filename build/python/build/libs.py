from os.path import abspath

from build.zlib import ZlibProject
from build.autotools import AutotoolsProject
from build.freetype import FreeTypeProject
from build.libstdcxxmuslheaders import LibstdcxxMuslHeadersProject

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
    'https://ftp.gnu.org/gnu/gcc/gcc-8.3.0/gcc-8.3.0.tar.xz',
    'http://mirrors.ibiblio.org/gnu/ftp/gnu/gcc/gcc-8.3.0/gcc-8.3.0.tar.xz',
    '64baadfe6cc0f4947a84cb12d7f0dfaf45bb58b7e92461639596c21e02d97d2c',
    'include/libstdc++/algorithm',
    [
        '--enable-clocale=generic',
        '--disable-shared',
        '--disable-multilib',
    ],
    config_script='libstdc++-v3/configure',
    use_actual_arch=True,
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

curl = AutotoolsProject(
    'http://curl.haxx.se/download/curl-7.71.1.tar.xz',
    'https://github.com/curl/curl/releases/download/curl-7_71_1/curl-7.71.1.tar.xz',
    '40f83eda27cdbeb25cd4da48cefb639af1b9395d6026d2da1825bf059239658c',
    'lib/libcurl.a',
    [
        '--disable-shared', '--enable-static',
        '--disable-debug',
        '--disable-ares',
        '--enable-http',
        '--disable-ftp', '--disable-file',
        '--disable-ldap', '--disable-ldaps',
        '--disable-rtsp', '--disable-proxy', '--disable-dict', '--disable-telnet',
        '--disable-tftp', '--disable-pop3', '--disable-imap', '--disable-smb',
        '--disable-smtp',
        '--disable-gopher',
        '--disable-mqtt',
        '--disable-manual',
        '--enable-ipv6',
        '--disable-threaded-resolver', '--disable-verbose', '--disable-sspi',
        '--disable-crypto-auth', '--disable-ntlm-wb', '--disable-tls-srp', '--disable-cookies',
        '--disable-doh',
        '--disable-mime',
        '--disable-dateparse',
        '--disable-netrc',
        '--disable-progress-meter',
        '--disable-dnsshuffle',
        '--disable-alt-svc',
        '--without-brotli',
        '--without-ssl', '--without-gnutls',
        '--without-mbedtls', '--without-wolfssl',
        '--without-mesalink', '--without-bearssl',
        '--without-nss', '--without-libssh2',
        '--without-nghttp2', '--without-ngtcp2', '--without-nghttp3',
        '--without-quiche',
    ],
)

libpng = AutotoolsProject(
    'ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.37.tar.xz',
    'http://downloads.sourceforge.net/project/libpng/libpng16/1.6.37/libpng-1.6.37.tar.xz',
    '505e70834d35383537b6491e7ae8641f1a4bed1876dbfe361201fc80868d88ca',
    'lib/libpng.a',
    [
        '--disable-shared', '--enable-static',
        '--enable-arm-neon',
    ]
)

libjpeg = AutotoolsProject(
    'http://downloads.sourceforge.net/project/libjpeg-turbo/1.5.2/libjpeg-turbo-1.5.2.tar.gz',
    'http://sourceforge.mirrorservice.org/l/li/libjpeg-turbo/1.5.2/libjpeg-turbo-1.5.2.tar.gz',
    '9098943b270388727ae61de82adec73cf9f0dbb240b3bc8b172595ebf405b528',
    'lib/libjpeg.a',
    [
        '--disable-shared', '--enable-static',
        '--enable-arm-neon',
    ]
)
