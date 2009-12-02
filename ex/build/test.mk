TESTFAST = \
	Test/test_fixed.exe \
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

TESTSLOW = \
	Test/test_randomtask.exe \
	Test/test_vopt.exe \
	Test/test_cruiseefficiency.exe \
	Test/test_bestcruisetrack.exe \
	Test/test_aat.exe \
	Test/test_flight.exe 

TESTS = $(TESTFAST) $(TESTSLOW)

testslow:	$(TESTSLOW)
	$(Q)perl Test/testall.pl $(TESTSLOW)

testfast:	$(TESTFAST)
	$(Q)perl Test/testall.pl $(TESTFAST)

Test/%.exe: Test/%.cpp src/task.a src/harness.a
	@$(NQ)echo "  CXX/LN      $@"
	$(Q)$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ src/harness.a src/task.a 

