set(_SOURCES
        system/EventPipe.cxx
        system/FileUtil.cpp
        system/Path.cpp
        system/PathName.cpp
        system/Process.cpp
        system/RunFile.cpp
        system/SystemLoad.cpp
)
if(UNIX)
  list(APPEND _SOURCES
##        system/EventPipe.cpp
  )
endif()

set(SCRIPT_FILES
    CMakeSource.cmake
)
