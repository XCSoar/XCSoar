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

if(NOT HEADER_FILES)  #  STREQUAL ""
file(GLOB_RECURSE   HEADER_FILES_TEMP   "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")  # ;../*.hxx;../*.h
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
    ${OUTPUT_FOLDER}/include
)

set(MATHTABLES_H ${OUTPUT_FOLDER}/include/MathTables.h)

if(MSVC)  # Multi-Config!
    set(MATHTABLES_CMD ${PROJECTGROUP_BINARY_DIR}/tools/Debug/MathTables)
else()
    set(MATHTABLES_CMD ${PROJECTGROUP_BINARY_DIR}/tools/MathTables)
endif()
add_custom_command( # TARGET ${TARGET_NAME} PRE_BUILD
        OUTPUT ${MATHTABLES_H}
        COMMENT "Create 'MathTables.h' with Sine tables"
        COMMAND ${MATHTABLES_CMD} >${MATHTABLES_H} 
        WORKING_DIRECTORY ${PROJECTGROUP_SOURCE_DIR}
        # DEPENDS GenerateSineTables.cpp
        DEPENDS MathTables
        # ${PROJECTGROUP_SOURCE_DIR}/tools/GenerateSineTables.cpp
    )
list(APPEND HEADER_FILES ${MATHTABLES_H})

add_library(${TARGET_NAME} ${XCSOAR_LIB_TYPE}
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${SCRIPT_FILES}
)
# message(FATAL_ERROR "Stop!")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Libs)

# Aug: mal kurz ausblenden 5.9.22, 
# add_dependencies(${TARGET_NAME} util)

## add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
##         # OUTPUT MathTables.h
##         COMMENT "Create 'MathTables.h' with Sine tables"
##         echo tools/Debug/MathTables to MathTables.h 
##         COMMAND ${PROJECTGROUP_BINARY_DIR}/tools/Debug/MathTables > MathTables.h 
##         WORKING_DIRECTORY ${PROJECTGROUP_SOURCE_DIR}/${OUTPUT}/include
##         DEPENDS GenerateSineTables.cpp
##     )

# target_link_libraries(${TARGET_NAME} MathTables)
add_dependencies(${TARGET_NAME} MathTables Data)

