cmake_minimum_required(VERSION 3.15)
if (SHOW_SUBPROJECTS)
    message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")
endif()

set(TARGET_NAME MathTables)

add_executable(${TARGET_NAME} GenerateSineTables.cpp)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Data)

add_custom_command(TARGET ${TARGET_NAME} PRE_LINK
# add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
       COMMENT "Make dir '${OUTPUT}/include'"
       COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_FOLDER}/include
       WORKING_DIRECTORY ${OUTPUT_FOLDER}
)

