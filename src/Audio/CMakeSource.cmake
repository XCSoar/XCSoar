set(_SOURCES
        Audio/Settings.cpp
        Audio/Sound.cpp
        Audio/VarioSettings.cpp
)


if(UNIX)
  list(APPEND _SOURCES
        Audio/VarioGlue.cpp
        Audio/ToneSynthesiser.cpp
        Audio/VarioSynthesiser.cpp
        Audio/PCMPlayer.cpp
        Audio/GlobalPCMResourcePlayer.cpp
        Audio/GlobalPCMMixer.cpp
        Audio/GlobalVolumeController.cpp
        Audio/MixerPCMPlayer.cpp
        Audio/PCMBufferDataSource.cpp
        Audio/PCMMixerDataSource.cpp
        Audio/PCMMixer.cpp
        Audio/PCMResourcePlayer.cpp
        Audio/VolumeController.cpp
        Audio/ALSAEnv.cpp
        Audio/ALSAPCMPlayer.cpp
  )
endif()

set(SCRIPT_FILES
    CMakeSource.cmake
)

