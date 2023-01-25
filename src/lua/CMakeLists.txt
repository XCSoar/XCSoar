cmake_minimum_required(VERSION 3.15)
if (SHOW_SUBPROJECTS)
    message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")
endif()

  get_filename_component(PUR_TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
  set(TARGET_NAME "${PUR_TARGET_NAME}")

include(CMakeSource.cmake)

# add_compile_definitions("PROGRAM_NAME_LC=\"${PROGRAM_NAME_LC}\"")
add_compile_definitions("PROGRAM_NAME_LC=\"xcsoar\"")

# organize the files in subdirectories
set(SOURCE_FILES )
foreach(source_file ${_SOURCES})
    string(REPLACE "${PUR_TARGET_NAME}/" "" source_file ${source_file})
    list(APPEND SOURCE_FILES ${source_file})
    get_filename_component(src_path ${source_file} DIRECTORY)
    if (src_path)
        string(REPLACE "/" "\\" src_path ${src_path})
    endif()
    source_group("Source\\${src_path}" FILES  ${source_file})
    # message(STATUS "### ${src_path}  --- ${source_file}")
endforeach()

if(NOT HEADER_FILES)  #  predefined in CMakeSource.cmake?
file(GLOB_RECURSE   HEADER_FILES_TEMP   "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")
#    message(FATAL_ERROR "### ### Header-Liste  --- ${HEADER_FILES_TEMP}" )
set(HEADER_FILES)
foreach(header_file ${HEADER_FILES_TEMP})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" header_file ${header_file})
    list(APPEND HEADER_FILES ${header_file})
    get_filename_component(src_path ${header_file} DIRECTORY)
    if (src_path)
        string(REPLACE "/" "\\" src_path ${src_path})
    endif()
    source_group("Header\\${src_path}" FILES  ${header_file})
    ## message(STATUS "### ### ${src_path}  --- ${header_file}" )
endforeach()
#     message(FATAL_ERROR "### ### Header-Liste  --- ${HEADER_FILES}" )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_OUTPUT_FOLDER}/include)

add_library(${TARGET_NAME} ${XCSOAR_LIB_TYPE}
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${SCRIPT_FILES}
)
# message(FATAL_ERROR "Stop!")
target_link_libraries(${TARGET_NAME} PUBLIC util)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Libs)

# if(NOT EXISTS "${INSTALL_LIB}")
if(${WITH_3RD_PARTY})
add_dependencies(${TARGET_NAME} util Input lua_3rd)
else()
add_dependencies(${TARGET_NAME} util Input)
endif()