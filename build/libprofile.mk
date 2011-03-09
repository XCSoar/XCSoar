# Build rules for the profile library

PROFILE_SOURCES = \
	$(SRC)/Profile/ProfileMap.cpp

PROFILE_OBJS = $(call SRC_TO_OBJ,$(PROFILE_SOURCES))
PROFILE_LIBS = $(TARGET_OUTPUT_DIR)/profile.a
PROFILE_LDLIBS =

$(PROFILE_LIBS): $(PROFILE_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
