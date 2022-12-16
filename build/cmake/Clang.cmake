set(TARGET_NAME "XCSoarAug-Clang")  # hardcoded

message(STATUS "+++ System = WIN32 / Clang!")

set(LIB_PREFIX "" )  # "lib")
set(LIB_SUFFIX ".lib")    # "a")
# add_compile_definitions(BOOST_ASIO_SEPARATE_COMPILATION)
add_compile_definitions(__CLANG__)
add_compile_definitions(_AUG_CLANG=1)
# add_compile_definitions(HAVE_MSVCRT)
add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)  # ???
add_compile_definitions(STRICT)
add_compile_definitions(_USE_MATH_DEFINES)   # necessary under C++17!

add_compile_definitions(-std=c++20)
# gibt es nicht mehr: --- include_directories("${PROJECTGROUP_SOURCE_DIR}/temp/data")  # temporary data!
if (ON OR WIN64)  # momentan kein Flag verfügbar!
    add_compile_definitions(_AMD64_)
else()
    message(FATAL_ERROR "Error: WIN32 not implemented?")
endif()

set(SSL_LIB )
set(CRYPTO_LIB )
