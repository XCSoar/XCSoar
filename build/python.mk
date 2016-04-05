ifeq ($(MAKECMDGOALS),python)

name-to-so = $(patsubst %,$(ABI_BIN_DIR)/%.so,$(1))

python: $(call name-to-so,xcsoar)

PYTHON_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/IGC/IGCFix.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
        $(SRC)/Computer/Wind/Settings.cpp \
        $(SRC)/Computer/Wind/WindEKF.cpp \
        $(SRC)/Computer/Wind/WindEKFGlue.cpp \
        $(SRC)/Computer/Wind/CirclingWind.cpp \
        $(SRC)/Computer/Wind/Computer.cpp \
        $(SRC)/Computer/Wind/MeasurementList.cpp \
        $(SRC)/Computer/Wind/Store.cpp \
	$(TEST_SRC_DIR)/FlightPhaseDetector.cpp \
	$(PYTHON_SRC)/Flight/Flight.cpp \
	$(PYTHON_SRC)/Flight/DebugReplayVector.cpp \
	$(PYTHON_SRC)/Flight/FlightTimes.cpp \
	$(PYTHON_SRC)/Flight/DouglasPeuckerMod.cpp \
	$(PYTHON_SRC)/Flight/AnalyseFlight.cpp \
        $(PYTHON_SRC)/Tools/GoogleEncode.cpp \
	$(PYTHON_SRC)/PythonConverters.cpp \
	$(PYTHON_SRC)/PythonGlue.cpp \
	$(PYTHON_SRC)/Flight.cpp \
	$(PYTHON_SRC)/Airspaces.cpp \
	$(PYTHON_SRC)/Util.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskBehaviour.cpp \
	$(SRC)/Logger/Settings.cpp \
	$(SRC)/TeamCode/Settings.cpp \
	$(SRC)/Tracking/TrackingSettings.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideSettings.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(ENGINE_SRC_DIR)/Task/Ordered/Settings.cpp \
	$(ENGINE_SRC_DIR)/Task/Ordered/StartConstraints.cpp \
	$(ENGINE_SRC_DIR)/Task/Ordered/FinishConstraints.cpp \
	$(SRC)/Computer/Settings.cpp \
	$(SRC)/Computer/AutoQNH.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AbstractAirspace.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceAltitude.cpp \
	$(ENGINE_SRC_DIR)/Airspace/Airspace.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceIntersectionVisitor.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceIntersectSort.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspacePolygon.cpp \
	$(ENGINE_SRC_DIR)/Airspace/Airspaces.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceSorter.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceAircraftPerformance.cpp \
	$(ENGINE_SRC_DIR)/Airspace/Predicate/AirspacePredicate.cpp \
	$(SRC)/NMEA/Aircraft.cpp
PYTHON_LDADD = $(DEBUG_REPLAY_LDADD)
PYTHON_LDLIBS = $(shell python-config --ldflags)
PYTHON_DEPENDS = CONTEST WAYPOINT UTIL ZZIP GEO MATH TIME
PYTHON_CPPFLAGS = $(shell python-config --includes) \
	-I$(TEST_SRC_DIR) -Wno-write-strings
PYTHON_NO_LIB_PREFIX = y
$(eval $(call link-shared-library,xcsoar,PYTHON))

endif
