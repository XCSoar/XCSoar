set(TARGET_NAME "XCSoarAug-Linux")  # hardcoded yet

message(STATUS "+++ System = Linux / GCC (${TOOLCHAIN})  on ${CMAKE_HOST_SYSTEM_NAME} !")

set(LIB_PREFIX "lib" )  # "lib")
set(LIB_SUFFIX ".a")    # "a")


set(UNIX ON)
add_compile_definitions(__LINGCC__)

#    include_directories("/usr/include/x86_64-linux-gnu")
#    message(FATAL_ERROR "Stop????")

#********************************************************************************
if(AUGUST_SPECIAL)
    add_compile_definitions(_AUG_GCC=1)
endif()
#********************************************************************************
set(ENABLE_OPENGL ON)  # better outside????
set(ENABLE_SDL OFF)  # better outside????
set(USE_MEMORY_CANVAS OFF)  # das ist hier auch falsch!!!!

add_compile_definitions(BOOST_NO_IOSTREAM)
add_compile_definitions(BOOST_MATH_NO_LEXICAL_CAST)
add_compile_definitions(BOOST_UBLAS_NO_STD_CERR)
add_compile_definitions(BOOST_ERROR_CODE_HEADER_ONLY)
add_compile_definitions(BOOST_SYSTEM_NO_DEPRECATED)
add_compile_definitions(BOOST_NO_STD_LOCALE)
add_compile_definitions(BOOST_LEXICAL_CAST_ASSUME_C_LOCALE)
add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE)
add_compile_definitions(HAVE_POSIX)
add_compile_definitions(HAVE_VASPRINTF)
add_compile_definitions(EYE_CANDY)
add_compile_definitions(_GLIBCXX_ASSERTIONS)
add_compile_definitions(ENABLE_ALSA)
add_compile_definitions(USE_FREETYPE)
add_compile_definitions(ENABLE_OPENGL)
add_compile_definitions(HAVE_GLES)
add_compile_definitions(HAVE_GLES2)
add_compile_definitions(GL_GLEXT_PROTOTYPES)
add_compile_definitions(USE_GLX)
add_compile_definitions(USE_X11)
add_compile_definitions(USE_POLL_EVENT)
add_compile_definitions(BOOST_JSON_STANDALONE)

set(USE_POLL_EVENT ON)
set(ENABLE_OPENGL ON)

include_directories(
    ${SRC}/unix
    ${SRC}
    ${SRC}/Engine
    lib/glm
    /usr/include
    /usr/include/x86_64-linux-gnu
    ${LINK_LIBS}/boost/boost-1.80.0 
)

# string(APPEND CMAKE_CXX_FLAGS " -Og -funit-at-a-time -ffast-math -g -std=c++20 -fno-threadsafe-statics -fmerge-all-constants -fcoroutines -fconserve-space -fno-operator-names -fvisibility=hidden -finput-charset=utf-8 -Wall -Wextra -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare -Wundef -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wvla -Wno-format-truncation -Wno-missing-field-initializers -Wcast-align -Werror ")

string(APPEND CMAKE_CXX_FLAGS " -Og -funit-at-a-time -ffast-math -g -std=c++20 -fno-threadsafe-statics -fmerge-all-constants -fcoroutines -fconserve-space -fno-operator-names -fvisibility=hidden -finput-charset=utf-8 -Wall -Wextra -Wwrite-strings -Wpointer-arith -Wsign-compare -Wmissing-declarations -Wmissing-noreturn -Wvla -Wno-format-truncation -Wno-missing-field-initializers -Wcast-align -Werror ")


# string(APPEND CMAKE_CXX_FLAGS "-Werror=redundant-decls -Werror=cast-qual")

set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS}")


set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ ${CMAKE_CXX_STANDARD_LIBRARIES}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libstdc++ -Wl,-Bstatic,--whole-archive -Wl,--no-whole-archive -v")
# set(FREEGLUT_LIB_DIR "${LINK_LIBS}/freeglut-MinGW-3.0.0-1/freeglut")
    
    set(SSL_LIB)
    set(CRYPTO_LIB) # no extra (OpenSSL-)crypto lib on linux??!

set(PERCENT_CHAR \%)
set(DOLLAR_CHAR  $$)

list(APPEND CMAKE_PROGRAM_PATH "/home/august/Projects/XCSoar/_build")

include_directories(${SRC}/unix)
# message(FATAL_ERROR "Include ${SRC}/unix")

