cmake_minimum_required(VERSION 3.15)
# get_filename_component(LIB_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
set(INCLUDE_WITH_TOOLCHAIN 0)  # special include path for every toolchain!
# set(_LIB_NAME libfmt) # "liblibfmt.a"
###     debug fmtd optimized fmt
set(_LIB_NAME fmt)   # "liblibfmt.a"
## set(_LIB_NAME fmtd)   # "liblibfmt.a"
Prepare_3rdParty(fmt ${_LIB_NAME})


string(REPLACE "/" "\\" _INSTALL_DIR ${FMT_INSTALL_DIR})
string(REPLACE "/" "\\" _SOURCE_DIR  ${FMT_PREFIX}/src/${LIB_TARGET_NAME})

if (WIN32 AND MSVC)
      set(BUILD_COMMAND devenv ${_SOURCE_DIR}\\libfmt.sln /Build Release|x64)
      set(CONFIGURE_COMMAND  echo ${BUILD_COMMAND})
      # set(INSTALL_COMMAND xcopy ${_SOURCE_DIR}\\Build\\Release\\x64\\*.lib ${_INSTALL_DIR}\\lib\\${TOOLCHAIN}\\* /Y /T)
      set(INSTALL_COMMAND xcopy ${_SOURCE_DIR}\\src\\libfmt\\include\\*.h ${_INSTALL_DIR}\\include\\* /Y /I /S /E)
elseif (WIN32 AND MINGW AND NOT CLANG)
      find_program(MAKE_EXECUTABLE NAMES gmake make mingw32-make REQUIRED)
endif()

if (_COMPLETE_INSTALL)
    set(CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}"
        "-DCMAKE_INSTALL_BINDIR=${_INSTALL_BIN}"
        "-DCMAKE_INSTALL_LIBDIR=${_INSTALL_LIB}"
        # "-DCMAKE_INSTALL_COMPONENT=bin/${TOOLCHAIN}"
        "-DCMAKE_INSTALL_INCLUDEDIR=include"
        "-DCMAKE_BUILD_TYPE=Release"
    )

    # message(FATAL_ERROR "+++ BINARY_STEP (${TARGET_CNAME}): ${_BINARY_STEP}")

    ExternalProject_Add(
       ${_BUILD_TARGET}

        GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
        GIT_TAG  ${FMT_VERSION}

        PREFIX  "${${TARGET_CNAME}_PREFIX}"
        ${_BINARY_STEP}
        INSTALL_DIR "${_INSTALL_DIR}"   # ${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}"

        # PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECTGROUP_SOURCE_DIR}/3rd_party/fmt_CMakeLists.txt.in" <SOURCE_DIR>/CMakeLists.txt
        CMAKE_ARGS ${CMAKE_ARGS}
        # INSTALL_COMMAND   cmake --build . --target install --config Release
        ${_INSTALL_COMMAND}
        BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
        BUILD_BYPRODUCTS  ${_TARGET_LIBS} # ${${TARGET_CNAME}_LIB}
    )
endif()

post_3rdparty()
