# Build rules for the profile library

PROFILE_SOURCES = \
	$(SRC)/Profile/ProfileMap.cpp

ifeq ($(HAVE_WIN32),y)
PROFILE_SOURCES += $(SRC)/Profile/Registry.cpp
endif

ifeq ($(TARGET),UNIX)
PROFILE_SOURCES += $(SRC)/Profile/GConf.cpp
endif

PROFILE_OBJS = $(call SRC_TO_OBJ,$(PROFILE_SOURCES))
PROFILE_LIBS = $(TARGET_OUTPUT_DIR)/profile.a

$(PROFILE_LIBS): $(PROFILE_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
