set(_SOURCES
        lib/zlib/Error.cxx
        lib/zlib/GunzipReader.cxx
        io/async/AsioThread.cpp
        io/async/GlobalAsioThread.cpp
        io/BufferedOutputStream.cxx
        io/BufferedReader.cxx
        io/BufferedLineReader.cpp
        io/ConfiguredFile.cpp
        io/ConvertLineReader.cpp
        io/CSVLine.cpp
        io/DataFile.cpp
        io/FileCache.cpp
        io/FileLineReader.cpp
        io/FileOutputStream.cxx
        io/FileReader.cxx
        io/FileTransaction.cpp
        io/KeyValueFileReader.cpp
        io/KeyValueFileWriter.cpp
        io/MapFile.cpp
        io/StringConverter.cpp
        io/ZipArchive.cpp
        io/ZipLineReader.cpp
        io/ZipReader.cpp
        io/FileDescriptor.cxx
)
# if(UNIX)
#   list(APPEND _SOURCES
#         io/async/SignalListener.cpp
#   )
# endif()

set(SCRIPT_FILES
    CMakeSource.cmake
)
