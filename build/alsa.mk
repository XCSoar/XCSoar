ifeq ($(TARGET),ANDROID)
# Using ALSA directly makes no sense on Android
ENABLE_ALSA = n
else ifeq ($(ENABLE_SDL),y)
# If SDL is being used, we do not use ALSA directly
# unless it is explicitly activated
ENABLE_ALSA ?= n
else ifeq ($(TARGET_IS_LINUX),y)
# Enable ALSA for Linux by default
ENABLE_ALSA ?= y
else
# ALSA must be activated explicitly on all other platforms
ENABLE_ALSA ?= n
endif

ifeq ($(ENABLE_ALSA),y)

$(eval $(call pkg-config-library,ALSA,alsa))
ALSA_CPPFLAGS += -DENABLE_ALSA

endif
