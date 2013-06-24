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

ifeq ($(HAVE_PCM_PLAYER),y)

AUDIO_SRC_DIR = $(SRC)/Audio

AUDIO_SOURCES = \
	$(AUDIO_SRC_DIR)/ToneSynthesiser.cpp \
	$(AUDIO_SRC_DIR)/VarioSynthesiser.cpp \
	$(AUDIO_SRC_DIR)/PCMPlayer.cpp

ifeq ($(TARGET),ANDROID)
AUDIO_SOURCES += \
	$(AUDIO_SRC_DIR)/SLES/Init.cpp
endif

$(eval $(call link-library,libaudio,AUDIO))

endif
