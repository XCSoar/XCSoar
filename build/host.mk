HOST_EXEEXT = $(findstring .exe,$(MAKE))
HOSTCC = $(LOCAL_TCPREFIX)gcc$(LOCAL_TCSUFFIX)$(HOST_EXEEXT)
HOSTCXX = $(LOCAL_TCPREFIX)g++$(LOCAL_TCSUFFIX)$(HOST_EXEEXT)

HOST_CPPFLAGS = $(INCLUDES)
HOST_CXXFLAGS = $(OPTIMIZE) $(HOST_OPTIMIZE) $(HOST_CXX_FEATURES) $(CXX_WARNINGS)

ifeq ($(HOST_IS_WIN32),y)
HOST_CPPFLAGS += -DHAVE_MSVCRT
endif

host-cc-flags = $(DEPFLAGS) $(HOST_CFLAGS) $(HOST_CPPFLAGS)
host-cxx-flags = $(DEPFLAGS) $(HOST_CXXFLAGS) $(HOST_CPPFLAGS)
host-ld-libs = -lm -lstdc++

WRAPPED_HOST_CC = $(CCACHE) $(HOSTCC)
WRAPPED_HOST_CXX = $(CCACHE) $(HOSTCXX)

$(HOST_OUTPUT_DIR)/%.o: %.c | $(HOST_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  HOSTCC  $@"
	$(Q)$(WRAPPED_HOST_CC) -c $(host-cc-flags) -o $@ $^

$(HOST_OUTPUT_DIR)/%.o: %.cpp | $(HOST_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  HOSTCXX $@"
	$(Q)$(WRAPPED_HOST_CXX) -c $(host-cxx-flags) -o $@ $^

$(HOST_OUTPUT_DIR)/%$(HOST_EXEEXT): $(HOST_OUTPUT_DIR)/%.o
	@$(NQ)echo "  HOSTLD  $@"
	$(Q)$(WRAPPED_HOST_CC) $^ $(host-ld-libs) -o $@
