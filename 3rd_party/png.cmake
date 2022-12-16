cmake_minimum_required(VERSION 3.15)
# get_filename_component(LIB_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

 set(INCLUDE_WITH_TOOLCHAIN 0)  # special include path for every toolchain!
if (MSVC)  # unfortunately the lib name is a little bit 'tricky' at libPng..
  set(_LIB_NAME libpng16_static)
  # message(FATAL_ERROR LibPng: MSVC')
 else()
  set(_LIB_NAME png16)
endif()

prepare_3rdparty(png ${_LIB_NAME})
if (_COMPLETE_INSTALL)

    # Aug: Wieso ist das nicht besetzt????
    # set(ZLIB_LIB D:/Projects/link_libs/zlib/zlib-1.2.12/lib/clang14/libzlibstatic.a) 
    # message(FATAL_ERROR "Png-Stop: ${ZLIB_LIB} /// ${ZLIB_INCLUDE_DIR}")
    set(CMAKE_ARGS
             "-DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}"
             "-DCMAKE_INSTALL_LIBDIR=${_INSTALL_LIB}"
            "-DCMAKE_INSTALL_INCLUDEDIR=include"
            "-DCMAKE_BUILD_TYPE=Release"
  
            "-DPNG_ZLIB_BUILD=OFF"
            "-DZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIR}"
            "-DZLIB_LIBRARY=${ZLIB_LIB}"

            "-DPNG_SHARED=OFF"
            "-DPNG_STATIC=ON"
            "-DPNG_TESTS=OFF"
    )

    ExternalProject_Add(
        ${_BUILD_TARGET}
        GIT_REPOSITORY "https://github.com/glennrp/libpng.git"
        # GIT_REPOSITORY "https://github.com/libpng/libpng.git" # alternateive?
        GIT_TAG "v${${TARGET_CNAME}_VERSION}"           # git tag by libpng!
  
        PREFIX  "${${TARGET_CNAME}_PREFIX}"
        ${_BINARY_STEP}
        INSTALL_DIR "${_INSTALL_DIR}"
  
        # PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/LIBPNG/CMakeLists.txt.in" <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${CMAKE_ARGS}
        # INSTALL_COMMAND   cmake --build . --target install --config Release
        ${_INSTALL_COMMAND}
  
        BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
        DEPENDS ${ZLIB_TARGET}
     
        BUILD_BYPRODUCTS  ${_TARGET_LIBS} # ${${TARGET_CNAME}_LIB}
    )
endif()
post_3rdparty()
