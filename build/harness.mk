HARNESS_SOURCES = \
	$(SRC)/NMEA/MoreData.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/Computer/FlyingComputer.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Replay/IgcReplay.cpp \
	$(SRC)/Replay/TaskAutoPilot.cpp \
	$(SRC)/Replay/AircraftSim.cpp \
	$(SRC)/Computer/Settings.cpp \
	$(SRC)/Computer/Wind/Settings.cpp \
	$(SRC)/TeamCode/Settings.cpp \
	$(SRC)/Logger/Settings.cpp \
	$(SRC)/Tracking/TrackingSettings.cpp \
	$(SRC)/Computer/TraceComputer.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/AirspacePrinting.cpp \
	$(TEST_SRC_DIR)/TaskPrinting.cpp \
	$(TEST_SRC_DIR)/ContestPrinting.cpp \
	$(TEST_SRC_DIR)/test_debug.cpp \
	$(TEST_SRC_DIR)/harness_aircraft.cpp \
	$(TEST_SRC_DIR)/harness_airspace.cpp \
	$(TEST_SRC_DIR)/harness_flight.cpp \
	$(TEST_SRC_DIR)/harness_waypoints.cpp \
	$(TEST_SRC_DIR)/harness_task.cpp \
	$(TEST_SRC_DIR)/harness_task2.cpp \
	$(TEST_SRC_DIR)/TaskEventsPrint.cpp \
	$(TEST_SRC_DIR)/tap.c

$(eval $(call link-library,harness,HARNESS))
