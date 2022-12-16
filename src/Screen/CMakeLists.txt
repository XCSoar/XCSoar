cmake_minimum_required(VERSION 3.15)
if (SHOW_SUBPROJECTS)
    message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")
endif()

  get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

include(CMakeSource.cmake)
include(CMakeSource.cmake)
# organize the files in subdirectories


set(SOURCE_FILES )
foreach(source_file ${_SOURCES})
    string(REPLACE "${TARGET_NAME}/" "" source_file ${source_file})
    string(REPLACE "${SRC}/" "" source_file ${source_file})
    list(APPEND SOURCE_FILES ${source_file})
    get_filename_component(src_path ${source_file} DIRECTORY)
    if (src_path)
        string(REPLACE "/" "\\" src_path ${src_path})  ### for VisualStudio-IDE
    endif()
    source_group("Source\\${src_path}" FILES  ${source_file})
    ## message(STATUS "### ${src_path}  --- ${source_file}")
endforeach()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

source_group("Scripts" FILES  ${SCRIPT_FILES})

add_library(${TARGET_NAME} ${XCSOAR_LIB_TYPE}
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${SCRIPT_FILES}
)
# message(FATAL_ERROR "Stop!")
target_link_libraries(${TARGET_NAME} PUBLIC Hardware)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER FrontEnd)

add_dependencies(${TARGET_NAME} util)
