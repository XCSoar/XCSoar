set(_SOURCES
        json/Boost.cxx
        json/ParserOutputStream.cxx
        json/Serialize.cxx
        json/Parse.cxx  # add 7.38
)

set(SCRIPT_FILES
    CMakeSource.cmake
)

file(GLOB_RECURSE HEADER_FILES "*.h;*.hxx;*.hpp")
