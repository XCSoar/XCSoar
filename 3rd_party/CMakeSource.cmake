file(GLOB SCRIPT_FILES *.txt *.in)
list(APPEND SCRIPT_FILES CMakeSource.cmake 3rd_party.cmake)


set(CMAKE_FILES
  boost.cmake
  zlib.cmake
  lua.cmake
  png.cmake
  cares.cmake
)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND CMAKE_FILES curl.cmake)
else()
# TODO(August2111): only temporarily curl.cmake
endif()

list(APPEND CMAKE_FILES sodium.cmake)
list(APPEND CMAKE_FILES fmt.cmake)

if(0)  # MapServer
    list(APPEND CMAKE_FILES mapserver.cmake)
endif()
if(0)  # zzip
    list(APPEND CMAKE_FILES zzip.cmake)
endif()
if(0) # XML-Parser
    list(APPEND CMAKE_FILES xmlparser.cmake)
endif()

# for icon convert:
if(0) # RSVG
    list(APPEND CMAKE_FILES rsvg.cmake)
endif()
if(0)  # InkScape
    list(APPEND CMAKE_FILES inkscape.cmake)
endif()
if(0) # RSVG
    list(APPEND CMAKE_FILES iconv.cmake)
    list(APPEND CMAKE_FILES xml2.cmake)
    list(APPEND CMAKE_FILES xlst.cmake)
endif()


# For OpenGL:
if(0)  # freeglut
    list(APPEND CMAKE_FILES freeglut.cmake)
endif()
if(ENABLE_SDL) # SDL for OpenGL?
    list(APPEND CMAKE_FILES sdl.cmake)
endif()
if(ENABLE_GLM) # GLM for OpenGL?
    list(APPEND CMAKE_FILES glm.cmake)
endif()
