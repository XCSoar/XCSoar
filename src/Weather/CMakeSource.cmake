set(_SOURCES
        Weather/METARParser.cpp
        Weather/NOAADownloader.cpp
        Weather/NOAAFormatter.cpp
        Weather/NOAAGlue.cpp
        Weather/NOAAStore.cpp
        Weather/NOAAUpdater.cpp
        Weather/PCMet/Images.cpp
        Weather/PCMet/Overlays.cpp
#        Weather/Rasp/Providers.cpp
        Weather/Rasp/RaspCache.cpp
        Weather/Rasp/RaspRenderer.cpp
        Weather/Rasp/RaspStore.cpp
        Weather/Rasp/RaspStyle.cpp
)

set(SCRIPT_FILES
    CMakeSource.cmake
)

