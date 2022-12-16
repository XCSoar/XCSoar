set(TARGET_NAME "XCSoarAug-MinGW")  # hardcoded yet

message(STATUS "+++ System = WIN32 / MinGW (${TOOLCHAIN})! on ${CMAKE_HOST_SYSTEM_NAME} ")

set(LIB_PREFIX "lib" )  # "lib")
set(LIB_SUFFIX ".a")    # "a")

# August2111: warum? string(APPEND CMAKE_CXX_FLAGS " -U_REENTRANT")
add_compile_definitions(BOOST_NO_IOSTREAM)
add_compile_definitions(BOOST_MATH_NO_LEXICAL_CAST)
add_compile_definitions(BOOST_UBLAS_NO_STD_CERR)
add_compile_definitions(BOOST_ERROR_CODE_HEADER_ONLY)
add_compile_definitions(BOOST_SYSTEM_NO_DEPRECATED)
add_compile_definitions(BOOST_NO_STD_LOCALE)
add_compile_definitions(BOOST_LEXICAL_CAST_ASSUME_C_LOCALE)
add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE)
add_compile_definitions(WINVER=0x0600)
add_compile_definitions(_WIN32_WINDOWS=0x0600)
add_compile_definitions(_WIN32_WINNT=0x0600)
add_compile_definitions(_WIN32_IE=0x0600)
add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(NOMINMAX)
add_compile_definitions(HAVE_STRUCT_POLLFD)
add_compile_definitions(HAVE_MSVCRT)
add_compile_definitions(UNICODE)  # ???
add_compile_definitions(_UNICODE)
add_compile_definitions(STRICT)
add_compile_definitions(EYE_CANDY)
add_compile_definitions(USE_WIN32_RESOURCES)
add_compile_definitions(_GLIBCXX_ASSERTIONS)

add_compile_definitions(BOOST_JSON_STANDALONE)


#  string(APPEND CMAKE_CXX_FLAGS " -Og -funit-at-a-time -ffast-math -g -std=c++20 -fno-threadsafe-statics -fmerge-all-constants -fcoroutines -fconserve-space -fno-operator-names -fvisibility=hidden -finput-charset=utf-8 -Wall -Wextra -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare -Wundef -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wvla -Wno-format-truncation -Wno-missing-field-initializers -Wcast-align -Werror -I./src/unix -I./_build/include -isystem /home/august/Projects/link_libs/boost/boost-1.80.0 ")
string(APPEND CMAKE_CXX_FLAGS " -c -Og -funit-at-a-time -ffast-math -g -std=c++20 -fno-threadsafe-statics -fmerge-all-constants -fcoroutines -fconserve-space -fno-operator-names -finput-charset=utf-8 -Wall -Wextra -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare -Wundef -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wvla -Wno-format-truncation -Wno-missing-field-initializers -Wcast-align -Werror -m64 -mwin32 -mwindows -mms-bitfields")

# string(APPEND CMAKE_CXX_FLAGS " -v")

include_directories(
    ${SRC}
    ${SRC}/Engine
        /usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/c++
        /usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/c++/x86_64-w64-mingw32
        /usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/c++/backward
        /usr/lib/gcc/x86_64-w64-mingw32/10-win32/include
        /usr/lib/gcc/x86_64-w64-mingw32/10-win32/include-fixed
        /usr/lib/gcc/x86_64-w64-mingw32/10-win32/../../../../x86_64-w64-mingw32/include
)
#         /home/august/Projects/XCsoar/output/WIN64/lib/x86_64-w64-mingw32/include

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
    include_directories("D:/Programs/MinGW/${TOOLCHAIN}/include")
    #  later: include_directories("${PROJECTGROUP_SOURCE_DIR}/output/include")
endif()


#additional to make TARGET=WIN64: -isystem statt -I 
include_directories(
       /usr/include
       /usr/include/x86_64-linux-gnu
       ${LINK_LIBS}/boost/boost-1.80.0
)

#######################################################################
      list(APPEND XCSOAR_LINK_LIBRARIES
        msimg32
        winmm
        # dl
        pthread
        stdc++
        user32
        gdi32
        gdiplus
        ws2_32
        mswsock
        kernel32
        # ?? msvcrt32
        shell32
        gcc_s
      )

set(MINGW ON)
add_compile_definitions(__MINGW__)
#********************************************************************************
if(AUGUST_SPECIAL)
    add_compile_definitions(_AUG_MGW=1)
endif()
#********************************************************************************
set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -m64 -lwsock32 -lws2_32 -lgdi32 -lgdiplus -lcrypt32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive -v")
    
if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
    set(SSL_LIB)
    set(CRYPTO_LIB Crypt32.lib BCrypt.lib) # no (OpenSSL-)crypto lib on windows!
endif()

set(PERCENT_CHAR \%)
set(DOLLAR_CHAR  $$)
