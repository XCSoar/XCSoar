cmake_minimum_required(VERSION 3.15)

set(INCLUDE_WITH_TOOLCHAIN 1)  # special include path for every toolchain!
prepare_3rdparty(cares cares)

if (_COMPLETE_INSTALL)
    set( CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}"
        "-DCMAKE_INSTALL_BINDIR=${_INSTALL_BIN}"
        "-DCMAKE_INSTALL_LIBDIR=${_INSTALL_LIB}"
        # "-DCMAKE_INSTALL_COMPONENT=bin/${TOOLCHAIN}"
        "-DCMAKE_INSTALL_INCLUDEDIR=include/${TOOLCHAIN}"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCARES_SHARED=OFF"
        "-DCARES_STATIC=ON"
        "-DCARES_STATIC_PIC=ON"
        "-DCARES_BUILD_TESTS=OFF" )

    string(REPLACE "." "_" GIT_TAG cares-${${TARGET_CNAME}_VERSION})  # after 1.17.1 only 'cares', before c-ares!

    ExternalProject_Add(
          ${_BUILD_TARGET}
       GIT_REPOSITORY "https://github.com/c-ares/c-ares.git"
       GIT_TAG  ${GIT_TAG}  # cares-1_1_17 -> for this use string(REPLACE...)
        PREFIX  "${${TARGET_CNAME}_PREFIX}"
        ${_BINARY_STEP}
        INSTALL_DIR "${_INSTALL_DIR}"  # ${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}"
        # PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/CURL_CMakeLists.txt.in" <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${CMAKE_ARGS}
        # INSTALL_COMMAND   cmake --build . --target install --config Release
        ${_INSTALL_COMMAND}
        BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
        DEPENDS  ${ZLIB_TARGET}  # !!!!
        BUILD_BYPRODUCTS  ${_TARGET_LIBS} # ${${TARGET_CNAME}_LIB}
    )
endif()

post_3rdparty()

if (_COMPLETE_INSTALL)
    add_dependencies(${_BUILD_TARGET}  ${ZLIB_TARGET})
endif()
