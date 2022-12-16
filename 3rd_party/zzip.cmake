set(DISPLAY_STRING "# ZZIP          # ZZIP          # ZZIP          # ZZIP          # ZZIP")
message(STATUS "${DISPLAY_STRING}")
cmake_minimum_required(VERSION 3.15)

set(LIB_TARGET_NAME                          zzip)
set(TARGET_NAME                              ${LIB_TARGET_NAME}_3rd)
#==========================================================
string(TOUPPER ${LIB_TARGET_NAME} TARGET_CNAME)

# defined in zlib.cmake: set(ZLIB_DIR ${LINK_LIBS}/zlib/${XCSOAR_ZLIB_VERSION})

# ---------------------------------------------------------------------------
option(USE_SYSTEM_${TARGET_CNAME} "Should we use the system ${LIB_TARGET_NAME}?" OFF)
option(USE_DRAHEIM "Should we use the draheim (or the BBDE) system?" OFF)

set(INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
#-------------------
  set(${TARGET_CNAME}_VERSION "xcsoar")    # ddebin
  
  set(XCSOAR_${TARGET_CNAME}_VERSION "${LIB_TARGET_NAME}-${${TARGET_CNAME}_VERSION}")  # reset!
  set(${TARGET_CNAME}_INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
  set(${TARGET_CNAME}_PREFIX "${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
  set(${TARGET_CNAME}_FILE "${THIRD_PARTY}/zzip/zzip-xcsoar.zip")
if(OFF)  # ${WITH_3RD_PARTY})
     set(${TARGET_CNAME}_URL "http://www.FlapsOnline.de/XCSoarAug/zzip-xcsoar.zip")
  ExternalProject_Add(
     ${LIB_TARGET_NAME}_3rd
     URL "${${TARGET_CNAME}_URL}"
     PREFIX  "${${TARGET_CNAME}_PREFIX}"
     # ${BINARY_STEP}
     BINARY_DIR    "${${TARGET_CNAME}_PREFIX}/build/${TOOLCHAIN}"
     INSTALL_DIR "${INSTALL_DIR}"
     # PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_CNAME}_CMakeLists.txt.in" <SOURCE_DIR>/CMakeLists.txt
     CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
         "-DCMAKE_INSTALL_BINDIR=bin/${TOOLCHAIN}"  #  :PATH=<INSTALL_DIR>/bin/${TOOLCHAIN}"
         "-DCMAKE_INSTALL_LIBDIR=lib/${TOOLCHAIN}"  # :PATH=<INSTALL_DIR>/lib/${TOOLCHAIN}"
         "-DCMAKE_INSTALL_COMPONENT=bin/${TOOLCHAIN}"  # :PATH=<INSTALL_DIR>/lib/${TOOLCHAIN}"
         "-DCMAKE_INSTALL_INCLUDEDIR=include"  #  :PATH=<INSTALL_DIR>/bin/${TOOLCHAIN}"

         "-DZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIR}"
         "-DZLIB_LIBRARY=${ZLIB_LIB}"

         "-DZLIB_INCLUDE_DIR=${ZLIB_DIR}/include"
         "-DZLIB_LIBRARY_DEBUG=${ZLIB_DIR}/lib/${TOOLCHAIN}/zlibstatic.lib"
         "-DZLIB_LIBRARY_RELEASE=${ZLIB_DIR}/lib/${TOOLCHAIN}/zlibstatic.lib"

    BUILD_ALWAYS ${EP_BUILD_ALWAYS}
    # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
    DEPENDS zlib
  )
  else()
    message(STATUS "!!! ZZIP-XCSOAR DON'T EXISTS !!!!!!!!!!!!!!!!!!!!!!!")
  endif()

set(${TARGET_CNAME}_LIB  "${INSTALL_DIR}/lib/${TOOLCHAIN}/${LIB_PREFIX}${LIB_TARGET_NAME}${LIB_SUFFIX}")
set(${TARGET_CNAME}_INCLUDE_DIR  "${INSTALL_DIR}/include")
# PARENT_SCOPE only available in Parent, not here...
if(EXISTS "${${TARGET_CNAME}_LIB}")
  set(${TARGET_CNAME}_LIB  ${${TARGET_CNAME}_LIB} PARENT_SCOPE)
else()
  set(${TARGET_CNAME}_LIB  ${LIB_TARGET_NAME} PARENT_SCOPE)
endif()
set(${TARGET_CNAME}_INCLUDE_DIR  ${${TARGET_CNAME}_INCLUDE_DIR} PARENT_SCOPE)

list(APPEND THIRDPARTY_INCLUDES ${${TARGET_CNAME}_INCLUDE_DIR})

list(APPEND 3RDPARTY_TARGETS ${LIB_TARGET_NAME})
set_target_properties(${LIB_TARGET_NAME} PROPERTIES FOLDER 3rdParty)  # ExternalProjectTargets)


