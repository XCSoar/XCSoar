set(DISPLAY_STRING "# BOOST          # BOOST          # BOOST          # BOOST          # BOOST")
message(STATUS "${DISPLAY_STRING}")
cmake_minimum_required(VERSION 3.10)

set(INCLUDE_WITH_TOOLCHAIN 0)  # special include path for every toolchain!

set(LIB_TARGET_NAME                                  boost)
#==========================================================
string(TOUPPER ${LIB_TARGET_NAME} TARGET_CNAME)

# check correct link_libs_path on Flaps6
if(NOT ${LINK_LIBS} MATCHES "link_libs")
    message(STATUS "LINK_LIBS = ${LINK_LIBS}")
    message(FATAL_ERROR "Stop!") 
endif()

# BOOST_VERSION  is defined in 3rd_Party.cmake
set(XCSOAR_BOOST_VERSION "${LIB_TARGET_NAME}-${BOOST_VERSION}")
string(FIND ${BOOST_VERSION} "." BOOST_VERSION_LEN REVERSE )  # search for the last dot!
string(SUBSTRING ${BOOST_VERSION} 0 ${BOOST_VERSION_LEN} BOOST_SHORTVERSION)

string(REPLACE "." "_" _BOOST_SHORTVERSION ${BOOST_SHORTVERSION})  # defined in 3rd_Party.cmake
message(STATUS "### BOOST_VERSION = ${BOOST_VERSION} ||| short(1): ${BOOST_SHORTVERSION} ||| short(2): ${_BOOST_SHORTVERSION} ")
# message(FATAL_ERROR "Stop!") 

set(BOOST_ROOT "${LINK_LIBS}/boost/boost-${BOOST_VERSION}") 

#=======================================
# TOOLCHAIN is defined from CMAKE caller
if(${TOOLCHAIN} MATCHES "msvc")
    set(_TOOLSET msvc)
    if(${TOOLCHAIN} MATCHES "msvc2022")
        set(Boost_COMPILER "-vc143")  # msvc2022
    elseif(${TOOLCHAIN} MATCHES "msvc2019")
        set(Boost_COMPILER "-vc142")  # msvc2019
    else()
    endif()
elseif((${TOOLCHAIN} MATCHES "mgw") OR (${TOOLCHAIN} MATCHES "unix"))
    set(_TOOLSET gcc)
    if(${TOOLCHAIN} MATCHES "mgw122")
        set(Boost_COMPILER "-mgw12")
    elseif(${TOOLCHAIN} MATCHES "mgw112")
        set(Boost_COMPILER "-mgw11")
    elseif(${TOOLCHAIN} MATCHES "mgw103")
        set(Boost_COMPILER "-mgw10")
    else()
        set(Boost_COMPILER "-gcc11")  # TODO(August2111): meke this real!
    endif()
elseif(${TOOLCHAIN} MATCHES "clang")
      set(_TOOLSET clang)
      set(Boost_COMPILER "-clang")
else()
   message(FATAL_ERROR "+++ Unknown ToolChain: '${TOOLCHAIN}'!")
endif()
message(STATUS "Boost_COMPILER = ${Boost_COMPILER}")
#=======================================


set(Boost_USE_STATIC_LIBS         ON)
set(Boost_USE_MULTITHREADED       ON)
set(Boost_USE_STATIC_RUNTIME      OFF)
set(Boost_USE_RELEASE_LIBS        ON)
set(Boost_RELEASE_ABI_TAG        "")
set(Boost_USE_DEBUG_LIBS          OFF)
set(Boost_DEBUG                   OFF) # Debugging more info in the find_package-process!
set(BOOST_LIBRARYDIR              "${BOOST_ROOT}/lib/${TOOLCHAIN}")
set(BOOST_COMPONENTS system regex filesystem thread chrono date_time) # headers) 
if(NOT ${TOOLCHAIN} MATCHES "mgw122")
    list(APPEND BOOST_COMPONENTS json)  # headers) 
endif()

set(Boost_INCLUDE_DIR  "${BOOST_ROOT}/include/boost-${_BOOST_SHORTVERSION}")

