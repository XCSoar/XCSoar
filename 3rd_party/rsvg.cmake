cmake_minimum_required(VERSION 3.15)

set(INCLUDE_WITH_TOOLCHAIN 0)  # special include path for every toolchain!

## not functional: get_filename_component(_SUBPROJECT ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(_SUBPROJECT rsvg)
prepare_3rdparty(${_SUBPROJECT} ${_SUBPROJECT})
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
    GIT_REPOSITORY "https://gitlab.gnome.org/GNOME/librsvg.git"
    GIT_TAG  librsvg-2.55

    PREFIX  "${${TARGET_CNAME}_PREFIX}"

    BINARY_DIR            "${${TARGET_CNAME}_SOURCE_DIR}_build"

    # PATCH_COMMAND         "echo currDir = '%CD%' & dir"
    # PATCH_COMMAND         dir
    # CONFIGURE_COMMAND      "${_CONFIGURE_CMD}"
    CONFIGURE_COMMAND      ./configure  --host=x86_64-w64-mingw32 RUST_TARGET=x86_64-pc-windows-gnu  LIBS="-lws2_32 -luserenv"
    BUILD_COMMAND  ./make

    # BUILD_COMMAND cd /D ..\\.. & ./configure  --host=x86_64-w64-mingw32 RUST_TARGET=x86_64-pc-windows-gnu  LIBS="-lws2_32 -luserenv" & make
    # BUILD_COMMAND echo "currDir = %CD%" & dir
    # ${_BINARY_STEP}
    INSTALL_DIR "${_INSTALL_DIR}"   # ${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}"

    # PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECTGROUP_SOURCE_DIR}/3rd_party/${LIB_TARGET_NAME}_CMakeLists.txt.in" <SOURCE_DIR>/CMakeLists.txt
    CMAKE_ARGS ${CMAKE_ARGS}
    # INSTALL_COMMAND   cmake --build . --target install --config Release
    INSTALL_COMMAND ./configure  --host=x86_64-w64-mingw32 RUST_TARGET=x86_64-pc-windows-gnu  LIBS="-lws2_32 -luserenv" & make

    BUILD_ALWAYS ${EP_BUILD_ALWAYS}
    # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
    BUILD_BYPRODUCTS  ${_TARGET_LIBS} # ${${TARGET_CNAME}_LIB}
)

post_3rdparty()
