TESTFAST = \
	$(TEST_SRC_DIR)/test_olc.exe \
	$(TEST_SRC_DIR)/test_fixed.exe \
	$(TEST_SRC_DIR)/test_waypoints.exe \
	$(TEST_SRC_DIR)/test_edittp.exe \
	$(TEST_SRC_DIR)/test_pressure.exe \
	$(TEST_SRC_DIR)/test_task.exe \
	$(TEST_SRC_DIR)/test_mc.exe \
	$(TEST_SRC_DIR)/test_modes.exe \
	$(TEST_SRC_DIR)/test_automc.exe \
	$(TEST_SRC_DIR)/test_trees.exe \
	$(TEST_SRC_DIR)/test_acfilter.exe \
	$(TEST_SRC_DIR)/test.exe \
	$(TEST_SRC_DIR)/test_airspace.exe \
	$(TEST_SRC_DIR)/test_highterrain.exe

ifeq ($(HAVE_WIN32),y)
TESTFAST += $(TEST_SRC_DIR)/test_win32.exe
endif

TESTSLOW = \
	$(TEST_SRC_DIR)/test_effectivemc.exe \
	$(TEST_SRC_DIR)/test_randomtask.exe \
	$(TEST_SRC_DIR)/test_vopt.exe \
	$(TEST_SRC_DIR)/test_cruiseefficiency.exe \
	$(TEST_SRC_DIR)/test_bestcruisetrack.exe \
	$(TEST_SRC_DIR)/test_aat.exe \
	$(TEST_SRC_DIR)/test_flight.exe

TESTS = $(TESTFAST:.exe=-$(TARGET).exe) $(TESTSLOW:.exe=-$(TARGET).exe) 

testslow:	$(TESTSLOW:.exe=-$(TARGET).exe)
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(TESTSLOW:.exe=-$(TARGET).exe)

testfast:	$(TESTFAST:.exe=-$(TARGET).exe)
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(TESTFAST:.exe=-$(TARGET).exe)

TESTLIBS = $(TEST_SRC_DIR)/harness-$(TARGET).a \
	   $(ENGINE_SRC_DIR)/task-$(TARGET).a 

$(TEST_SRC_DIR)/%-$(TARGET).exe: $(TEST_SRC_DIR)/%.cpp $(TESTLIBS)
	@$(NQ)echo "  CXX/LN      $@"
	$(Q)$(CXX) -o $@ $(cxx-flags) $(INCLUDES) $^

