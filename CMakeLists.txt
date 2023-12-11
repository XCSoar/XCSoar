cmake_minimum_required(VERSION 3.18)

set(SHOW_SUBPROJECTS OFF)
message(STATUS "+++ Start CMake ${CMAKE_CURRENT_SOURCE_DIR}!")
message(STATUS "+++ CMAKE_HOST_SYSTEM_NAME:     ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "+++ CMAKE_SYSTEM_NAME:          ${CMAKE_SYSTEM_NAME}")
message(STATUS "+++ TOOLCHAIN:                  ${TOOLCHAIN}")


get_filename_component(PROJECTGROUP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(PROJECTGROUP_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} ABSOLUTE)
message(STATUS "*** PROJECTGROUP_SOURCE_DIR = '${PROJECTGROUP_SOURCE_DIR}'")
message(STATUS "*** PROJECTGROUP_BINARY_DIR = '${PROJECTGROUP_BINARY_DIR}'")

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_BINARY_DIR} NAME)
set (PROJECT_NAME "OpenSoar-${PROJECT_NAME}")
project(${PROJECT_NAME})
set(TARGET_NAME OpenSoar)
set(PROGRAM_NAME ${TARGET_NAME})
set(PROGRAM_NAME_CC ${TARGET_NAME})              # CamelCase
string(TOLOWER  ${PROGRAM_NAME} PROGRAM_NAME_LC) # lowercase

### file(READ "VERSION.txt" PROGRAM_VERSION)
# add version string to OpenSoar:
file(READ OpenSoar.config _config_file)
# put the complete string in a list of lines
string(REPLACE "\n" ";" _config_file ${_config_file})
foreach(_line ${_config_file})
  string(REPLACE "=" ";" _line ${_line} )
  list(GET _line 0 _param0)
  list(GET _line 1 _param1)
  string(STRIP ${_param0} _param0)
  string(STRIP ${_param1} _param1)
  set(${_param0} ${_param1}) 
  # message(STATUS "+++++ '${_param0}' = '${_param1}'")
endforeach()# ---------------------------------------------------------------

message(STATUS "+++++ PROGRAM_NAME = '${PROGRAM_NAME}'")
message(STATUS "+++++ PROGRAM_VERSION = '${PROGRAM_VERSION}'")

add_compile_definitions("PROGRAM_VERSION=\"${PROGRAM_VERSION}\"")
file(WRITE "${PROJECTGROUP_BINARY_DIR}/VERSION.txt" ${PROGRAM_VERSION})

find_program(CMAKE_APP     NAMES cmake REQUIRED)
get_filename_component(CMAKE_PROGRAM_PATH ${CMAKE_APP} DIRECTORY)
get_filename_component(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} DIRECTORY)
get_filename_component(USER_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} DIRECTORY)
  message(STATUS "CMAKE_SYSTEM_PROGRAM_PATH - ${CMAKE_SYSTEM_PROGRAM_PATH}")
  message(STATUS "CMAKE_PROGRAM_PATH        - ${CMAKE_PROGRAM_PATH}")
  message(STATUS "USER_PROGRAM_PATH         - ${USER_PROGRAM_PATH}")

include(build/cmake/opensoar.cmake)
get_filename_component(OUTPUT_FOLDER ${PROJECTGROUP_BINARY_DIR}/.. ABSOLUTE)
set(OUTPUT ${OUTPUT_FOLDER})

# 2021 clear the right place:
set(JASPER_OUTSIDE      OFF)
set(ZZIP_OUTSIDE        OFF)

add_definitions(-DCMAKE_PROJECT)  # to decide if make or cmake...

# find all necessary programs/apps
find_program(PYTHON_APP         NAMES python3 python REQUIRED)
find_program(PERL_APP           NAMES perl   REQUIRED)
# find_program(BMP_CONVERT_APP    NAMES ImageMagick/convert )
    list(APPEND CMAKE_PROGRAM_PATH "${USER_PROGRAM_PATH}/GnuWin/libxslt/bin")
find_program(XSLTPROC_APP    NAMES xsltproc REQUIRED)
    list(APPEND CMAKE_PROGRAM_PATH "${USER_PROGRAM_PATH}/Inkscape/bin")