if (BOOST_COMPONENTS)
### 2022-09-01 ???  find_package(Boost ${BOOST_VERSION} COMPONENTS ${BOOST_COMPONENTS})
else()
  find_package(Boost ${BOOST_SHORTVERSION}) # REQUIRED)
endif()

if(Boost_FOUND)
  message(Status "+++ Boost found! ${Boost_INCLUDE_DIR}") 
else()
  message(STATUS "Boost NOT found!") 
  # message(FATAL_ERROR "Stop: Boost missing!") 
endif()

set(Boost_DIR "${BOOST_ROOT}/lib/${Boost_COMPILER}/cmake/Boost-${BOOST_VERSION}")
## 2022-09-02:  set(Boost_INCLUDE_DIR  "${BOOST_ROOT}/include/boost-${_BOOST_SHORTVERSION}")

if(EXISTS "${Boost_INCLUDE_DIR}/boost/version.hpp")
    set(BOOST_IS_READY ON)
else()
    # message(FATAL_ERROR "!!! Boost: include directories not found! (BOOST_ROOT = ${BOOST_ROOT})")
    set(BOOST_IS_READY OFF)
endif()

# ---------------------------------------------------------------------------
option(USE_SYSTEM_BOOST "Should we use the system ${LIB_TARGET_NAME}?" OFF)

set(BOOST_BUILD_COMPONENTS)
set(BOOST_BUILD_TEST)
foreach(component  ${BOOST_COMPONENTS} headers)
  list(APPEND BOOST_BUILD_COMPONENTS --with-${component})
  string(APPEND BOOST_BUILD_TEST "--with-${component} ")
#  list(APPEND BOOST_BUILD_COMPONENTS ${}
endforeach()
message(STATUS "XXX ${BOOST_BUILD_COMPONENTS}")
message(STATUS "XXX ${BOOST_BUILD_TEST}")

## see above: set(${TARGET_CNAME}_VERSION "1.77.0")
set(BOOST_INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_BOOST_VERSION}")
set(BOOST_PREFIX "${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_BOOST_VERSION}")
# set(BOOST_BUILD_DIR "${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_BOOST_VERSION}/build")

set(_SRC_DIR ${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_BOOST_VERSION}/src/${LIB_TARGET_NAME})  # = src_dir
if(EXISTS ${_SRC_DIR})
    message(STATUS "_SRC_DIR = ${_SRC_DIR}")
else()
    message(STATUS "_SRC_DIR = '${_SRC_DIR}' doesn't exist!")
###    message(FATAL_ERROR "Stop!") 
endif()

# variant=release 
set(_BUILD_CMD ./b2 -j4 toolset=${_TOOLSET} link=static runtime-link=shared threading=multi address-model=64 --layout=versioned --prefix=${BOOST_INSTALL_DIR} --build-dir=${BOOST_PREFIX}/build/${TOOLCHAIN} ${BOOST_BUILD_COMPONENTS} --includedir=${BOOST_INSTALL_DIR}/include --libdir=${BOOST_INSTALL_DIR}/lib/${TOOLCHAIN} install)

set(_INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_BOOST_VERSION}")

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  string(REPLACE "/" "\\" _SRC_DIR ${_SRC_DIR})
  set(_CONFIGURE_CMD if not exist b2.exe call bootstrap.bat)
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
  set(_CONFIGURE_CMD [[ -f "${_SRC_DIR}/b2" ]] && echo "b2 exists." || ./bootstrap.sh)
endif()


if (ON)
# if (MSVC)  # in this moment not for MinGW enabled!!
#===========================================
#-------------------
# if(NOT EXISTS "${_INSTALL_DIR}")

# if(OFF)

# if(NOT Boost_FOUND)
if(${WITH_3RD_PARTY})  # ON)

message(STATUS "### ${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_BOOST_VERSION}/src/${LIB_TARGET_NAME}")
message(STATUS "### Build-Command: ${_BUILD_CMD}")

