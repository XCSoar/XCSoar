set(ZZIP_DIR     ${CMAKE_CURRENT_SOURCE_DIR}) #/zzip) 
set(_SOURCES
        ${ZZIP_DIR}/fetch.c
        ${ZZIP_DIR}/file.c
        ${ZZIP_DIR}/plugin.c
        ${ZZIP_DIR}/stat.c
        ${ZZIP_DIR}/zip.c
)

set(HEADER_FILES
        # zzip/autoconf.h
        ${ZZIP_DIR}/conf.h
        ${ZZIP_DIR}/fetch.h
        ${ZZIP_DIR}/file.h
        ${ZZIP_DIR}/format.h
        ${ZZIP_DIR}/info.h
        ${ZZIP_DIR}/lib.h
        ${ZZIP_DIR}/plugin.h
        ${ZZIP_DIR}/stdint.h
        ${ZZIP_DIR}/types.h
        ${ZZIP_DIR}/util.h
        ${ZZIP_DIR}/zzip.h
        ${ZZIP_DIR}/zzip32.h
        ${ZZIP_DIR}/_config.h
        ${ZZIP_DIR}/_msvc.h  # only for windows host
        ${ZZIP_DIR}/__debug.h
        ${ZZIP_DIR}/__hints.h
        ${ZZIP_DIR}/__mmap.h
)
set(SCRIPT_FILES
        ${ZZIP_DIR}/CMakeLists.txt
        ${ZZIP_DIR}/CMakeSource.cmake
)

