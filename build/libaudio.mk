# Build rules for the PCM audio library

HAVE_PCM_PLAYER = n

ifeq ($(ENABLE_SDL),y)
HAVE_PCM_PLAYER = y
endif

ifeq ($(TARGET),ANDROID)
HAVE_PCM_PLAYER = y
endif

ifeq ($(ENABLE_ALSA),y)
HAVE_PCM_PLAYER = y
endif

ifeq ($(HAVE_PCM_PLAYER),y)

AUDIO_SRC_DIR = $(SRC)/Audio

AUDIO_SOURCES = \
	$(AUDIO_SRC_DIR)/ToneSynthesiser.cpp \
	$(AUDIO_SRC_DIR)/VarioSynthesiser.cpp \
	$(AUDIO_SRC_DIR)/PCMPlayer.cpp

ifeq ($(TARGET),ANDROID)
AUDIO_SOURCES += \
	$(AUDIO_SRC_DIR)/AndroidPCMPlayer.cpp \
	$(AUDIO_SRC_DIR)/SLES/Init.cpp
else

AUDIO_SOURCES += \
	$(AUDIO_SRC_DIR)/GlobalPCMResourcePlayer.cpp \
	$(AUDIO_SRC_DIR)/GlobalPCMMixer.cpp \
	$(AUDIO_SRC_DIR)/GlobalVolumeController.cpp \
	$(AUDIO_SRC_DIR)/MixerPCMPlayer.cpp \
	$(AUDIO_SRC_DIR)/PCMBufferDataSource.cpp \
	$(AUDIO_SRC_DIR)/PCMMixerDataSource.cpp \
	$(AUDIO_SRC_DIR)/PCMMixer.cpp \
	$(AUDIO_SRC_DIR)/PCMResourcePlayer.cpp \
	$(AUDIO_SRC_DIR)/VolumeController.cpp

ifeq ($(ENABLE_ALSA),y)
AUDIO_SOURCES += \
	$(AUDIO_SRC_DIR)/ALSAEnv.cpp \
	$(AUDIO_SRC_DIR)/ALSAPCMPlayer.cpp
else ifeq ($(ENABLE_SDL),y)
AUDIO_SOURCES += $(AUDIO_SRC_DIR)/SDLPCMPlayer.cpp
endif

endif

AUDIO_CPPFLAGS_INTERNAL = $(SDL_CPPFLAGS)
AUDIO_CPPFLAGS = $(ALSA_CPPFLAGS)
AUDIO_LDLIBS = $(ALSA_LDLIBS)

$(eval $(call link-library,libaudio,AUDIO))

endif
