cmake_minimum_required(VERSION 3.15)
if (SHOW_SUBPROJECTS)
    message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")
endif()

  get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

include(CMakeSource.cmake)
# organize the files in subdirectories
set(SOURCE_FILES )
foreach(source_file ${_SOURCES})
    string(REPLACE "${TARGET_NAME}/" "" source_file ${source_file})
    list(APPEND SOURCE_FILES ${source_file})
    get_filename_component(src_path ${source_file} DIRECTORY)
    if (src_path)
        string(REPLACE "/" "\\" src_path ${src_path})
    endif()
    source_group("Source\\${src_path}" FILES  ${source_file})
    # message(STATUS "### ${src_path}  --- ${source_file}")
endforeach()

source_group("Scripts" FILES  ${SCRIPT_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_compile_definitions(CARES_STATICLIB)

add_library(${TARGET_NAME} ${XCSOAR_LIB_TYPE}
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${SCRIPT_FILES}
)

# message(STATUS "CARES-Lib = ${CARES_LIB}")
# message(STATUS "CARES-Lib = ${CARES_LIB}")
# set(XCSOAR_CARES_VERSION c-ares-1.17.1)
# set(CARES_LIB ${LINK_LIBS}/c-ares/${XCSOAR_CARES_VERSION}/lib/${TOOLCHAIN}/cares.lib)

# message(STATUS "CARES-Lib = ${CARES_LIB}")
# message(FATAL_ERROR "CARES-Lib = ${CARES_LIB} ")

# target_link_libraries(${TARGET_NAME} PUBLIC ${CARES_LIB} system time)
target_link_libraries(${TARGET_NAME} PUBLIC system time ${CARES_LIB})

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Libs)

add_dependencies(${TARGET_NAME} util)

