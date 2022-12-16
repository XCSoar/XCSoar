if (OFF)
  # without XML-Parser yet

set(DISPLAY_STRING "# XML-PARSER     # XML-PARSER     # XML-PARSER     # XML-PARSER     # XML-PARSER")
message(STATUS "${DISPLAY_STRING}")
cmake_minimum_required(VERSION 3.15)

set(LIB_TARGET_NAME  xmlparser)
#==========================================================
string(TOUPPER ${LIB_TARGET_NAME} TARGET_CNAME)

# ---------------------------------------------------------------------------
option(USE_SYSTEM_${TARGET_CNAME} "Should we use the system ${LIB_TARGET_NAME}?" OFF)

# set(${TARGET_CNAME}_VERSION "2.44")
set(${TARGET_CNAME}_VERSION "1.08")
set(XCSOAR_${TARGET_CNAME}_VERSION "${LIB_TARGET_NAME}-${${TARGET_CNAME}_VERSION}")  # reset!
set(${TARGET_CNAME}_INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
set(${TARGET_CNAME}_PREFIX "${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
set(${TARGET_CNAME}_URL "http://www.FlapsOnline.de/XCSoarAug/xml-1.08.zip")

#-------------------
if(${WITH_3RD_PARTY})
if (MSVC OR MINGW)
  set( BINARY_STEP BINARY_DIR "${${TARGET_CNAME}_PREFIX}/build/${TOOLCHAIN}")
else()
  set(BINARY_STEP   )
endif()
ExternalProject_Add(
    ${LIB_TARGET_NAME}
    # GIT_REPOSITORY "https://github.com/"
    # GIT_TAG "v${${TARGET_CNAME}_VERSION}"

    URL "${${TARGET_CNAME}_URL}"
    PREFIX  "${${TARGET_CNAME}_PREFIX}"
    ${BINARY_STEP}
    # BINARY_DIR    "${${TARGET_CNAME}_PREFIX}/build/${TOOLCHAIN}"
    INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}"
   PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_CNAME}_CMakeLists.txt.in" <SOURCE_DIR>/CMakeLists.txt
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
     "-DINSTALL_LIB_DIR:PATH=<INSTALL_DIR>/lib/${TOOLCHAIN}"
     "-DINSTALL_LIB_DIR:PATH=<INSTALL_DIR>/lib/${TOOLCHAIN}"
     "-DCMAKE_INSTALL_INCLUDEDIR=include"
        INSTALL_COMMAND   cmake --build . --target install --config Release
        BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
)
endif()
set(${TARGET_CNAME}_LIB  "${INSTALL_DIR}/lib/${TOOLCHAIN}/${LIB_NAME}${LIB_SUFFIX}")
set(${TARGET_CNAME}_INCLUDE_DIR  "${INSTALL_DIR}/include")
# PARENT_SCOPE only available in Parent, not here...
if(EXISTS "${${TARGET_CNAME}_LIB}")
  set(${TARGET_CNAME}_LIB  ${${TARGET_CNAME}_LIB} PARENT_SCOPE)
else()
  set(${TARGET_CNAME}_LIB  ${LIB_TARGET_NAME} PARENT_SCOPE)
endif()
set(${TARGET_CNAME}_INCLUDE_DIR  ${${TARGET_CNAME}_INCLUDE_DIR} PARENT_SCOPE)

list(APPEND THIRDPARTY_INCLUDES ${${TARGET_CNAME}_INCLUDE_DIR})

message(FATAL_ERROR "xmlparser: ${TARGET_CNAME}  ${INSTALL_DIR}/include  => ${${TARGET_CNAME}_INCLUDE_DIR}")
endif()