find_program(INKSCAPE_APP    NAMES inkscape REQUIRED)
    list(APPEND CMAKE_PROGRAM_PATH "${USER_PROGRAM_PATH}/ImageMagick")
find_program(BMP_CONVERT_APP    NAMES convert REQUIRED)
find_program(BMP_MONTAGE_APP    NAMES montage REQUIRED)
    list(APPEND CMAKE_PROGRAM_PATH "${USER_PROGRAM_PATH}/7-Zip")
find_program(ZIP_APP            NAMES 7z gzip REQUIRED)  # REQUIRED since CMake 3.18!

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set (PERCENT_CHAR "\%")
    set (DOLLAR_CHAR "$$")
#    message(FATAL_ERROR "Stop: Clang Compiler??? - '${CMAKE_CXX_COMPILER}'")
elseif(CMAKE_CXX_COMPILER MATCHES "clang[+][+]")
    set (PERCENT_CHAR "\%")
    set (DOLLAR_CHAR "$$")
    set(CLANG ON)
elseif (WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
    message(STATUS "MSVC-Compiler - ${CMAKE_CXX_COMPILER_ID}")
    set (PERCENT_CHAR "%%")
    set (DOLLAR_CHAR "\$")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "GCC-Compiler - ${CMAKE_CXX_COMPILER_ID}")
    set (PERCENT_CHAR "%%")
    set (DOLLAR_CHAR "$$")
else()
  ### message(FATAL_ERROR "GDI-Stop!  in ./CmakeLists")
  message(FATAL_ERROR "Stop: unknown Compiler??? - '${CMAKE_CXX_COMPILER}'")
endif()

add_subdirectory(build/cmake)
# add_subdirectory(Data)
# add_subdirectory(po)

if(UNIX)
 set(ENABLE_OPENGL ON)  # better outside????
  # moved to build/cmake/LinuxGCC.cmake
elseif (MSVC OR 1)  # 1 = always..
# if (MSVC AND 0)  # 0 = never..
### August, 2022-09-19:
 set(ENABLE_OPENGL OFF)  # better outside????
 set(USE_MEMORY_CANVAS OFF)  # das ist hier auch falsch!!!!
else()
 set(ENABLE_OPENGL ON)  # better outside????
 set(ENABLE_SDL OFF)  # better outside????
 set(USE_MEMORY_CANVAS OFF)  # das ist hier auch falsch!!!!
endif()
# Target links to target GLUT::GLUT but the target was not

#---------------------------------------------------------
if (USE_MEMORY_CANVAS)
       add_compile_definitions(USE_MEMORY_CANVAS)
elseif(ENABLE_OPENGL)
       add_compile_definitions(ENABLE_OPENGL)
       # add_compile_definitions(ENABLE_SDL)  # only windows????
       # add_compile_definitions(USE_FREETYPE)  # only windows????
else()
      set(USE_GDI ON)  # das ist hier auch falsch!!!!
      add_compile_definitions(USE_GDI)
      add_compile_definitions(USE_WINUSER)
endif()
#---------------------------------------------------------

set_property(GLOBAL PROPERTY USE_FOLDERS ON)  # important for folder structure f.e. in Visual Studio
# Setting Area ===================================================
include(ExternalProject)  # possible since 3.10

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # only by Makefile or Ninja generators: export compile commands
set(XCSOAR_LIB_TYPE STATIC)  # SHARED)

if (CMAKE_TOOLCHAIN_FILE)
  message(STATUS "CMAKE_TOOLCHAIN_FILE = '${CMAKE_TOOLCHAIN_FILE}'")
  message(STATUS "CMAKE_SYSTEM_NAME = '${CMAKE_SYSTEM_NAME}'")
endif()

if(ANDROID OR KOBO OR SDL)
    add_compile_definitions(HAVE_BATTERY)
endif()

get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
  
message(STATUS "CMAKE_HOST_SYSTEM_NAME = ${CMAKE_HOST_SYSTEM_NAME} vs. CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME} ")

