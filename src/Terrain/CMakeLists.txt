cmake_minimum_required(VERSION 3.15)
if (SHOW_SUBPROJECTS)
    message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")
endif()

  get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

  project(${TARGET_NAME} CXX) # Your project name

# include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeSource.cmake)
if (EXISTS CMakeSource.cmake)
    include(CMakeSource.cmake)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeSource.cmake)
    #  message(FATAL_ERROR "Stop: ${CMAKE_CURRENT_SOURCE_DIR}/CMakeSource.cmake found")
    # message(STATUS "!!! ${CMAKE_CURRENT_SOURCE_DIR}/CMakeSource.cmake found")
    include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeSource.cmake)
else()
    message(FATAL_ERROR "Stop!")
    # include(../../CMakeSource.cmake)
endif()
# organize the files in subdirectories

# list(APPEND ${TARGET_NAME}_SOURCES  ${TARGET_NAME}/jpc_rtc.cpp)
set(SOURCE_FILES )
foreach(source_file ${_SOURCES})
    list(APPEND SOURCE_FILES ${source_file})
    # string(REPLACE "${TARGET_NAME}/" "" source_file ${source_file})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" source_file ${source_file})
    get_filename_component(src_path ${source_file} DIRECTORY)
    if (src_path)
        string(REPLACE "/" "\\" src_path ${src_path})
    endif()
    source_group("Source\\${src_path}" FILES  "${source_file}")
    ### hide: message(STATUS "### ${src_path}  --- ${source_file}")
endforeach()

if(NOT HEADER_FILES)  #  STREQUAL ""
# with jasper...:  
file(GLOB_RECURSE   HEADER_FILES_TEMP   "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")  # ;../*.hxx;../*.h
# without jasper...:  file(GLOB   HEADER_FILES_TEMP   "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")  # ;../*.hxx;../*.h
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
   ##  message(FATAL_ERROR "### ### Header-Liste  --- ${HEADER_FILES}" )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${TARGET_NAME} ${XCSOAR_LIB_TYPE}
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${SCRIPT_FILES}
)
target_link_libraries(${TARGET_NAME} PUBLIC
    io
    ui
    thread
    # Screen
    Renderer
#    ${JASPER_LIB}  # internal or external...
#    ${ZZIP_LIB}    # internal or external...
    jasper
    zzip
)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Topo)

add_dependencies(${TARGET_NAME} util jasper zzip)

