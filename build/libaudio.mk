# Build rules for the PCM audio library

HAVE_PCM_PLAYER = n

ifeq ($(TARGET),UNIX)
HAVE_PCM_PLAYER = y
endif

ifeq ($(HAVE_PCM_PLAYER),y)

AUDIO_SRC_DIR = $(SRC)/Audio

AUDIO_SOURCES = \
	$(AUDIO_SRC_DIR)/ToneSynthesiser.cpp \
	$(AUDIO_SRC_DIR)/PCMPlayer.cpp

$(eval $(call link-library,libaudio,AUDIO))

endif
