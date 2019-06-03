
import re


# Build and return a list of third-party libraries to be used by XCSoar

def libs_for_target (libs,toolchain, target):
    
    if 'mingw32' in toolchain.actual_arch:
        thirdparty_libs = [
            libs.zlib,
            libs.curl,
            libs.lua,
        ]
    elif re.match('(arm.*|aarch64)-apple-darwin', toolchain.actual_arch) is not None:
        thirdparty_libs = [
            libs.curl,
            libs.lua,
            libs.proj,
            libs.libtiff,
            libs.libgeotiff,
            libs.sdl2
        ]
    elif 'apple-darwin' in toolchain.actual_arch:
        thirdparty_libs = [
            libs.lua,
            libs.proj,
            libs.libtiff,
            libs.libgeotiff,
            libs.sdl2
        ]
    elif target == 'ANDROID':
        thirdparty_libs = [
            libs.curl,
            libs.lua,
            libs.proj,
            libs.libtiff,
            libs.libgeotiff,
        ]
    elif toolchain.toolchain_arch.endswith('-musleabihf'):
        thirdparty_libs = [
            libs.zlib,
            libs.freetype,
            libs.curl,
            libs.libpng,
            libs.libjpeg,
            libs.lua,
            libs.libsalsa,
            libs.libusb,
            libs.simple_usbmodeswitch,
        ]
    else:
        thirdparty_libs = [
            libs.musl,
            libs.libstdcxx_musl_headers,
            libs.zlib,
            libs.freetype,
            libs.curl,
            libs.libpng,
            libs.libjpeg,
            libs.lua,
            libs.libsalsa,
            libs.libusb,
            libs.simple_usbmodeswitch,
        ]

    return thirdparty_libs
