cmake_minimum_required(VERSION 3.15)

set(INCLUDE_WITH_TOOLCHAIN 0)  # special include path for every toolchain!

prepare_3rdparty(lua lua)
if (_COMPLETE_INSTALL)
    set(CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}"
        "-DCMAKE_INSTALL_BINDIR=${_INSTALL_BIN}"
        "-DCMAKE_INSTALL_LIBDIR=${_INSTALL_LIB}"
        # "-DCMAKE_INSTALL_COMPONENT=bin/${TOOLCHAIN}"
        "-DCMAKE_INSTALL_INCLUDEDIR=include"
        "-DCMAKE_BUILD_TYPE=Release"
    )

    ExternalProject_Add(
        ${_BUILD_TARGET}
        GIT_REPOSITORY "https://github.com/lua/lua.git"
        GIT_TAG  v${LUA_VERSION}

        PREFIX  "${${TARGET_CNAME}_PREFIX}"
        ${_BINARY_STEP}
        INSTALL_DIR "${_INSTALL_DIR}"   # ${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}"

        PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECTGROUP_SOURCE_DIR}/3rd_party/${LIB_TARGET_NAME}_CMakeLists.txt.in" <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${CMAKE_ARGS}
        # INSTALL_COMMAND   cmake --build . --target install --config Release
        ${_INSTALL_COMMAND}
        BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
        BUILD_BYPRODUCTS  ${_TARGET_LIBS} # ${${TARGET_CNAME}_LIB}
    )
endif()
post_3rdparty()
