HOST_EXEEXT = $(findstring .exe,$(MAKE))
HOSTCC = $(LOCAL_TCPREFIX)gcc$(LOCAL_TCSUFFIX)$(HOST_EXEEXT)
HOSTCXX = $(LOCAL_TCPREFIX)g++$(LOCAL_TCSUFFIX)$(HOST_EXEEXT)
HOSTCPP = $(LOCAL_TCPREFIX)cpp$(LOCAL_TCSUFFIX)$(HOST_EXEEXT)

ifeq ($(HOST_IS_WIN32),y)
HOST_CPPFLAGS = $(INCLUDES) $(CPPFLAGS) -DHAVE_MSVCRT
HOST_CXXFLAGS = $(OPTIMIZE) $(CXX_FEATURES) $(CXXFLAGS) -DHAVE_MSVCRT
else
HOST_CPPFLAGS = $(INCLUDES) $(CPPFLAGS)
HOST_CXXFLAGS = $(OPTIMIZE) $(CXX_FEATURES) $(CXXFLAGS)
endif
HOST_CFLAGS = $(OPTIMIZE) $(C_FEATURES) $(CFLAGS)

host-cc-flags = $(DEPFLAGS) $(HOST_CFLAGS) $(HOST_CPPFLAGS)
host-cxx-flags = $(DEPFLAGS) $(HOST_CXXFLAGS) $(HOST_CPPFLAGS)
host-ld-libs = -lm -lstdc++

$(HOST_OUTPUT_DIR)/%.o: %.c | $(HOST_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  HOSTCC  $@"
	$(Q)$(HOSTCC) -c $(host-cc-flags) -o $@ $^

$(HOST_OUTPUT_DIR)/%.o: %.cpp | $(HOST_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  HOSTCXX $@"
	$(Q)$(HOSTCXX) -c $(host-cxx-flags) -o $@ $^

$(HOST_OUTPUT_DIR)/%$(HOST_EXEEXT): $(HOST_OUTPUT_DIR)/%.o
	@$(NQ)echo "  HOSTLD  $@"
	$(Q)$(HOSTCC) $^ $(host-ld-libs) -o $@