#-------------------
ExternalProject_Add(
   ${LIB_TARGET_NAME}
   GIT_REPOSITORY        "https://github.com/boostorg/boost.git"
   GIT_TAG               "${XCSOAR_BOOST_VERSION}"

   PREFIX                "${BOOST_PREFIX}"
   INSTALL_DIR           "${_INSTALL_DIR}"
   # BINARY_DIR            "${BOOST_PREFIX}/build/${TOOLCHAIN}"
   BINARY_DIR            "${_SRC_DIR}"

   # PATCH_COMMAND         "bootstrap"
   CONFIGURE_COMMAND      "${_CONFIGURE_CMD}"
   # 29.08.2022 herausgenommen:  
#   CONFIGURE_COMMAND     bootstrap     ##  "${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_BOOST_VERSION}/src/${LIB_TARGET_NAME}/TestOutput.cmd"

   BUILD_COMMAND ${_BUILD_CMD}
   INSTALL_COMMAND echo Install done
    BUILD_ALWAYS                OFF
    CONFIGURE_HANDLED_BY_BUILD  ON
    # BUILD_IN_SOURCE   # ${EP_BUILD_IN_SOURCE}
    WORKING_DIRECTORY  ${_SRC_DIR}
        BUILD_BYPRODUCTS  ${${TARGET_CNAME}_LIB}
        EXCLUDE_FROM_ALL ON
        EXCLUDE_FROM_DEFAULT_BUILD ON
        UPDATE_COMMAND ""
)
#===========================================
set_target_properties(${LIB_TARGET_NAME} PROPERTIES FOLDER 3rdParty)  # ExternalProjectTargets)

endif() # if not exist

endif()  # MSVC
## 2022-09-02: set(BOOST_LIB  "${_INSTALL_DIR}/lib/${TOOLCHAIN}/${LIB_PREFIX}${LIB_TARGET_NAME}${LIB_SUFFIX}")
## 2022-09-02: set(BOOST_INCLUDE_DIR  "${_INSTALL_DIR}/include")   # 29.06.2022: das ist falsch!
## 2022-09-02: set(BOOST_INCLUDE_DIR  ${Boost_INCLUDE_DIR})  # 29.06.2022: ...und das richtig?
# PARENT_SCOPE only available in Parent, not here...
if(EXISTS "${BOOST_LIB}")
  set(BOOST_LIB  ${BOOST_LIB} PARENT_SCOPE)
else()
  set(BOOST_LIB  ${LIB_TARGET_NAME} PARENT_SCOPE)
endif()
## 2022-09-02: set(BOOST_INCLUDE_DIR  ${BOOST_INCLUDE_DIR} PARENT_SCOPE)
set(Boost_INCLUDE_DIR  ${Boost_INCLUDE_DIR} PARENT_SCOPE)  # 29.06.2022: ...und das richtig?

# set(THIRDPARTY_INCLUDES ${THIRDPARTY_INCLUDES} ${Boost_INCLUDE_DIR} PARENT_SCOPE)
list(APPEND THIRDPARTY_INCLUDES ${Boost_INCLUDE_DIR})
set(BOOST_ROOT ${BOOST_ROOT} PARENT_SCOPE)

# link_directories(${BOOST_ROOT}/lib/${TOOLCHAIN} PARENT_SCOPE)  # aug: not affected in main project..

if (MSVC OR MINGW OR CLANG)  # in this moment not for MinGW enabled!!
   message(STATUS "Boost: BOOST_ROOT = ${BOOST_ROOT} /// Boost_DIR = ${Boost_DIR}")
elseif(UNIX)
   set(Boost_DIR "$ENV{HOME}/Projects/link_libs/boost/boost-${BOOST_VERSION}/lib/unix/cmake/Boost-${BOOST_VERSION}") 
else()
   message(FATAL_ERROR "BOOST-Stop: ${BOOST_ROOT} /// ${Boost_DIR}  ")
endif()

set(BOOST_TARGET ${LIB_TARGET_NAME} PARENT_SCOPE)
list(APPEND 3RDPARTY_TARGETS ${LIB_TARGET_NAME})  # boost
