cmake_minimum_required(VERSION 3.15)
message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")

  get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

file(GLOB_RECURSE TEST_LIST *.cpp)

foreach(lib ${XCSOAR_LINK_LIBRARIES})
     message(STATUS "### Add-Library to test: ${lib} ")
 
endforeach()
# message(FATAL_ERROR "### Stop !!!!")
foreach(source_file ${TEST_LIST})
#    string(REPLACE "${TARGET_NAME}/" "" source_file ${source_file})
#    list(APPEND SOURCE_FILES ${source_file})

    add_compile_definitions("__STDC_VERSION__= 199901L") # ?
    get_filename_component(test ${source_file} NAME_WE)
    get_filename_component(test_path ${source_file} DIRECTORY)

    add_executable(${test}  ${source_file} 
    ${test_path}/tap.c  )
#    set_target_properties(${test} PROPERTIES FOLDER _Test)
#    set_target_properties(${test} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(${test} PROPERTIES 
                FOLDER _Test
                EXCLUDE_FROM_ALL TRUE
    )

    target_link_libraries(${test} PUBLIC ${XCSOAR_LINK_LIBRARIES})

    add_dependencies(${test} util libXCSoar)  # Dialogs Math)

#     message(STATUS "### ${test_path}  --- ${test}")
endforeach()
