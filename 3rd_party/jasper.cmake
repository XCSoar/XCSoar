if (JASPER_OUTSIDE)
  # noch ohne jasper-package (jasper inside)
set(DISPLAY_STRING "# JASPER         # JASPER         # JASPER         # JASPER         # JASPER")
message(STATUS "${DISPLAY_STRING}")
cmake_minimum_required(VERSION 3.15)

set(LIB_TARGET_NAME                                   jasper)
#============================================================


string(TOUPPER ${LIB_TARGET_NAME} TARGET_CNAME)
# ---------------------------------------------------------------------------
option(USE_SYSTEM_${TARGET_CNAME} "Should we use the system ${LIB_TARGET_NAME}?" OFF)

set(_VERSION "${${TARGET_CNAME}_VERSION}")
set(XCSOAR_${TARGET_CNAME}_VERSION "${LIB_TARGET_NAME}-${_VERSION}")
set(${TARGET_CNAME}_INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
set(${TARGET_CNAME}_PREFIX "${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
#-------------------
set(INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
#-------------------
### if(NOT EXISTS "${INSTALL_DIR}")
# if(On)
if(${WITH_3RD_PARTY})

if (MSVC OR MINGW)
  set( BINARY_STEP BINARY_DIR "${${TARGET_CNAME}_PREFIX}/build/${TOOLCHAIN}")
else()
  set(BINARY_STEP   )
endif()
    ExternalProject_Add(
       ${LIB_TARGET_NAME}_3rd
       # URL "file://${THIRD_PARTY}/_Download/${XCSOAR_JASPER_VERSION}.tar.gz"
       GIT_REPOSITORY "https://github.com/mdadams/jasper.git"
       GIT_TAG "version-${${TARGET_CNAME}_VERSION}"           # git tag by jasper!
    
       PREFIX  "${${TARGET_CNAME}_PREFIX}"
       ${BINARY_STEP}
       # BINARY_DIR    "${${TARGET_CNAME}_PREFIX}/build/${TOOLCHAIN}"
       INSTALL_DIR "${INSTALL_DIR}"   # "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}"
    
       CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
           "-DCMAKE_INSTALL_BINDIR=bin/${TOOLCHAIN}"  #  :PATH=<INSTALL_DIR>/bin/${TOOLCHAIN}"
           "-DCMAKE_INSTALL_LIBDIR=lib/${TOOLCHAIN}"  # :PATH=<INSTALL_DIR>/lib/${TOOLCHAIN}"
    #       "-DCMAKE_INSTALL_COMPONENT=bin/${TOOLCHAIN}"  # :PATH=<INSTALL_DIR>/lib/${TOOLCHAIN}"
     
     "-DJAS_ENABLE_PROGRAMS=OFF"
           "-DJAS_ENABLE_SHARED=OFF"
           "-DJAS_ENABLE_OPENGL=OFF"
        BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
    )
endif()

set(LIB_NAME ${LIB_PREFIX}${LIB_TARGET_NAME})
# set(LIB_NAME lib${LIB_TARGET_NAME})

# TODO(aug): move this to a macro!!!
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
else(JASPER_OUTSIDE)
  set(JASPER_LIB  "jasper" PARENT_SCOPE)
endif(JASPER_OUTSIDE)