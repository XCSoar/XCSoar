cmake_minimum_required(VERSION 3.15)
  
message(STATUS "=== Build-Scripts  ======================")
set(TARGET_NAME Scripts)

file(GLOB_RECURSE SCRIPT_FILES
    ${PROJECTGROUP_SOURCE_DIR}/src/*.txt
    ${PROJECTGROUP_SOURCE_DIR}/src/*.cmake
    ${PROJECTGROUP_SOURCE_DIR}/src/*.in
    ${PROJECTGROUP_SOURCE_DIR}/build/*.txt
    ${PROJECTGROUP_SOURCE_DIR}/build/*.cmake
    ${PROJECTGROUP_SOURCE_DIR}/build/*.toolchain
    # ${PROJECTGROUP_SOURCE_DIR}/build/*.py
    ${PROJECTGROUP_SOURCE_DIR}/build/*.in
)
list(APPEND 
  ${PROJECTGROUP_SOURCE_DIR}/CMakeSource.cmake
  ${PROJECTGROUP_SOURCE_DIR}/NEWS.txt
)

file(GLOB_RECURSE MAKE_FILES 
    ${PROJECTGROUP_SOURCE_DIR}/build/*.py 
    ${PROJECTGROUP_SOURCE_DIR}/build/*.mk
)

set(_subfolder Scripts)
set(_replacement ${PROJECTGROUP_SOURCE_DIR})

foreach(file ${SCRIPT_FILES} ${MAKE_FILES})
    string(REPLACE "${_replacement}/" "" _file ${file})
    get_filename_component(src_path ${_file} DIRECTORY)
    if (src_path)
        string(REPLACE "/" "\\" src_path ${src_path})
        set(_folder_name "${_subfolder}\\{src_path}")
    else()
        set(_folder_name "${_subfolder}")
    endif()
    if(${file} MATCHES ".mk")
      string(APPEND src_path "\\make")
      # message(FATAL_ERROR "Stop: ${src_path} -> ${file}")
    endif()
    # source_group("TestFolder" FILES  XX\\${file})
    # source_group("XX\\${src_path}" FILES  ${file})
    source_group("${src_path}" FILES  ${file})
    
endforeach()

add_library(${TARGET_NAME}
   EXCLUDE_FROM_ALL
    ../../src/Version.cpp 
    ${SCRIPT_FILES}
    ${MAKE_FILES}
)
add_custom_target(${TARGET_NAME}X
   EXCLUDE_FROM_ALL
    ${SCRIPT_FILES}
    ${MAKE_FILES}
)


# ================ Android =============================
file(GLOB_RECURSE ANDROID_FILES 
    ${PROJECTGROUP_SOURCE_DIR}/android/*.*
)
# set(_replacement)
foreach(file ${SCRIPT_FILES} ${ANDROID_FILES})
    string(REPLACE "${_replacement}/" "" _file ${file})
    get_filename_component(src_path ${_file} DIRECTORY)
    if (src_path)
        string(REPLACE "/" "\\" src_path ${src_path})
        set(_folder_name "${_subfolder}\\{src_path}")
    else()
        set(_folder_name "${_subfolder}")
    endif()
    source_group("${src_path}" FILES  ${file})
endforeach()

if(0)
add_custom_target(${TARGET_NAME}J
   EXCLUDE_FROM_ALL
    ${ANDROID_FILES}
)
else()
add_library(${TARGET_NAME}J
   EXCLUDE_FROM_ALL
    ../../src/Version.cpp 
    ${ANDROID_FILES}
)
endif()
# ================ Android-End =========================

include(xcsoar.cmake)

set_target_properties(${TARGET_NAME}  PROPERTIES FOLDER _Scripts)
set_target_properties(${TARGET_NAME}X PROPERTIES FOLDER _Scripts)
set_target_properties(${TARGET_NAME}J PROPERTIES FOLDER _Scripts)