set(3RDPARTY_DEPENDENCIES ON)
set(3RDPARTY_TARGETS )
add_subdirectory(3rd_party)
add_subdirectory(Data)
add_subdirectory(po)

set(SRC "${PROJECTGROUP_SOURCE_DIR}/src")
include(CMakeSource.cmake)

if (CMAKE_INCLUDE_SYSTEM)
  include(${CMAKE_INCLUDE_SYSTEM})  # build/cmake/LinuxMinGW.cmake)
elseif (WIN32 AND MSVC)
   include(build/cmake/WinMSVC.cmake)
 elseif(WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
   include(build/cmake/WinClang.cmake)
   set(CLANG ON)
 elseif(WIN32 AND (CMAKE_CXX_COMPILER MATCHES "clang[+][+]"))
   message(FATAL_ERROR "+++ Unknown System: CLANG-1!")
   include(build/cmake/WinClang.cmake)
   set(CLANG ON)
 elseif(${CMAKE_HOST_SYSTEM_NAME}' MATCHES 'Linux') # August2111: Test only
   include(build/cmake/LinuxMinGW.cmake)
 elseif(WIN32 AND MINGW AND (${CMAKE_HOST_SYSTEM_NAME}' MATCHES 'Linux'))
   include(build/cmake/LinuxMinGW.cmake)
 elseif(WIN32 AND MINGW)
   include(build/cmake/WinMinGW.cmake)
 elseif(WIN32 AND CLANG)
    message(FATAL_ERROR "+++ Unknown System: CLANG!")
 elseif(WIN32 AND NINJA)
    message(FATAL_ERROR "+++ Unknown System: NINJA!")
 elseif(UNIX)
   message(STATUS "+++ System = LINUX / GCC(?) (${TOOLCHAIN})!")
   include(build/cmake/LinuxGCC.cmake)
 elseif(ANDROID)
     message(STATUS "+++ System = ANDROID / ${ANDROID_ABI}!")
 else()
   message(FATAL_ERROR "+++ Unknown System: ${CMAKE_SYSTEM}!")
endif()

if (MSVC)  # Themes for MSVC only!
    add_compile_definitions(GNU_CONST=)
    add_compile_definitions(GNU_PURE=)
else()   ### MSVC
    add_compile_definitions("GNU_CONST=[[gnu::const]]")
    add_compile_definitions("GNU_PURE=[[gnu::pure]]")
endif()  ### MSVC

message(STATUS "+++ Lib Pre- and Suffix: '${LIB_PREFIX}' -- '${LIB_SUFFIX}' !")


include_directories("${SRC}/io") # only regarding Logfile!
include_directories("${SRC}/lib/fmt") # ???!
# include_directories("${OUTPUT_FOLDER}/include")

#3rd party components
add_compile_definitions(CURL_STATICLIB)

# git commit hash macro
execute_process(
  COMMAND  git rev-parse --short HEAD
  WORKING_DIRECTORY ${PROJECTGROUP_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_ID
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_compile_definitions("GIT_COMMIT_ID=\"${GIT_COMMIT_ID}\"")

# 3rd Party !!!!
#=============================
message(STATUS "### THIRDPARTY_INCLUDES = '${THIRDPARTY_INCLUDES}'")
include_directories(${THIRDPARTY_INCLUDES} )
#####################################################################################
#####################################################################################
add_compile_definitions(NOMINMAX)

if(ENABLE_OPENGL)
    message(FATAL_ERROR "Not funtional yet")
    add_compile_definitions(ENABLE_OPENGL)
    # set(FreeGLUT_DIR ${LINK_LIBS}/glut/freeglut-3.2.1/lib/msvc2019/cmake/FreeGLUT)
    # find_package(FreeGLUT REQUIRED)
    set(OpenGL_GL_PREFERENCE LEGACY)
    find_package(OpenGL REQUIRED)  # FindOpenGL() ???
    # set(GLEW_ROOT D:/link_libs/glew/glew-2.1.0/lib/Release/x64)
    # find_package(GLEW REQUIRED) 
    
    if(NOT TARGET OpenGL::GLU)
        message(FATAL_ERROR "GLU could not be found")
    endif(NOT TARGET OpenGL::GLU)
    
    message(STATUS "glu: ${OPENGL_glu_LIBRARY}") 
    message(STATUS "glu: ${OpenGL_INCLUDE_DIR}") 
    
    get_target_property(GLU_IMPORTED_LIBNAME OpenGL::GLU IMPORTED_LIBNAME)
    message(STATUS "glu imported libname: ${GLU_IMPORTED_LIBNAME}")
    add_compile_definitions(USE_WIN32_RESOURCES) #  USE_WINUSER) # USE_WINUSER entspricht USE_GDI!

    if (ANDROID)
      #  this is a hard coded path!
      include_directories(D:/Programs/Android/android-ndk-r25b/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/GLES2)
    else()
      #  this is a hard coded path!
      ## include_directories(D:/Programs/Android/android-ndk-r25b/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include)
    endif()
    include_directories(${Boost_INCLUDE_DIR}/boost/compute/interop)

    include_directories($ENV{OCL_ROOT}/include)
    include_directories(D:/Programs/OCL_SDK_Light/include) # alternative???

  if (MSVC)
     message(FATAL_ERROR "OpenGL-Stop!  OpenGL_INCLUDE_DIR = ${OpenGL_INCLUDE_DIR}")
  endif()
  set(OPENGL_DEFINED ON)
else()
      add_compile_definitions(USE_WIN32_RESOURCES USE_WINUSER)
endif()
#####################################################################################
#####################################################################################

link_directories(${BOOST_ROOT}/lib/${TOOLCHAIN}) # this line in boost.cmake is not affected to the main project!

get_filename_component(TEMP_INCLUDES ${PROJECTGROUP_BINARY_DIR}/../include ABSOLUTE)
include_directories(
    ${PROJECTGROUP_SOURCE_DIR}/src
    ${PROJECTGROUP_SOURCE_DIR}/src/Engine
    ${TEMP_INCLUDES}

    # better with find_package
    ${Boost_INCLUDE_DIR}/boost/predef/other

    ${THIRD_PARTY}/glfw/glfw-3.3.2.bin.WIN64/include
    ${LINK_LIBS}/glm/glm-0.9.9/include
)

if (JASPER_OUTSIDE)  # 2021
    # before add_subdirectories!!!
    add_compile_definitions(JASPER_OUTSIDE)
endif (JASPER_OUTSIDE)

set(SOURCE_FILES )  # clean it
    add_subdirectory(src/Airspace)
    add_subdirectory(src/Atmosphere)
    add_subdirectory(src/Audio)
    add_subdirectory(src/Blackboard)
    add_subdirectory(src/co)
    add_subdirectory(src/Computer)
if (OFF) ## - 'Cloud'- files are not used yet
   add_subdirectory(src/Cloud)
endif()
    add_subdirectory(src/CrossSection)
    add_subdirectory(src/Engine)
    add_subdirectory(src/event)
    add_subdirectory(src/FLARM)
    add_subdirectory(src/Form)
    add_subdirectory(src/Formatter)
    add_subdirectory(src/Gauge)
    add_subdirectory(src/Geo)
    add_subdirectory(src/Hardware)
    add_subdirectory(src/IGC)
    add_subdirectory(src/InfoBoxes)
    add_subdirectory(src/Input)
    add_subdirectory(src/Job)
    add_subdirectory(src/json)
    add_subdirectory(src/Language)
    add_subdirectory(src/Logger)
    add_subdirectory(src/Look)
    add_subdirectory(src/lua)
    add_subdirectory(src/Markers)
    add_subdirectory(src/Math)
    add_subdirectory(src/Menu)
    add_subdirectory(src/Monitor)
    add_subdirectory(src/net)
    add_subdirectory(src/NMEA)
    add_subdirectory(src/Operation)
    add_subdirectory(src/system)
    add_subdirectory(src/Plane)
    add_subdirectory(src/Polar)
    add_subdirectory(src/Profile)
    add_subdirectory(src/Projection)
    add_subdirectory(src/Renderer)
    add_subdirectory(src/Replay)
    add_subdirectory(src/Repository)
    add_subdirectory(src/Screen)
    add_subdirectory(src/Task)
    add_subdirectory(src/TeamCode)
    add_subdirectory(src/Terrain)
# if (NOT JASPER_OUTSIDE)  # 2021
    add_subdirectory(src/Terrain/jasper)  # 2021
# endif (NOT JASPER_OUTSIDE)
    add_subdirectory(src/thread)
    add_subdirectory(src/time)
    add_subdirectory(src/Topography)
    add_subdirectory(src/ui    )
    add_subdirectory(src/UIUtil)
    add_subdirectory(src/Units)
    add_subdirectory(src/util)
    add_subdirectory(src/Waypoint)
    add_subdirectory(src/Weather)
    add_subdirectory(src/Widget)
    add_subdirectory(src/XML)

    add_subdirectory(src/Device)
    add_subdirectory(src/Dialogs)
    add_subdirectory(src/io)
    add_subdirectory(src/MapWindow)
    add_subdirectory(src/Tracking)
    add_subdirectory(src/net/client/WeGlide)
if(NOT ZZIP_OUTSIDE)
    add_subdirectory(src/zzip)   # aug: new!!
endif()
    add_subdirectory(src/lib)
add_subdirectory(src)   # libOpenSoar!

list(APPEND SOURCE_FILES "src/OpenSoar.cpp")
# list(APPEND SOURCE_FILES "src/Version.cpp")
## configure_file(
##     "${CMAKE_CURRENT_SOURCE_DIR}/Data/OpenSoar.rc.in"
##     "${OUTPUT_FOLDER}/Data/OpenSoar.rc"
## )
# list(APPEND SOURCE_FILES "${OUTPUT_FOLDER}/Data/OpenSoar.rc")
list(APPEND SOURCE_FILES "${PROJECTGROUP_BINARY_DIR}/OpenSoar.rc")
## list(APPEND SCRIPT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Data/OpenSoar.rc.in")
list(APPEND SCRIPT_FILES "CMakeSource.cmake")

if (MSVC)
  file(GLOB DEF_FILE "Data/*.def")
  if (DEF_FILE)
    list(APPEND SOURCE_FILES ${DEF_FILE})
  endif()
endif()
#==========================================================
#==========================================================

# preparing target_link_libraries!
list(APPEND XCSOAR_LINK_LIBRARIES  libOpenSoar ${XCSOAR_SOURCE_LISTS})

# TODO(aug): Cleaning up this code!!!
if(1)
  message(STATUS "Boost:        BOOST_LIB       =  ${Boost_LIBRARIES} ")  # BOOST_LIB ???
  message(STATUS "C-Ares:       CARES_LIB       =  ${CARES_LIB}     / ${CARES_TARGET} ")
  message(STATUS "Curl:         CURL_LIB        =  ${CURL_LIB}      / ${CURL_TARGET} ")
  message(STATUS "Jasper:       JASPER_LIB      =  ${JASPER_LIB}    / ${JASPER_TARGET} ")
  ## message(STATUS "LibPng:       LIBPNG_LIB      =  ${LIBPNG_LIB} ")
  message(STATUS "Png:          PNG_LIB         =  ${PNG_LIB}       / ${PNG_TARGET} ")
  message(STATUS "Sodium:       SODIUM_LIB      =  ${SODIUM_LIB}    / ${SODIUM_TARGET} ")
  # message(STATUS "LibSodium:    LIBSODIUM_LIB   =  ${LIBSODIUM_LIB} ")
  message(STATUS "Lua:          LUA_LIB         =  ${LUA_LIB}       / ${LUA_TARGET} ")
  #message(STATUS "Lua_3rd:      LUA_LIB_3RD     =  ${LUA_LIB_3RD} ")
  message(STATUS "ZLib:         ZLIB_LIB        =  ${ZLIB_LIB}      / ${ZLIB_TARGET} ")
  message(STATUS "ZZip:         ZZIP_LIB        =  ${ZZIP_LIB}      / ${ZZIP_TARGET} ")
  message(STATUS "Fmt:          FMT_LIB         =  ${FMT_LIB}       / ${FMT_TARGET} ")
  # message(FATAL_ERROR "Stop: XCSOAR_LINK_LIBRARIES")
endif()

list(APPEND XCSOAR_LINK_LIBRARIES
   ${LUA_TARGET}
   ${CURL_TARGET}
###   ${FMT_TARGET}
   ${FMT_LIB}
#   ${MAPSERVER_TARGET}
   ${LIBPNG_TARGET}
   ${ZLIB_TARGET}
   ${ZZIP_TARGET}    # internal or external...
   ${SODIUM_TARGET}  # new at 06/2020
   ${SSL_LIB}  # new at 03/2021
   ${CRYPTO_LIB}  # new at 03/2021
   ${CARES_TARGET}  # new at 03/2021
   
   ${Boost_LIBRARIES}
)

if (WIN32)  # Windows only:
    # BASIC_LINK_LIBRARIES from Toolchain
    list(APPEND XCSOAR_LINK_LIBRARIES ${BASIC_LINK_LIBRARIES})
endif()
  #==========================================================
  #==========================================================

list(APPEND SCRIPT_FILES
    CMakeSource.cmake
    ${PROJECTGROUP_BINARY_DIR}/VERSION.txt
    OpenSoar.config
    OpenSoar-News.md
    OpenSoar-AddOn.md
    NEWS.txt
)

if (ANDROID)  # library!
   add_library(${TARGET_NAME} STATIC ${SOURCE_FILES})
elseif(MINGW)
   add_executable(${TARGET_NAME}     ${SOURCE_FILES} ${SCRIPT_FILES} ${EXT_PROJ}) #  ${XCSOAR_LINK_LIBRARIES})
else()        # executable!
   message(STATUS  "### add_executable(${TARGET_NAME}     ${SOURCE_FILES} ${SCRIPT_FILES} ${EXT_PROJ}")
   add_executable(${TARGET_NAME}     ${SOURCE_FILES} ${SCRIPT_FILES} ${EXT_PROJ})
endif()


list(APPEND XCSOAR_LINK_LIBRARIES ) # sequence is very important, I don't no, why'!!!!

target_link_libraries(${TARGET_NAME} PUBLIC ${XCSOAR_LINK_LIBRARIES})
if(MSVC)
  target_link_options(${TARGET_NAME} PUBLIC "/SUBSYSTEM:WINDOWS")
endif()

if(OPENGL_DEFINED)
    target_link_libraries(${TARGET_NAME}
        PRIVATE   # PUBLIC
        OpenGL::GL
        OpenGL::GLU # single colon - was previous!
    )
endif()

add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${directory} ${CMAKE_CURRENT_BINARY_DIR}/po
)

foreach(translation ${TRANSLATIONS})
        add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECTGROUP_SOURCE_DIR}/src/_Deprecated/${translation}.mo ${CMAKE_CURRENT_BINARY_DIR}/po/${translation}.mo
            # TODO(aug): later do the creation here...
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECTGROUP_SOURCE_DIR}/src/_Deprecated/${translation}.mo.c ${CMAKE_CURRENT_BINARY_DIR}/po/${translation}.mo.c
        )
endforeach()

# list(APPEND SOURCE_FILES "${PROJECTGROUP_BINARY_DIR}/OpenSoar.rc")
# target_sources(${TARGET_NAME} PRIVATE "${PROJECTGROUP_BINARY_DIR}/OpenSoar.rc")

add_dependencies(${TARGET_NAME} util libOpenSoar)

# Test-Folder !!!!
#=============================
    add_subdirectory(test/src)
    add_subdirectory(tools)
    message(STATUS "### Test  =====================")

# isn't possible: set_target_properties(ALL_BUILD PROPERTIES FOLDER _CMake)
# isn't possible: set_target_properties(ZERO_CHECK PROPERTIES FOLDER _CMake)
if (MSVC)
    message(STATUS "Configure:     ${PROJECTGROUP_BINARY_DIR}/${TARGET_NAME}.vcxproj.user")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/ide/msvc/OpenSoar.vcxproj.user.in"
        "${PROJECTGROUP_BINARY_DIR}/${TARGET_NAME}.vcxproj.user"
    )
endif()
