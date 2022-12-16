set(TARGET_NAME "XCSoarAug-MinGW")  # hardcoded yet

message(STATUS "+++ System = WIN32 / MinGW (${TOOLCHAIN})  on ${CMAKE_HOST_SYSTEM_NAME} !!!")

set(LIB_PREFIX "lib" )  # "lib")
set(LIB_SUFFIX ".a")    # "a")

add_compile_definitions(BOOST_ASIO_SEPARATE_COMPILATION)
add_compile_definitions(BOOST_MATH_DISABLE_DEPRECATED_03_WARNING=ON)

# Bei Windows brauche ich das nicht, aber hilft das eventuell beim Cross-Compile unter Linux?
add_compile_definitions(BOOST_JSON_HEADER_ONLY)
add_compile_definitions(BOOST_JSON_STANDALONE)

        # add_compile_definitions(HAVE_MSVCRT)
add_compile_definitions(UNICODE)  # ???
add_compile_definitions(_UNICODE)
add_compile_definitions(STRICT)
add_compile_definitions(_USE_MATH_DEFINES)   # necessary under C++17!
add_compile_definitions(ZZIP_1_H)   # definition of uint32_t and Co.!
#  string(APPEND CMAKE_CXX_FLAGS " -Og -funit-at-a-time -ffast-math -g -std=c++20 -fno-threadsafe-statics -fmerge-all-constants -fcoroutines -fconserve-space -fno-operator-names -fvisibility=hidden -finput-charset=utf-8 -Wall -Wextra -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare -Wundef -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wvla -Wno-format-truncation -Wno-missing-field-initializers -Wcast-align -Werror -I./src/unix -I./_build/include -isystem /home/august/Projects/link_libs/boost/boost-1.80.0 ")
# string(APPEND CMAKE_CXX_FLAGS " -c -Og -funit-at-a-time -ffast-math -g -std=c++20 -fno-threadsafe-statics -fmerge-all-constants -fcoroutines -fconserve-space -fno-operator-names -finput-charset=utf-8 -Wall -Wextra -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare -Wundef -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wvla -Wno-format-truncation -Wno-missing-field-initializers -Wcast-align -Werror -m64 -mwin32 -mwindows -mms-bitfields")
string(APPEND CMAKE_CXX_FLAGS " -std=c++20")    # C++20
string(APPEND CMAKE_CXX_FLAGS " -fcoroutines")  # use CoRoutines
string(APPEND CMAKE_CXX_FLAGS " -Wno-cpp")  # disable the warning 'Please include winsock2.h before windows.h'
if(VERBOSE_CXX)
    string(APPEND CMAKE_CXX_FLAGS " -v")  # verbose..
endif()

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
    include_directories("D:/Programs/MinGW/${TOOLCHAIN}/include")
    #  later: include_directories("${PROJECTGROUP_SOURCE_DIR}/output/include")
    # include_directories(D:/Projects/link_libs/boost/boost-1.80.0/include/boost-1_80)
    include_directories(${LINK_LIBS}/boost/boost-1.80.0/include/boost-1_80)
else()
include_directories(
       /usr/include
       /usr/include/x86_64-linux-gnu
       /usr/lib/gcc/x86_64-w64-mingw32/10-win32/include
       #  later: include_directories("${PROJECTGROUP_SOURCE_DIR}/output/include")
       include_directories("${PROJECTGROUP_SOURCE_DIR}/output/src/boost_1_80_0")
)

endif()
#######################################################################
      # list(APPEND XCSOAR_LINK_LIBRARIES
      set(BASIC_LINK_LIBRARIES
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

# if(EXISTS "D:/Programs")  # on Windows - and on Flaps6 (August2111)
if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
    list(APPEND CMAKE_PROGRAM_PATH "D:/Programs")
endif()
