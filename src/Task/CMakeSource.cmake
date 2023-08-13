set(_SOURCES
        Task/DefaultTask.cpp
        Task/Deserialiser.cpp
        Task/FileProtectedTaskManager.cpp
        Task/LoadFile.cpp
        Task/MapTaskManager.cpp
        Task/ProtectedRoutePlanner.cpp
        Task/ProtectedTaskManager.cpp
        Task/RoutePlannerGlue.cpp
        Task/SaveFile.cpp
        Task/Serialiser.cpp
        Task/TaskFile.cpp
        Task/TaskFileIGC.cpp
        Task/TaskFileSeeYou.cpp
        Task/TaskFileXCSoar.cpp
        Task/TaskStore.cpp
        Task/TypeStrings.cpp
        Task/ValidationErrorStrings.cpp
        Task/XCTrackTaskFile.cpp  # add 7.38
        Task/XCTrackTaskDecoder.cpp  # add 7.38
        Task/PolylineDecoder.cpp  # add 7.38
)

set(SCRIPT_FILES
    CMakeSource.cmake
)
