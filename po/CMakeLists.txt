cmake_minimum_required(VERSION 3.15)
if (SHOW_SUBPROJECTS)
    message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")
endif()

# get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
set(TARGET_NAME po-Files)

# include(CMakeSource.cmake)
# organize the files in subdirectories

find_program(PYTHON_APP NAMES python REQUIRED)

set (SOURCE_FILES)

# set(PO_FILES        "${PROJECTGROUP_SOURCE_DIR}/po/de.po")
file(GLOB PO_FILES        "*.po")
set(SCRIPT_FILES )
set(MO_C_FILES)
set(output_dir ${OUTPUT_FOLDER}/data)
get_filename_component(_python_dir ${PYTHON_APP} DIRECTORY)
set(MSGFMT  "${_python_dir}/Tools/i18n/msgfmt.py")

if (1)  # only if a po_file changed...
    foreach(po_file ${PO_FILES})
        get_filename_component(lang_name ${po_file}  NAME_WE)
        ## message(STATUS "po_file  = ${po_file}  ")
        set(out_file ${output_dir}/${lang_name}.mo.c)
        set(_mo_file ${output_dir}/temp/${lang_name}.mo)
        # add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        add_custom_command(OUTPUT ${out_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}/temp 
            COMMAND ${PYTHON_APP} ${MSGFMT} --output-file=${_mo_file} ${po_file}
            COMMAND ${PYTHON_APP} tools/python/bin2c.py ${_mo_file} ${output_dir}
            WORKING_DIRECTORY  ${PROJECTGROUP_SOURCE_DIR}
            COMMENT "${lang_name} ==> ${out_file}"
            DEPENDS ${po_file}
        )
        list(APPEND MO_C_FILES ${out_file})
    endforeach()
endif()
#! add_custom_target(${TARGET_NAME}
### add_custom_target(TARGET ${TARGET_NAME} PRE_BUILD
###     COMMAND ${CMAKE_COMMAND} -E make_directory  ${output_dir}/temp
###     WORKING_DIRECTORY  ${PROJECTGROUP_SOURCE_DIR}
###     COMMENT Make dir '${output_dir}'
### )

if(MSVC)
    source_group("po-Files" FILES  ${PO_FILES})
endif()

add_library(${TARGET_NAME} ${XCSOAR_LIB_TYPE} ${MO_C_FILES} ${PO_FILES})
set_target_properties(${TARGET_NAME} PROPERTIES
                        LINKER_LANGUAGE C
                        FOLDER Data
)

add_dependencies(${TARGET_NAME}  Data)


