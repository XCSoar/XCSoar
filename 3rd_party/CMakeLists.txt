# if (ON)
  # 3rd-Party # 3rd-Party # 3rd-Party # 3rd-Party # 3rd-Party # 3rd-Party # 3rd-Party # 3rd-Party # 3rd-Party # 3rd-Party 
  cmake_minimum_required(VERSION 3.15)
  
  if (SHOW_SUBPROJECTS)
    message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CLANG ON)
  endif()

  include(3rd_party.cmake)
  
  # set(EP_BASE "${THIRD_PARTY}/external")  # later remove 'External!' # not used up to now!
  set(EP_CMAKE "${THIRD_PARTY}")   ##/cmake")  # later remove 'cmake!'
  # set(EP_BINARIES "${BINARY_DIR}/3rd_party")  # Binaries dahin verlagern?
  option(USE_SYSTEM_${TARGET_CNAME} "Should we use the system ${TARGET_NAME}?" OFF)
  option(EP_BUILD_ALWAYS "Build 3rd party packages always?" OFF)
  option(EP_BUILD_IN_SOURCE "Build 3rd party in sources (Recommended: OFF)?" OFF)


# file(GLOB_RECURSE SCRIPT_FILES *.txt *.cmake *.in)
# file(GLOB_RECURSE SCRIPT_FILES *.txt *.in)

include(CMakeSource.cmake)

set(EXTRA_BUILD 1)


