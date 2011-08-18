TEST_SRC = \
	$(SRC)/Computer/FlyingComputer.cpp \
	$(SRC)/Replay/IGCParser.cpp \
	$(SRC)/Replay/IgcReplay.cpp \
	$(SRC)/Replay/TaskAutoPilot.cpp \
	$(SRC)/Replay/AircraftSim.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/SettingsComputer.cpp \
	$(SRC)/Computer/TraceComputer.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/test_debug.cpp \
	$(TEST_SRC_DIR)/harness_aircraft.cpp \
	$(TEST_SRC_DIR)/harness_airspace.cpp \
	$(TEST_SRC_DIR)/harness_flight.cpp \
	$(TEST_SRC_DIR)/harness_flight2.cpp \
	$(TEST_SRC_DIR)/harness_waypoints.cpp \
	$(TEST_SRC_DIR)/harness_task.cpp \
	$(TEST_SRC_DIR)/harness_task2.cpp \
	$(TEST_SRC_DIR)/TaskEventsPrint.cpp \
	$(TEST_SRC_DIR)/tap.c

HARNESS_LIBS = $(TARGET_OUTPUT_DIR)/harness.a

$(HARNESS_LIBS): CPPFLAGS += -DDO_PRINT
$(HARNESS_LIBS): $(call SRC_TO_OBJ,$(TEST_SRC))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
