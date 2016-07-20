# Build rules for the PCM audio library

HAVE_PCM_PLAYER = n

ifeq ($(ENABLE_SDL),y)
ifeq ($(TARGET_IS_KOBO),n)
HAVE_PCM_PLAYER = y
endif
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
else ifeq ($(ENABLE_ALSA),y)
AUDIO_SOURCES += $(AUDIO_SRC_DIR)/ALSAPCMPlayer.cpp
else ifeq ($(ENABLE_SDL),y)
AUDIO_SOURCES += $(AUDIO_SRC_DIR)/SDLPCMPlayer.cpp
endif

AUDIO_CPPFLAGS_INTERNAL = $(SDL_CPPFLAGS)
AUDIO_CPPFLAGS = $(ALSA_CPPFLAGS)
AUDIO_LDLIBS = $(ALSA_LDLIBS)

$(eval $(call link-library,libaudio,AUDIO))

endif
