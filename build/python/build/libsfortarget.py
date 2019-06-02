
import re

from build.libs import *

# Build and return a list of third-party libraries to be used by XCSoar

def libs_for_target (actual_host_triplet,target,toolchain_host_triplet):
    
    if 'mingw32' in actual_host_triplet:
        thirdparty_libs = [
            zlib,
            curl,
            lua,
        ]
    elif re.match('(arm.*|aarch64)-apple-darwin', actual_host_triplet) is not None:
        thirdparty_libs = [
            curl,
            lua,
            proj,
            libtiff,
            libgeotiff,
            sdl2
        ]
    elif 'apple-darwin' in actual_host_triplet:
        thirdparty_libs = [
            lua,
            proj,
            libtiff,
            libgeotiff,
            sdl2
        ]
    elif target == 'ANDROID':
        thirdparty_libs = [
            curl,
            lua,
            proj,
            libtiff,
            libgeotiff,
        ]
    elif toolchain_host_triplet.endswith('-musleabihf'):
        thirdparty_libs = [
            zlib,
            freetype,
            curl,
            libpng,
            libjpeg,
            lua,
            libsalsa,
            libusb,
            simple_usbmodeswitch,
        ]
    else:
        thirdparty_libs = [
            musl,
            libstdcxx_musl_headers,
            zlib,
            freetype,
            curl,
            libpng,
            libjpeg,
            lua,
            libsalsa,
            libusb,
            simple_usbmodeswitch,
        ]

    return thirdparty_libs