#macro(prepare_3rdparty target_name) #  lib_name)
macro(prepare_3rdparty target_name lib_name)
    string(TOUPPER ${target_name} TARGET_CNAME)

    string(LENGTH ${TARGET_CNAME} _LENGTH)
    math(EXPR _LENGTH2 "15 - ${_LENGTH}")
    string(REPEAT " " ${_LENGTH2} _DIFF_STRING)
    set(_NEW_STRING "# ${TARGET_CNAME}${_DIFF_STRING}" )
    string(REPEAT ${_NEW_STRING} 6 _DISPLAY_STRING)
    message(STATUS "${_DISPLAY_STRING}")
    # message(STATUS "${_DISPLAY_STRING}  (${_LENGTH} vs ${_LENGTH2})")  # to check the length

    set(LIB_TARGET_NAME ${target_name})
    set(${TARGET_CNAME}_TARGET ${LIB_TARGET_NAME}_3rd)

    if(${EXTRA_BUILD})
       set(_BUILD_TARGET ${LIB_TARGET_NAME}_build)
    else()
       set(_BUILD_TARGET ${${TARGET_CNAME}_TARGET})
    endif()
    # ---------------------------------------------------------------------------
    option(USE_SYSTEM_${TARGET_CNAME} "Should we use the system ${LIB_TARGET_NAME}?" OFF)

    set(XCSOAR_${TARGET_CNAME}_VERSION "${LIB_TARGET_NAME}-${${TARGET_CNAME}_VERSION}")
    set(_INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
    set(${TARGET_CNAME}_INSTALL_DIR "${_INSTALL_DIR}")
    # set(${TARGET_CNAME}_INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
    set(${TARGET_CNAME}_PREFIX "${EP_CMAKE}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
    set(${TARGET_CNAME}_SOURCE_DIR  "${${TARGET_CNAME}_PREFIX}/src/${LIB_TARGET_NAME}")


    #-------------------
        if(INCLUDE_WITH_TOOLCHAIN)
        set(${TARGET_CNAME}_INCLUDE_DIR "${_INSTALL_DIR}/include/${TOOLCHAIN}")
    else()
        set(${TARGET_CNAME}_INCLUDE_DIR "${_INSTALL_DIR}/include")
    endif()

    # set(_INSTALL_DIR "${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}")
    # set(INSTALL_BIN "${_INSTALL_DIR}/bin/${TOOLCHAIN}")
    # set(INSTALL_LIB "${_INSTALL_DIR}/lib/${TOOLCHAIN}")
    set(_INSTALL_BIN "bin/${TOOLCHAIN}")
    set(_INSTALL_LIB "lib/${TOOLCHAIN}")

    if(${EXTRA_BUILD})
       set(_BUILD_TARGET ${LIB_TARGET_NAME}_build)
    else()
       set(_BUILD_TARGET ${${TARGET_CNAME}_TARGET})
       # string(REPLACE "\\" "/" _BUILD_TARGET  ${${TARGET_CNAME}_TARGET})
    endif()

    set(_TARGET_LIBS  "${_INSTALL_DIR}/lib/${TOOLCHAIN}/${LIB_PREFIX}${lib_name}${LIB_SUFFIX}")  ## D:/Project/link_libs...
    string(REPLACE "/" "\\" _INSTALL_DIR ${${TARGET_CNAME}_INSTALL_DIR})
    string(REPLACE "/" "\\" _SOURCE_DIR  ${${TARGET_CNAME}_SOURCE_DIR})
    string(REPLACE "\\" "/" _PREFIX_DIR  ${${TARGET_CNAME}_PREFIX})

    set(${TARGET_CNAME}_LIB  ${_TARGET_LIBS})  ## for internal use
if (${lib_name} MATCHES "fmt")
    set(_TARGET_LIBS  "${_INSTALL_DIR}/lib/${TOOLCHAIN}/${LIB_PREFIX}${lib_name}d${LIB_SUFFIX}")  ## D:/Project/link_libs...
    set(
      ${TARGET_CNAME}_LIB
      optimized "${_INSTALL_DIR}/lib/${TOOLCHAIN}/${LIB_PREFIX}${lib_name}${LIB_SUFFIX}"
      debug "${_INSTALL_DIR}/lib/${TOOLCHAIN}/${LIB_PREFIX}${lib_name}d${LIB_SUFFIX}"
      PARENT_SCOPE
    )   ## D:/Project/link_libs...
else()
    set(${TARGET_CNAME}_LIB  ${_TARGET_LIBS} PARENT_SCOPE)  ## D:/Project/link_libs...
endif()
    set(${TARGET_CNAME}_TARGET  ${${TARGET_CNAME}_TARGET} PARENT_SCOPE) # f.e. zlib_3rd, sodium_3rd

if (MSVC OR MINGW OR CLANG)
  set( _BINARY_STEP BINARY_DIR ${${TARGET_CNAME}_PREFIX}/build/${TOOLCHAIN})
else()
  set( _BINARY_STEP   )
  string(REPLACE "\\" "/" _INSTALL_DIR ${${TARGET_CNAME}_INSTALL_DIR})
#  message(FATAL_ERROR "_INSTALL_DIR = ${_INSTALL_DIR}")
endif()

    set(_INSTALL_COMMAND INSTALL_COMMAND cmake --build . --target install --config Release)

if(EXISTS ${_TARGET_LIBS})  ## for internal use
    set(_COMPLETE_INSTALL OFF)
else()
    set(_COMPLETE_INSTALL ON)
endif()

endmacro()


macro(post_3rdparty)
    if(${EXTRA_BUILD})
        # ExternalProject_Get_Property(${_BUILD_TARGET} _INSTALL_DIR)
            add_library(${${TARGET_CNAME}_TARGET} STATIC IMPORTED GLOBAL)
        # set_source_files_properties(${${TARGET_CNAME}_LIB} PROPERTIES GENERATED TRUE)
        # set_target_properties(${${TARGET_CNAME}_TARGET} PROPERTIES IMPORTED_LOCATION  ${${TARGET_CNAME}_LIB}  GENERATED TRUE)
        set_target_properties(${${TARGET_CNAME}_TARGET} PROPERTIES IMPORTED_LOCATION  ${_TARGET_LIBS}  GENERATED TRUE)
        set_target_properties(${${TARGET_CNAME}_TARGET} PROPERTIES FOLDER 3rdParty)
        add_dependencies(${${TARGET_CNAME}_TARGET}  ${_BUILD_TARGET})
    endif()

if (_COMPLETE_INSTALL)
    set_target_properties(${_BUILD_TARGET} PROPERTIES FOLDER 3rdParty) # either zlib_build or zlib_3rd
endif()
    set(${TARGET_CNAME}_TARGET_LIBS  ${_TARGET_LIBS} PARENT_SCOPE)
    set(${TARGET_CNAME}_INCLUDE_DIR  ${${TARGET_CNAME}_INCLUDE_DIR} PARENT_SCOPE)
    list(APPEND THIRDPARTY_INCLUDES ${${TARGET_CNAME}_INCLUDE_DIR})

    # list(APPEND 3RDPARTY_TARGETS ${LIB_TARGET_NAME})
    list(APPEND 3RDPARTY_TARGETS ${${TARGET_CNAME}_TARGET})  # zlib_3rd
endmacro()

    foreach(cmake_file ${CMAKE_FILES})
      include(${cmake_file})
    endforeach()
  
    #this makes the THIRDPARTY_INCLUDES visible to outside in general:
    set(THIRDPARTY_INCLUDES ${THIRDPARTY_INCLUDES} PARENT_SCOPE)

    source_group("Scripts" FILES  ${SCRIPT_FILES} )
    source_group("Targets" FILES  ${CMAKE_FILES})

    ## message(FATAL_ERROR "SCRIPT_FILES = ${SCRIPT_FILES}")
    add_custom_target(ThirdParty
        # EXCLUDE_FROM_ALL
        SOURCES
        ${CMAKE_FILES}
        ${SCRIPT_FILES}
    )

    set(3RDPARTY_TARGETS ${3RDPARTY_TARGETS} PARENT_SCOPE)

    if (DISPLAY_TARGETS)
        foreach(_target ${3RDPARTY_TARGETS} ) 
           message(STATUS "### 3RDPARTY_TARGETS = ${_target}")
        endforeach()
    # message(FATAL_ERROR "SCRIPT_FILES = ${SCRIPT_FILES}")
    endif()
    
set_target_properties(ThirdParty PROPERTIES FOLDER 3rdParty)
