TESTFAST = \
	Test/test_fixed.exe \
	Test/test_pressure.exe \
	Test/test_waypoints.exe \
	Test/test_task.exe \
	Test/test_mc.exe \
	Test/test_airspace.exe \
	Test/test_automc.exe \
	Test/test_modes.exe \
	Test/test_trees.exe \
	Test/test.exe \
	Test/test_edittp.exe \
	Test/test_highterrain.exe

ifeq ($(HAVE_WIN32),y)
TESTFAST += Test/test_win32.exe
endif

TESTSLOW = \
	Test/test_randomtask.exe \
	Test/test_vopt.exe \
	Test/test_cruiseefficiency.exe \
	Test/test_bestcruisetrack.exe \
	Test/test_aat.exe \
	Test/test_flight.exe 

TESTS = $(TESTFAST:.exe=-$(TARGET).exe) $(TESTSLOW:.exe=-$(TARGET).exe) 

testslow:	$(TESTSLOW:.exe=-$(TARGET).exe)
ifeq ($(COVERAGE),y)
	$(Q)$(COVSTART)
endif
	$(Q)perl Test/testall.pl $(TESTSLOW:.exe=-$(TARGET).exe)
ifeq ($(COVERAGE),y)
	$(Q)$(COVEND)
	$(Q)$(COVPROC)
endif

testfast:	$(TESTFAST:.exe=-$(TARGET).exe)
ifeq ($(COVERAGE),y)
	$(Q)$(COVSTART)
endif
	$(Q)perl Test/testall.pl $(TESTFAST:.exe=-$(TARGET).exe)
ifeq ($(COVERAGE),y)
	$(Q)$(COVEND)
	$(Q)$(COVPROC)
endif

Test/%-$(TARGET).exe: Test/%.cpp src/task-$(TARGET).a Test/harness-$(TARGET).a
	@$(NQ)echo "  CXX/LN      $@"
	$(Q)$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) $< -o $@ \
		Test/harness-$(TARGET).a \
		src/task-$(TARGET).a 

