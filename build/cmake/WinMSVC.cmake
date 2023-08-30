message(STATUS "+++ System = WIN32 / MSVC (${TOOLCHAIN})!")

set(LIB_PREFIX "" )  # "lib")
set(LIB_SUFFIX ".lib")    # "a")
# ??? add_compile_definitions(PROJECT_OUTPUT_FOLDER=${OUTPUT_FOLDER})

add_definitions(-DIS_OPENVARIO)  # add special OpenVario functions
add_compile_definitions(__MSVC__)
#********************************************************************************
set(AUGUST_SPECIAL ON)

if(AUGUST_SPECIAL)
    add_compile_definitions(__AUGUST__=1)
    add_compile_definitions(_AUG_MSC)
endif()
#********************************************************************************

add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)  # ???
add_compile_definitions(NO_ERROR_CHECK)  # EnumBitSet funktioniert m.E. noch nicht korrekt!!!!
add_compile_definitions(WIN32_LEAN_AND_MEAN)
 # warning C4996: 'xxx': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _wcsdup. See online help for details.
 # xxx: wcscpy, wcsdup, strtok, strcpy, strdup, ....
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(_SCL_SECURE_NO_WARNINGS)
# add_compile_definitions(/std:c++20)
# add_definitions(/std:c++20)
add_compile_options(/std:c++20)
add_compile_options(/Zc:__cplusplus)
add_compile_options(/utf-8)
#add_compile_definitions(/Zc:__cplusplus)
#add_compile_definitions(/utf-8)
# add_definitions(/Zc:wchar_t)

# Disabling Warnings:
add_compile_options(/wd5030)
add_compile_options(/wd4455)  # "suffix warning?"
add_compile_options(/wd4805)  #  "|": unsichere Kombination von Typ "bool" mit Typ "int" in einer Operation


add_compile_definitions(BOOST_ASIO_SEPARATE_COMPILATION)
add_compile_definitions(BOOST_JSON_HEADER_ONLY)
add_compile_definitions(BOOST_JSON_STANDALONE)
add_compile_definitions(BOOST_MATH_DISABLE_DEPRECATED_03_WARNING=ON) 

if (ON OR WIN64)  # momentan kein Flag für 64bit verfügbar!
    add_compile_definitions(WIN64)
    add_compile_definitions(_AMD64_)
else()
    message(FATAL_ERROR "Error: WIN32 not implemented?")
endif()
# set(FREEGLUT_LIB_DIR "${LINK_LIBS}/freeglut-MSVC-3.0.0-2/freeglut")
#      set(SODIUM_LIB "${LINK_LIBS}/libsodium/x64/Release/v142/static/libsodium.lib")
add_compile_definitions(SODIUM_STATIC=1)  # MSCV only...

# see below      add_compile_definitions(CURL_STATICLIB)
add_compile_definitions(LDAP_STATICLIB)

set(BASIC_LINK_LIBRARIES
        msimg32.lib
        winmm.lib
        ws2_32.lib
        gdiplus
)

set(SSL_LIB )  # no ssl lib on windowsfor curl necessary!
set(CRYPTO_LIB Crypt32.lib BCrypt.lib)

set(USE_MEMORY_CANVAS OFF)

set(PERCENT_CHAR %%)
set(DOLLAR_CHAR \$)


if(EXISTS "D:/Programs")  # on Windows only - and at Flaps6 (August2111)
    list(APPEND CMAKE_PROGRAM_PATH "D:/Programs")
else()
    list(APPEND CMAKE_PROGRAM_PATH "C:/Program Files")
endif()
