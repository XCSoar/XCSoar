cmake_minimum_required(VERSION 3.15)

set(INCLUDE_WITH_TOOLCHAIN 0)  # special include path for every toolchain!
if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  set(_LIB_NAME zlibstatic)
else()
  set(_LIB_NAME z)
endif()

prepare_3rdparty(zlib ${_LIB_NAME})

if (_COMPLETE_INSTALL)
    #----------------------
    set(CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}"
        "-DINSTALL_BIN_DIR:PATH=${_INSTALL_DIR}/${_INSTALL_BIN}"
        "-DINSTALL_LIB_DIR:PATH=${_INSTALL_DIR}/${_INSTALL_LIB}"
        # ??? "-DCMAKE_CONFIGURATION_TYPES=Release"
        "-DCMAKE_BUILD_TYPE=Release"

        "-DZ_HAVE_UNISTD_H=OFF"  # MSVC only!!!!
    )
    # message(FATAL_ERROR "+++ BINARY_STEP (${TARGET_CNAME}): ${_BINARY_STEP}")

    ExternalProject_Add(
        ${_BUILD_TARGET}
        GIT_REPOSITORY "https://github.com/madler/zlib.git"
        GIT_TAG "v${${TARGET_CNAME}_VERSION}"

        PREFIX  "${${TARGET_CNAME}_PREFIX}"
        ${_BINARY_STEP}
        INSTALL_DIR "${_INSTALL_DIR}"
        CMAKE_ARGS ${CMAKE_ARGS}
 
        # BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        # INSTALL_COMMAND   cmake --build . --target install --config Release
        ${_INSTALL_COMMAND}
        BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        CONFIGURE_HANDLED_BY_BUILD  ON
        # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
        BUILD_BYPRODUCTS  ${_TARGET_LIBS} # important for ninja!
    )
endif()

post_3rdparty()
