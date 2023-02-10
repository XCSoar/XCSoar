set(TARGET_NAME "XCSoarAug-Clang")  # hardcoded

message(STATUS "+++ System = WIN32 / Clang!")

if(NOT TOOLCHAIN)
#    set(TOOLCHAIN clang14)
endif()

include_directories(D:/Programs/LLVM/${TOOLCHAIN}/include)

# include_directories(D:/Programs/Android/android-ndk-r25b/sources/cxx-stl/llvm-libc++/include)
# include_directories(D:/Programs/Android/android-ndk-r25b/sources/cxx-stl/system/include)

include_directories(D:/Programs/LLVM/${TOOLCHAIN}/lib/clang/${TOOLCHAIN}.0.7/include)

set(LIB_PREFIX "lib")
set(LIB_SUFFIX ".a")
# add_compile_definitions(BOOST_ASIO_SEPARATE_COMPILATION)
add_compile_definitions(BOOST_ASIO_SEPARATE_COMPILATION)
add_compile_definitions(BOOST_JSON_HEADER_ONLY)
add_compile_definitions(BOOST_JSON_STANDALONE)
add_compile_definitions(BOOST_MATH_DISABLE_DEPRECATED_03_WARNING=ON) 

add_compile_definitions(__CLANG__)
add_compile_definitions(_WIN32) # this should be by default?
# add_compile_definitions(HAVE_MSVCRT)
add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)  # ???
add_compile_definitions(STRICT)
add_compile_definitions(_USE_MATH_DEFINES)   # necessary under C++17!
# add_compile_definitions(USE_WIN32_RESOURCES)

# add_compile_options(-v)  # verbose compiler messages

add_compile_options(-fcoroutines-ts)
## add_compile_options(-fconserve-space)
## add_compile_options(-fno-operator-names)

# gibt es nicht mehr: --- include_directories("${PROJECTGROUP_SOURCE_DIR}/temp/data")  # temporary data!
if (ON OR WIN64)  # momentan kein Flag verfügbar!
    add_compile_definitions(WIN64)  ## ????
    add_compile_definitions(_AMD64_)
else()
    message(FATAL_ERROR "Error: WIN32 not implemented?")
endif()

list(APPEND XCSOAR_LINK_LIBRARIES
    wsock32
    ws2_32
    gdi32
    gdiplus
    crypt32
#    winpthread
)

add_compile_definitions(__CLANG__)

add_compile_definitions(WIN32_LEAN_AND_MEAN)
 # warning C4996: 'xxx': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _wcsdup. See online help for details.
 # xxx: wcscpy, wcsdup, strtok, strcpy, strdup, ....
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE)
###### add_compile_definitions(__STDC_WANT_SECURE_LIB__=0)
###### add_compile_definitions(_CRT_SECURE_NO_DEPRECATE=0)
###### add_definitions(-Wdeprecated-declarations)
#********************************************************************************
if(AUGUST_SPECIAL)
    add_compile_definitions(_AUG_CLANG)
endif()
#********************************************************************************
set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")
list(APPEND CMAKE_CXX_FLAGS  -std=c++20)  ## c++20 - only for cpp and not for c - "add_compile_options(-std=c++20)"!
# set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -m64 -lwsock32 -lws2_32 -lgdi32 -lgdiplus -lcrypt32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -m64")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static  -v")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive -v")
#  list(APPEND CMAKE_EXE_LINKER_FLAGS -static -v)


set(SSL_LIB )  # no ssl lib on windows! == Use Schannel
set(CRYPTO_LIB Crypt32.lib BCrypt.lib) # no (OpenSSL-)crypto lib on windows!

set(PERCENT_CHAR "%%" GLOBAL)
set(DOLLAR_CHAR  "$$" GLOBAL)
### message(FATAL_ERROR "Stop clang")
