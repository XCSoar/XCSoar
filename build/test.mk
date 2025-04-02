name-to-bin = $(patsubst %,$(TARGET_BIN_DIR)/%$(TARGET_EXEEXT),$(1))

MORE_SCREEN_SOURCES = \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(SRC)/Look/FontDescription.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Hardware/DisplayDPI.cpp
ifeq ($(TARGET_IS_KOBO),y)
MORE_SCREEN_SOURCES += \
	$(SRC)/Hardware/CPU.cpp \
	$(SRC)/Hardware/RotateDisplay.cpp
endif

TESTFAST = \
	test_fixed \
	TestWaypoints \
	test_pressure \
	test_mc \
	test_task \
	test_modes \
	test_automc \
	test_acfilter \
	test_vopt

TESTSLOW = \
	test_bestcruisetrack \
	test_airspace \
	test_effectivemc \
	test_cruiseefficiency \
	test_highterrain \
	test_randomtask \
	test_flight \
	test_aat

ifeq ($(TARGET_IS_ANDROID),n)
# These programs are broken on Android because they require Java code
TESTSLOW += \
	test_replay_olc
endif

HARNESS_PROGRAMS = $(TESTFAST) $(TESTSLOW)

build-harness: $(call name-to-bin,$(HARNESS_PROGRAMS))

testslow: $(call name-to-bin,$(TESTSLOW))
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(addprefix $(TARGET_BIN_DIR)/,$(TESTSLOW))

testfast: $(call name-to-bin,$(TESTFAST))
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(addprefix $(TARGET_BIN_DIR)/,$(TESTFAST))

TEST1_DEPENDS = HARNESS TASK ROUTE GLIDE CONTEST WAYPOINT AIRSPACE IO OS THREAD ZZIP GEO TIME MATH UTIL

define link-harness-program
$(1)_SOURCES = \
	$(SRC)/Engine/Trace/Point.cpp \
	$(SRC)/Engine/Trace/Trace.cpp \
	$(SRC)/Engine/Trace/Vector.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/NMEA/GPSState.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Formatter/AirspaceFormatter.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/$(1).cpp
$(1)_DEPENDS = $(TEST1_DEPENDS)
$$(eval $$(call link-program,$(1),$(1)))
endef

$(foreach name,$(HARNESS_PROGRAMS),$(eval $(call link-harness-program,$(name))))

TEST_NAMES = \
	test_fixed \
	TestWaypoints \
	test_pressure \
	test_task \
	TestOverwritingRingBuffer \
	TestDateTime TestRoughTime TestWrapClock \
	TestPolylineDecoder \
	TestTransponderCode \
	TestMath \
	TestMathTables \
	TestAngle TestARange \
	TestGrahamScan \
	TestUnits TestEarth TestSunEphemeris \
	TestValidity TestUTM \
	TestAllocatedGrid \
	TestRadixTree TestGeoBounds TestGeoClip \
	TestLogger TestGRecord TestClimbAvCalc \
	TestWaypointReader TestThermalBase \
	TestFlarmNet \
	TestColorRamp TestGeoPoint TestDiffFilter \
	TestFileUtil TestPolars TestCSVLine TestGlidePolar \
	test_replay_task TestProjection TestFlatPoint TestFlatLine TestFlatGeoPoint \
	TestMacCready TestOrderedTask TestAATPoint TestTaskSave\
	TestPlanes \
	TestTaskPoint \
	TestTaskWaypoint \
	TestTeamCode \
	TestZeroFinder \
	TestAirspaceParser \
	TestMETARParser \
	TestIGCParser \
	TestStrings TestUTF8 \
	TestCRC16 TestCRC8 \
	TestUnitsFormatter \
	TestGeoPointFormatter \
	TestHexColorFormatter \
	TestByteSizeFormatter \
	TestTimeFormatter \
	TestIGCFilenameFormatter \
	TestNMEAFormatter \
	TestLXNToIGC \
	TestLeastSquares \
	TestHexString \
	TestThermalBand \
	TestPackedFloat \
	TestVersionNumber

ifeq ($(TARGET_IS_ANDROID),n)
# These programs are broken on Android because they require Java code
TEST_NAMES += \
	TestProfile \
	TestDriver
endif

TESTS = $(call name-to-bin,$(TEST_NAMES))

TEST_HEX_STRING_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestHexString.cpp
$(eval $(call link-program,TestHexString,TEST_HEX_STRING))

TEST_CRC16_SOURCES = \
	$(SRC)/util/CRC16CCITT.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestCRC16.cpp
$(eval $(call link-program,TestCRC16,TEST_CRC16))

TEST_CRC8_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestCRC8.cpp
$(eval $(call link-program,TestCRC8,TEST_CRC8))

TEST_LEASTSQUARES_SOURCES = \
	$(SRC)/Math/LeastSquares.cpp \
	$(SRC)/Math/XYDataStore.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestLeastSquares.cpp
$(eval $(call link-program,TestLeastSquares,TEST_LEASTSQUARES))

TEST_THERMALBAND_SOURCES = \
$(ENGINE_SRC_DIR)/ThermalBand/ThermalBand.cpp \
$(ENGINE_SRC_DIR)/ThermalBand/ThermalSlice.cpp \
$(ENGINE_SRC_DIR)/ThermalBand/ThermalEncounterBand.cpp \
$(ENGINE_SRC_DIR)/ThermalBand/ThermalEncounterCollection.cpp \
$(TEST_SRC_DIR)/tap.c \
$(TEST_SRC_DIR)/TestThermalBand.cpp
$(eval $(call link-program,TestThermalBand,TEST_THERMALBAND))

TEST_OVERWRITING_RING_BUFFER_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestOverwritingRingBuffer.cpp
TEST_OVERWRITING_RING_BUFFER_DEPENDS = MATH
$(eval $(call link-program,TestOverwritingRingBuffer,TEST_OVERWRITING_RING_BUFFER))

TEST_IGC_PARSER_SOURCES = \
	$(SRC)/IGC/IGCParser.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestIGCParser.cpp
TEST_IGC_PARSER_DEPENDS = MATH UTIL
$(eval $(call link-program,TestIGCParser,TEST_IGC_PARSER))

TEST_METAR_PARSER_SOURCES = \
	$(SRC)/Weather/METARParser.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestMETARParser.cpp
TEST_METAR_PARSER_DEPENDS = MATH UTIL UNITS
$(eval $(call link-program,TestMETARParser,TEST_METAR_PARSER))

TEST_AIRSPACE_PARSER_SOURCES = \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestAirspaceParser.cpp
TEST_AIRSPACE_PARSER_LDADD = $(FAKE_LIBS)
TEST_AIRSPACE_PARSER_DEPENDS = IO OS AIRSPACE UNITS ZZIP GEO MATH UTIL UNITS
$(eval $(call link-program,TestAirspaceParser,TEST_AIRSPACE_PARSER))

TEST_DATE_TIME_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestDateTime.cpp
TEST_DATE_TIME_DEPENDS = MATH TIME
$(eval $(call link-program,TestDateTime,TEST_DATE_TIME))

TEST_POLYLINE_DECODER_SOURCES = \
	$(SRC)/Task/PolylineDecoder.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestPolylineDecoder.cpp
TEST_POLYLINE_DECODER_DEPENDS = GEO UTIL
$(eval $(call link-program,TestPolylineDecoder,TEST_POLYLINE_DECODER))

TEST_TRANSPONDER_CODE_SOURCES = \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestTransponderCode.cpp
TEST_TRANSPONDER_CODE_DEPENDS = MATH
$(eval $(call link-program,TestTransponderCode,TEST_TRANSPONDER_CODE))

TEST_ROUGH_TIME_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestRoughTime.cpp
TEST_ROUGH_TIME_DEPENDS = MATH TIME
$(eval $(call link-program,TestRoughTime,TEST_ROUGH_TIME))

TEST_WRAP_CLOCK_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestWrapClock.cpp
TEST_WRAP_CLOCK_DEPENDS = MATH TIME
$(eval $(call link-program,TestWrapClock,TEST_WRAP_CLOCK))

TEST_PROFILE_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/TestProfile.cpp
TEST_PROFILE_DEPENDS = PROFILE MATH IO OS UTIL
$(eval $(call link-program,TestProfile,TEST_PROFILE))

TEST_MAC_CREADY_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestMacCready.cpp
TEST_MAC_CREADY_OBJS = $(call SRC_TO_OBJ,$(TEST_MAC_CREADY_SOURCES))
TEST_MAC_CREADY_DEPENDS = GLIDE GEO MATH UTIL
$(eval $(call link-program,TestMacCready,TEST_MAC_CREADY))

TEST_ORDERED_TASK_SOURCES = \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestOrderedTask.cpp
TEST_ORDERED_TASK_OBJS = $(call SRC_TO_OBJ,$(TEST_ORDERED_TASK_SOURCES))
TEST_ORDERED_TASK_DEPENDS = TASK ROUTE GLIDE WAYPOINT GEO TIME MATH UTIL
$(eval $(call link-program,TestOrderedTask,TEST_ORDERED_TASK))

TEST_AAT_POINT_SOURCES = \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestAATPoint.cpp
TEST_AAT_POINT_OBJS = $(call SRC_TO_OBJ,$(TEST_AAT_POINT_SOURCES))
TEST_AAT_POINT_DEPENDS = TASK ROUTE GLIDE WAYPOINT GEO TIME MATH UTIL
$(eval $(call link-program,TestAATPoint,TEST_AAT_POINT))

TEST_TASK_SAVE_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/XML/Node.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(SRC)/Task/SaveFile.cpp \
	$(TEST_SRC_DIR)/TestTaskSave.cpp
TEST_TASK_SAVE_OBJS = $(call SRC_TO_OBJ,$(TEST_TASK_SAVE_SOURCES))
TEST_TASK_SAVE_DEPENDS = TASK TASKFILE ROUTE GLIDE WAYPOINT GEO TIME MATH UTIL XML
$(eval $(call link-program,TestTaskSave,TEST_TASK_SAVE))

TEST_PLANES_SOURCES = \
	$(SRC)/Polar/Parser.cpp \
	$(SRC)/Plane/PlaneFileGlue.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/TestPlanes.cpp
TEST_PLANES_DEPENDS = UNITS IO OS MATH UTIL
$(eval $(call link-program,TestPlanes,TEST_PLANES))

TEST_ZEROFINDER_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestZeroFinder.cpp
TEST_ZEROFINDER_DEPENDS = IO OS MATH
$(eval $(call link-program,TestZeroFinder,TEST_ZEROFINDER))

TEST_TASKPOINT_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestTaskPoint.cpp
TEST_TASKPOINT_DEPENDS = IO OS TASK GEO MATH
$(eval $(call link-program,TestTaskPoint,TEST_TASKPOINT))

TEST_TASKWAYPOINT_SOURCES = \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoint.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestTaskWaypoint.cpp
TEST_TASKWAYPOINT_DEPENDS = IO OS TASK GEO MATH UTIL
$(eval $(call link-program,TestTaskWaypoint,TEST_TASKWAYPOINT))

TEST_TEAM_CODE_SOURCES = \
	$(SRC)/TeamCode/TeamCode.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestTeamCode.cpp
TEST_TEAM_CODE_DEPENDS = GEO MATH UTIL
$(eval $(call link-program,TestTeamCode,TEST_TEAM_CODE))

TEST_TROUTE_SOURCES = \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/test_troute.cpp
TEST_TROUTE_DEPENDS = TERRAIN OPERATION IO ZZIP OS ROUTE GLIDE GEO MATH UTIL
$(eval $(call link-program,test_troute,TEST_TROUTE))

TEST_REACH_SOURCES = \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/test_reach.cpp
TEST_REACH_DEPENDS = TERRAIN OPERATION IO ZZIP OS ROUTE GLIDE GEO MATH UTIL
$(eval $(call link-program,test_reach,TEST_REACH))

TEST_ROUTE_SOURCES = \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/Formatter/AirspaceFormatter.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/AirspacePrinting.cpp \
	$(TEST_SRC_DIR)/harness_airspace.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/test_route.cpp
TEST_ROUTE_DEPENDS = TERRAIN OPERATION IO ZZIP OS ROUTE AIRSPACE GLIDE GEO MATH UTIL
$(eval $(call link-program,test_route,TEST_ROUTE))

TEST_REPLAY_TASK_SOURCES = \
	$(SRC)/Computer/FlyingComputer.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Replay/IgcReplay.cpp \
	$(SRC)/Replay/TaskAutoPilot.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/TaskPrinting.cpp \
	$(TEST_SRC_DIR)/TaskEventsPrint.cpp \
	$(TEST_SRC_DIR)/harness_task.cpp \
	$(TEST_SRC_DIR)/test_debug.cpp \
	$(TEST_SRC_DIR)/test_replay_task.cpp
TEST_REPLAY_TASK_DEPENDS = TASKFILE ROUTE WAYPOINT GLIDE LIBNMEA GEO MATH IO OS UTIL TIME UNITS
$(eval $(call link-program,test_replay_task,TEST_REPLAY_TASK))

TEST_MATH_TABLES_SOURCES = \
	$(SRC)/Computer/ThermalRecency.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestMathTables.cpp
TEST_MATH_TABLES_DEPENDS = MATH
$(eval $(call link-program,TestMathTables,TEST_MATH_TABLES))

TEST_ANGLE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestAngle.cpp
TEST_ANGLE_DEPENDS = MATH
$(eval $(call link-program,TestAngle,TEST_ANGLE))

TEST_ARANGE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestARange.cpp
TEST_ARANGE_DEPENDS = MATH
$(eval $(call link-program,TestARange,TEST_ARANGE))

TEST_MATH_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestLine2D.cpp \
	$(TEST_SRC_DIR)/TestQuadrilateral.cpp \
	$(TEST_SRC_DIR)/TestMath.cpp
QUADRILATERAL_ARANGE_DEPENDS = MATH
$(eval $(call link-program,TestMath,TEST_MATH))

TEST_GRAHAM_SCAN_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGrahamScan.cpp
TEST_GRAHAM_SCAN_DEPENDS = GEO MATH
$(eval $(call link-program,TestGrahamScan,TEST_GRAHAM_SCAN))

TEST_CSV_LINE_SOURCES = \
	$(SRC)/io/CSVLine.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestCSVLine.cpp
TEST_CSV_LINE_DEPENDS = MATH
$(eval $(call link-program,TestCSVLine,TEST_CSV_LINE))

TEST_GEO_BOUNDS_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGeoBounds.cpp
TEST_GEO_BOUNDS_DEPENDS = GEO MATH
$(eval $(call link-program,TestGeoBounds,TEST_GEO_BOUNDS))

TEST_FLARM_NET_SOURCES = \
	$(SRC)/FLARM/FlarmNetReader.cpp \
	$(SRC)/FLARM/Id.cpp \
	$(SRC)/FLARM/FlarmNetRecord.cpp \
	$(SRC)/FLARM/FlarmNetDatabase.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFlarmNet.cpp
TEST_FLARM_NET_DEPENDS = IO OS MATH UTIL
$(eval $(call link-program,TestFlarmNet,TEST_FLARM_NET))

TEST_GEO_CLIP_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGeoClip.cpp
TEST_GEO_CLIP_DEPENDS = GEO MATH
$(eval $(call link-program,TestGeoClip,TEST_GEO_CLIP))

TEST_CLIMB_AV_CALC_SOURCES = \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestClimbAvCalc.cpp
TEST_CLIMB_AV_CALC_DEPENDS = MATH
$(eval $(call link-program,TestClimbAvCalc,TEST_CLIMB_AV_CALC))

TEST_PROJECTION_SOURCES = \
	$(SRC)/Projection/Projection.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestProjection.cpp
TEST_PROJECTION_DEPENDS = MATH
TEST_PROJECTION_CPPFLAGS = $(SCREEN_CPPFLAGS)
$(eval $(call link-program,TestProjection,TEST_PROJECTION))

TEST_UNITS_SOURCES = \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Temperature.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUnits.cpp
TEST_UNITS_DEPENDS = MATH UNITS
$(eval $(call link-program,TestUnits,TEST_UNITS))

TEST_UNITS_FORMATTER_SOURCES = \
	$(SRC)/Formatter/Units.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUnitsFormatter.cpp
TEST_UNITS_FORMATTER_DEPENDS = MATH UTIL UNITS
$(eval $(call link-program,TestUnitsFormatter,TEST_UNITS_FORMATTER))

TEST_GEO_POINT_FORMATTER_SOURCES = \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGeoPointFormatter.cpp
TEST_GEO_POINT_FORMATTER_DEPENDS = GEO MATH UTIL
$(eval $(call link-program,TestGeoPointFormatter,TEST_GEO_POINT_FORMATTER))

TEST_HEX_COLOR_FORMATTER_SOURCES = \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestHexColorFormatter.cpp
TEST_HEX_COLOR_FORMATTER_DEPENDS = MATH SCREEN EVENT UTIL
$(eval $(call link-program,TestHexColorFormatter,TEST_HEX_COLOR_FORMATTER))

TEST_BYTE_SIZE_FORMATTER_SOURCES = \
	$(SRC)/Formatter/ByteSizeFormatter.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestByteSizeFormatter.cpp
TEST_BYTE_SIZE_FORMATTER_DEPENDS = MATH UTIL
$(eval $(call link-program,TestByteSizeFormatter,TEST_BYTE_SIZE_FORMATTER))

TEST_TIME_FORMATTER_SOURCES = \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestTimeFormatter.cpp
TEST_TIME_FORMATTER_DEPENDS = MATH UTIL TIME
$(eval $(call link-program,TestTimeFormatter,TEST_TIME_FORMATTER))

TEST_IGC_FILENAME_FORMATTER_SOURCES = \
	$(SRC)/Formatter/IGCFilenameFormatter.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestIGCFilenameFormatter.cpp
TEST_IGC_FILENAME_FORMATTER_DEPENDS = MATH UTIL TIME
$(eval $(call link-program,TestIGCFilenameFormatter,TEST_IGC_FILENAME_FORMATTER))

TEST_NMEA_FORMATTER_SOURCES = \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Driver/FLARM/StaticParser.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/Id.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/TestNMEAFormatter.cpp
TEST_NMEA_FORMATTER_DEPENDS = LIBNMEA GEO MATH IO UTIL TIME UNITS
$(eval $(call link-program,TestNMEAFormatter,TEST_NMEA_FORMATTER))

TEST_STRINGS_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestStrings.cpp
TEST_STRINGS_DEPENDS = UTIL
$(eval $(call link-program,TestStrings,TEST_STRINGS))

TEST_UTF8_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUTF8.cpp
TEST_UTF8_DEPENDS = UTIL
$(eval $(call link-program,TestUTF8,TEST_UTF8))

TEST_POLARS_SOURCES = \
	$(SRC)/Polar/Shape.cpp \
	$(SRC)/Polar/Polar.cpp \
	$(SRC)/Polar/Parser.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlidePolar.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideResult.cpp \
	$(SRC)/Polar/PolarFileGlue.cpp \
	$(SRC)/Polar/PolarStore.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestPolars.cpp
TEST_POLARS_DEPENDS = IO OS MATH UTIL UNITS
$(eval $(call link-program,TestPolars,TEST_POLARS))

TEST_GLIDE_POLAR_SOURCES = \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlidePolar.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideResult.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideState.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/MacCready.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGlidePolar.cpp
TEST_GLIDE_POLAR_DEPENDS = GEO MATH IO UNITS
$(eval $(call link-program,TestGlidePolar,TEST_GLIDE_POLAR))

TEST_FILE_UTIL_SOURCES = \
	$(SRC)/system/FileUtil.cpp \
	$(SRC)/system/Path.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFileUtil.cpp
TEST_FILE_UTIL_DEPENDS = UTIL
$(eval $(call link-program,TestFileUtil,TEST_FILE_UTIL))

TEST_GEO_POINT_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGeoPoint.cpp
TEST_GEO_POINT_DEPENDS = GEO MATH
$(eval $(call link-program,TestGeoPoint,TEST_GEO_POINT))

TEST_DIFF_FILTER_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestDiffFilter.cpp
TEST_DIFF_FILTER_DEPENDS = MATH
$(eval $(call link-program,TestDiffFilter,TEST_DIFF_FILTER))

TEST_FLAT_POINT_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFlatPoint.cpp
TEST_FLAT_POINT_DEPENDS = GEO MATH
$(eval $(call link-program,TestFlatPoint,TEST_FLAT_POINT))

TEST_FLAT_GEO_POINT_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFlatGeoPoint.cpp
TEST_FLAT_GEO_POINT_DEPENDS = GEO MATH
$(eval $(call link-program,TestFlatGeoPoint,TEST_FLAT_GEO_POINT))

TEST_FLAT_LINE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFlatLine.cpp
TEST_FLAT_LINE_DEPENDS = GEO MATH
$(eval $(call link-program,TestFlatLine,TEST_FLAT_LINE))

TEST_THERMALBASE_SOURCES = \
	$(SRC)/Computer/ThermalBase.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestThermalBase.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp
TEST_THERMALBASE_DEPENDS = GEO MATH THREAD
$(eval $(call link-program,TestThermalBase,TEST_THERMALBASE))

TEST_EARTH_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestEarth.cpp
TEST_EARTH_DEPENDS = GEO MATH
$(eval $(call link-program,TestEarth,TEST_EARTH))

TEST_COLOR_RAMP_SOURCES = \
	$(SRC)/ui/canvas/Ramp.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestColorRamp.cpp
TEST_COLOR_RAMP_CPPFLAGS = $(SCREEN_CPPFLAGS)
$(eval $(call link-program,TestColorRamp,TEST_COLOR_RAMP))

TEST_SUN_EPHEMERIS_SOURCES = \
	$(SRC)/Math/SunEphemeris.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestSunEphemeris.cpp
TEST_SUN_EPHEMERIS_DEPENDS = MATH
$(eval $(call link-program,TestSunEphemeris,TEST_SUN_EPHEMERIS))

TEST_UTM_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUTM.cpp
TEST_UTM_DEPENDS = GEO MATH
$(eval $(call link-program,TestUTM,TEST_UTM))

TEST_VALIDITY_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestValidity.cpp
$(eval $(call link-program,TestValidity,TEST_VALIDITY))

TEST_ALLOCATED_GRID_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestAllocatedGrid.cpp
TEST_ALLOCATED_GRID_DEPENDS = UTIL
$(eval $(call link-program,TestAllocatedGrid,TEST_ALLOCATED_GRID))

TEST_RADIX_TREE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestRadixTree.cpp
TEST_RADIX_TREE_DEPENDS = UTIL
$(eval $(call link-program,TestRadixTree,TEST_RADIX_TREE))

TEST_LOGGER_SOURCES = \
	$(SRC)/IGC/IGCFix.cpp \
	$(SRC)/IGC/IGCWriter.cpp \
	$(SRC)/IGC/IGCString.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/Logger/LoggerFRecord.cpp \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/LoggerEPE.cpp \
	$(SRC)/util/MD5.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestLogger.cpp
TEST_LOGGER_DEPENDS = IO OS GEO MATH UTIL UNITS
$(eval $(call link-program,TestLogger,TEST_LOGGER))

TEST_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/util/MD5.cpp \
	$(SRC)/Version.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGRecord.cpp
TEST_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,TestGRecord,TEST_GRECORD))

TEST_DRIVER_SOURCES = \
	$(SRC)/Device/Port/NullPort.cpp \
	$(SRC)/Device/Port/Port.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/Id.cpp \
	$(SRC)/FLARM/Calculations.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/TransponderMode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoint.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/TestDriver.cpp
TEST_DRIVER_DEPENDS = DRIVER OPERATION LIBNMEA GEO MATH IO OS THREAD UTIL TIME
$(eval $(call link-program,TestDriver,TEST_DRIVER))

TEST_WAY_POINT_FILE_SOURCES = \
	$(SRC)/Waypoint/CupWriter.cpp \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestWaypointReader.cpp
TEST_WAY_POINT_FILE_DEPENDS = WAYPOINTFILE OPERATION GEO MATH IO ZZIP OS THREAD UTIL
$(eval $(call link-program,TestWaypointReader,TEST_WAY_POINT_FILE))

TEST_TRACE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(SRC)/Engine/Trace/Point.cpp \
	$(SRC)/Engine/Trace/Trace.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/TestTrace.cpp
TEST_TRACE_DEPENDS = IO OS GEO MATH UTIL
$(eval $(call link-program,TestTrace,TEST_TRACE))

FLIGHT_TABLE_SOURCES = \
	$(SRC)/IGC/IGCParser.cpp \
	$(TEST_SRC_DIR)/FlightTable.cpp
FLIGHT_TABLE_DEPENDS = GEO MATH IO OS UTIL
$(eval $(call link-program,FlightTable,FLIGHT_TABLE))

build-check: $(TESTS)

check: $(TESTS) | $(OUT)/test/dirstamp
	@$(NQ)echo "  TEST    $(notdir $(patsubst %$(TARGET_EXEEXT),%,$^))"
	$(Q)$(PERL) $(TEST_SRC_DIR)/testall.pl $(TESTS)

check-no-build: $(OUT)/test/dirstamp
	$(PERL) $(TEST_SRC_DIR)/testall.pl $(TESTS)

DEBUG_PROGRAM_NAMES = \
	test_reach \
	test_route \
	test_troute \
	TestTrace \
	FlightTable \
	BenchmarkProjection \
	BenchmarkFAITriangleSector \
	DumpTextInflate \
	DumpHexColor \
	RunXMLParser \
	ReadMO \
	RunMD5 RunSHA256 \
	ReadGRecord VerifyGRecord AppendGRecord FixGRecord \
	AddChecksum \
	LoadTopography LoadTerrain \
	RunHeightMatrix \
	RunInputParser \
	RunWaypointParser RunAirspaceParser \
	RunFlightParser \
	EnumeratePorts \
	lxn2igc \
	DebugDisplay \
	TaskInfo DumpTaskFile \
	DumpFlarmNet \
	RunRepositoryParser \
	NearestWaypoints \
	RunKalmanFilter1d \
	ArcApprox

ifeq ($(TARGET_IS_ANDROID),n)
# These programs are broken on Android because they require Java code
DEBUG_PROGRAM_NAMES += \
	RunTrace \
	RunContestAnalysis \
	RunWaveComputer \
	FlightPath \
	ReadProfileString ReadProfileInt \
	KeyCodeDumper \
	ReadPort RunPortHandler LogPort \
	SplicePorts \
	RunDeviceDriver RunDeclare RunFlightList RunDownloadFlight \
	RunEnableNMEA \
	CAI302Tool \
	RunIGCWriter \
	RunFlightLogger RunFlyingComputer \
	RunCirclingWind RunWindEKF RunWindComputer \
	RunExternalWind \
	RunTask \
	LoadImage ViewImage \
	RunCanvas \
	RunListControl \
	RunTextEntry RunNumberEntry RunDateEntry RunTimeEntry RunAngleEntry \
	RunGeoPointEntry \
	RunTerminal \
	RunRenderOZ \
	RunChartRenderer \
	RunWindArrowRenderer \
	RunHorizonRenderer \
	RunFinalGlideBarRenderer \
	RunFAITriangleSectorRenderer \
	RunFlightListRenderer \
	RunProgressWindow \
	RunJobDialog \
	RunAnalysis \
	RunAirspaceWarningDialog \
	RunProfileListDialog \
	TestNotify \
	FeedNMEA \
	FeedVega EmulateDevice \
	RunVegaSettings \
	RunFlarmUtils \
	RunLX1600Utils \
	IGC2NMEA
endif

ifeq ($(TARGET),UNIX)
DEBUG_PROGRAM_NAMES += \
	AnalyseFlight \
	FeedFlyNetData
endif

ifeq ($(TARGET),PC)
DEBUG_PROGRAM_NAMES += \
  FeedFlyNetData
endif

ifeq ($(HAVE_HTTP)$(TARGET_IS_ANDROID),yn)
DEBUG_PROGRAM_NAMES += DownloadFile \
	RunDownloadToFile \
	UploadFile \
	RunWeGlideClient \
	RunTimClient \
	RunNOAADownloader RunSkyLinesTracking RunLiveTrack24
endif

ifeq ($(TARGET_IS_LINUX),y)
DEBUG_PROGRAM_NAMES += RunWPASupplicant
endif

ifeq ($(HAVE_PCM_PLAYER)$(TARGET_IS_ANDROID),yn)
DEBUG_PROGRAM_NAMES += PlayTone PlayVario DumpVario
endif

ifeq ($(LUA),y)
DEBUG_PROGRAM_NAMES += RunLua
endif

DEBUG_PROGRAMS = $(call name-to-bin,$(DEBUG_PROGRAM_NAMES))

ifeq ($(LUA),y)
RUN_LUA_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunLua.cpp
RUN_LUA_DEPENDS = LUA LIBLUA LIBHTTP IO OS GEO MATH UTIL
$(eval $(call link-program,RunLua,RUN_LUA))
endif

DEBUG_REPLAY_SOURCES = \
	$(SRC)/Device/Port/Port.cpp \
	$(SRC)/Device/Port/NullPort.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceWarningConfig.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalBand.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalSlice.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalEncounterBand.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalEncounterCollection.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(SRC)/FLARM/Id.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/Computer/BasicComputer.cpp \
	$(SRC)/Computer/GroundSpeedComputer.cpp \
	$(SRC)/Computer/FlyingComputer.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/GlideSolvers/GlidePolar.cpp \
	$(SRC)/Engine/GlideSolvers/GlideResult.cpp \
	$(SRC)/Engine/Route/Config.cpp \
	$(SRC)/Engine/Task/Stats/TaskStats.cpp \
	$(SRC)/Engine/Task/Stats/CommonStats.cpp \
	$(SRC)/Engine/Task/Stats/ElementStat.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/DebugReplayIGC.cpp \
	$(TEST_SRC_DIR)/DebugReplayNMEA.cpp \
	$(TEST_SRC_DIR)/DebugReplay.cpp
DEBUG_REPLAY_DEPENDS = DRIVER ASYNC LIBNET IO OS THREAD TIME

BENCHMARK_PROJECTION_SOURCES = \
	$(SRC)/Projection/Projection.cpp \
	$(TEST_SRC_DIR)/BenchmarkProjection.cpp
BENCHMARK_PROJECTION_DEPENDS = MATH
BENCHMARK_PROJECTION_CPPFLAGS = $(SCREEN_CPPFLAGS)
$(eval $(call link-program,BenchmarkProjection,BENCHMARK_PROJECTION))

BENCHMARK_FAI_TRIANGLE_SECTOR_SOURCES = \
	$(ENGINE_SRC_DIR)/Task/Shapes/FAITriangleSettings.cpp \
	$(ENGINE_SRC_DIR)/Task/Shapes/FAITriangleArea.cpp \
	$(TEST_SRC_DIR)/BenchmarkFAITriangleSector.cpp
BENCHMARK_FAI_TRIANGLE_SECTOR_DEPENDS = GEO MATH
$(eval $(call link-program,BenchmarkFAITriangleSector,BENCHMARK_FAI_TRIANGLE_SECTOR))

DUMP_TEXT_FILE_SOURCES = \
	$(TEST_SRC_DIR)/DumpTextFile.cpp
DUMP_TEXT_FILE_DEPENDS = IO OS ZZIP UTIL
$(eval $(call link-program,DumpTextFile,DUMP_TEXT_FILE))

RUN_KALMAN_FILTER_1D_SOURCES = \
	$(TEST_SRC_DIR)/RunKalmanFilter1d.cpp
RUN_KALMAN_FILTER_1D_DEPENDS = IO OS MATH UTIL
$(eval $(call link-program,RunKalmanFilter1d,RUN_KALMAN_FILTER_1D))

ARC_APPROX_SOURCES = \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/ArcApprox.cpp
ARC_APPROX_DEPENDS = UTIL GEO MATH
$(eval $(call link-program,ArcApprox,ARC_APPROX))

DUMP_TEXT_INFLATE_SOURCES = \
	$(TEST_SRC_DIR)/DumpTextInflate.cpp
DUMP_TEXT_INFLATE_DEPENDS = IO OS ZLIB UTIL
$(eval $(call link-program,DumpTextInflate,DUMP_TEXT_INFLATE))

DUMP_HEX_COLOR_SOURCES = \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/DumpHexColor.cpp
DUMP_HEX_COLOR_DEPENDS = SCREEN EVENT UTIL
$(eval $(call link-program,DumpHexColor,DUMP_HEX_COLOR))

DEBUG_DISPLAY_SOURCES = \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/DebugDisplay.cpp
DEBUG_DISPLAY_DEPENDS = SCREEN IO OS MATH UTIL OS
$(eval $(call link-program,DebugDisplay,DEBUG_DISPLAY))

DOWNLOAD_FILE_SOURCES = \
	$(SRC)/Version.cpp \
	$(TEST_SRC_DIR)/DownloadFile.cpp
DOWNLOAD_FILE_DEPENDS = LIBHTTP ASYNC OS LIBNET OS IO THREAD UTIL
$(eval $(call link-program,DownloadFile,DOWNLOAD_FILE))

RUN_DOWNLOAD_TO_FILE_SOURCES = \
	$(SRC)/net/SocketError.cxx \
	$(SRC)/Version.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/RunDownloadToFile.cpp
RUN_DOWNLOAD_TO_FILE_DEPENDS = LIBHTTP ASYNC LIBNET OPERATION IO OS THREAD UTIL
$(eval $(call link-program,RunDownloadToFile,RUN_DOWNLOAD_TO_FILE))

UPLOAD_FILE_SOURCES = \
	$(SRC)/Version.cpp \
	$(TEST_SRC_DIR)/UploadFile.cpp
UPLOAD_FILE_DEPENDS = LIBHTTP ASYNC OS LIBNET OS IO UTIL
$(eval $(call link-program,UploadFile,UPLOAD_FILE))

RUN_TIM_CLIENT_SOURCES = \
	$(SRC)/Version.cpp \
	$(TEST_SRC_DIR)/RunTimClient.cpp
RUN_TIM_CLIENT_DEPENDS = LIBCLIENT JSON LIBHTTP ASYNC OS LIBNET IO UTIL
$(eval $(call link-program,RunTimClient,RUN_TIM_CLIENT))

RUN_WEGLIDE_CLIENT_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/net/SocketError.cxx \
	$(TEST_SRC_DIR)/RunWeGlideClient.cpp
RUN_WEGLIDE_CLIENT_DEPENDS = LIBCLIENT JSON TASKFILE ROUTE GLIDE WAYPOINT GEO TIME MATH LIBHTTP ASYNC LIBNET OPERATION IO OS UTIL FMT
$(eval $(call link-program,RunWeGlideClient,RUN_WEGLIDE_CLIENT))

RUN_NOAA_DOWNLOADER_SOURCES = \
	$(SRC)/net/SocketError.cxx \
	$(SRC)/Version.cpp \
	$(SRC)/Weather/NOAADownloader.cpp \
	$(SRC)/Weather/NOAAStore.cpp \
	$(SRC)/Weather/NOAAUpdater.cpp \
	$(SRC)/Weather/METARParser.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunNOAADownloader.cpp
RUN_NOAA_DOWNLOADER_DEPENDS = GEO MATH LIBHTTP ASYNC LIBNET OPERATION OS IO THREAD UTIL TIME UNITS
$(eval $(call link-program,RunNOAADownloader,RUN_NOAA_DOWNLOADER))

RUN_WPA_SUPPLICANT_SOURCES = \
	$(SRC)/Kobo/WPASupplicant.cpp \
	$(TEST_SRC_DIR)/RunWPASupplicant.cpp
RUN_WPA_SUPPLICANT_DEPENDS = LIBNET IO OS UTIL
$(eval $(call link-program,RunWPASupplicant,RUN_WPA_SUPPLICANT))

RUN_SL_TRACKING_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/net/SocketError.cxx \
	$(SRC)/Tracking/SkyLines/Client.cpp \
	$(SRC)/Tracking/SkyLines/Assemble.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/RunSkyLinesTracking.cpp
RUN_SL_TRACKING_DEPENDS = $(DEBUG_REPLAY_DEPENDS)
$(eval $(call link-program,RunSkyLinesTracking,RUN_SL_TRACKING))

RUN_LIVETRACK24_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/net/SocketError.cxx \
	$(SRC)/Tracking/LiveTrack24/SessionID.cpp \
	$(SRC)/Tracking/LiveTrack24/Client.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/RunLiveTrack24.cpp
RUN_LIVETRACK24_DEPENDS = LIBHTTP $(DEBUG_REPLAY_DEPENDS) CO ASYNC LIBNET OS IO THREAD GEO MATH UTIL
$(eval $(call link-program,RunLiveTrack24,RUN_LIVETRACK24))

RUN_REPOSITORY_PARSER_SOURCES = \
	$(SRC)/Repository/FileRepository.cpp \
	$(SRC)/Repository/Parser.cpp \
	$(TEST_SRC_DIR)/RunRepositoryParser.cpp
RUN_REPOSITORY_PARSER_DEPENDS = LIBNET IO OS UTIL
$(eval $(call link-program,RunRepositoryParser,RUN_REPOSITORY_PARSER))

RUN_XML_PARSER_SOURCES = \
	$(TEST_SRC_DIR)/RunXMLParser.cpp
RUN_XML_PARSER_DEPENDS = XML
$(eval $(call link-program,RunXMLParser,RUN_XML_PARSER))

READ_MO_SOURCES = \
	$(SRC)/Language/MOFile.cpp \
	$(TEST_SRC_DIR)/ReadMO.cpp
READ_MO_DEPENDS = IO UTIL
$(eval $(call link-program,ReadMO,READ_MO))

READ_PROFILE_STRING_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/ReadProfileString.cpp
READ_PROFILE_STRING_DEPENDS = PROFILE IO OS UTIL
$(eval $(call link-program,ReadProfileString,READ_PROFILE_STRING))

READ_PROFILE_INT_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/ReadProfileInt.cpp
READ_PROFILE_INT_DEPENDS = PROFILE IO OS UTIL
$(eval $(call link-program,ReadProfileInt,READ_PROFILE_INT))

RUN_MD5_SOURCES = \
	$(SRC)/util/MD5.cpp \
	$(TEST_SRC_DIR)/RunMD5.cpp
RUN_MD5_DEPENDS = IO OS UTIL
$(eval $(call link-program,RunMD5,RUN_MD5))

RUN_SHA256_SOURCES = \
	$(TEST_SRC_DIR)/RunSHA256.cpp
RUN_SHA256_DEPENDS = LIBSODIUM IO OS UTIL
$(eval $(call link-program,RunSHA256,RUN_SHA256))

READ_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/util/MD5.cpp \
	$(TEST_SRC_DIR)/ReadGRecord.cpp
READ_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,ReadGRecord,READ_GRECORD))

VERIFY_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/util/MD5.cpp \
	$(TEST_SRC_DIR)/VerifyGRecord.cpp
VERIFY_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,VerifyGRecord,VERIFY_GRECORD))

APPEND_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/util/MD5.cpp \
	$(TEST_SRC_DIR)/AppendGRecord.cpp
APPEND_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,AppendGRecord,APPEND_GRECORD))

FIX_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/util/MD5.cpp \
	$(TEST_SRC_DIR)/FixGRecord.cpp
FIX_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,FixGRecord,FIX_GRECORD))

ADD_CHECKSUM_SOURCES = \
	$(TEST_SRC_DIR)/AddChecksum.cpp
ADD_CHECKSUM_DEPENDS = IO
$(eval $(call link-program,AddChecksum,ADD_CHECKSUM))

KEY_CODE_DUMPER_SOURCES = \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Look/ButtonLook.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/KeyCodeDumper.cpp
KEY_CODE_DUMPER_LDADD = $(FAKE_LIBS)
KEY_CODE_DUMPER_DEPENDS = FORM SCREEN EVENT ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,KeyCodeDumper,KEY_CODE_DUMPER))

LOAD_TOPOGRAPHY_SOURCES = \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/system/Path.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/LoadTopography.cpp
ifeq ($(OPENGL),y)
LOAD_TOPOGRAPHY_SOURCES += \
	$(CANVAS_SRC_DIR)/opengl/Triangulate.cpp
endif
LOAD_TOPOGRAPHY_DEPENDS = TOPO RESOURCE GEO MATH THREAD IO SYSTEM UTIL ZZIP
LOAD_TOPOGRAPHY_CPPFLAGS = $(SCREEN_CPPFLAGS)
$(eval $(call link-program,LoadTopography,LOAD_TOPOGRAPHY))

LOAD_TERRAIN_SOURCES = \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/LoadTerrain.cpp
LOAD_TERRAIN_CPPFLAGS = $(SCREEN_CPPFLAGS)
LOAD_TERRAIN_DEPENDS = TERRAIN OPERATION GEO MATH OS IO ZZIP UTIL
$(eval $(call link-program,LoadTerrain,LOAD_TERRAIN))

RUN_HEIGHT_MATRIX_SOURCES = \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/RunHeightMatrix.cpp
RUN_HEIGHT_MATRIX_CPPFLAGS = $(SCREEN_CPPFLAGS)
RUN_HEIGHT_MATRIX_DEPENDS = TERRAIN OPERATION GEO MATH IO OS ZZIP UTIL
$(eval $(call link-program,RunHeightMatrix,RUN_HEIGHT_MATRIX))

RUN_INPUT_PARSER_SOURCES = \
	$(SRC)/Input/InputKeys.cpp \
	$(SRC)/Input/InputConfig.cpp \
	$(SRC)/Input/InputParser.cpp \
	$(SRC)/Menu/MenuData.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunInputParser.cpp
RUN_INPUT_PARSER_CPPFLAGS = $(SCREEN_CPPFLAGS)
RUN_INPUT_PARSER_DEPENDS = IO OS UTIL
$(eval $(call link-program,RunInputParser,RUN_INPUT_PARSER))

RUN_WAY_POINT_PARSER_SOURCES = \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/RadioFrequency.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunWaypointParser.cpp
RUN_WAY_POINT_PARSER_LDADD = $(FAKE_LIBS)
RUN_WAY_POINT_PARSER_DEPENDS = WAYPOINTFILE OPERATION IO OS THREAD ZZIP GEO MATH UTIL
$(eval $(call link-program,RunWaypointParser,RUN_WAY_POINT_PARSER))

NEAREST_WAYPOINTS_SOURCES = \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/RadioFrequency.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/NearestWaypoints.cpp
NEAREST_WAYPOINTS_LDADD = $(FAKE_LIBS)
NEAREST_WAYPOINTS_DEPENDS = WAYPOINTFILE OPERATION IO OS THREAD ZZIP GEO MATH UTIL
$(eval $(call link-program,NearestWaypoints,NEAREST_WAYPOINTS))

RUN_FLIGHT_PARSER_SOURCES = \
	$(SRC)/Logger/FlightParser.cpp \
	$(TEST_SRC_DIR)/RunFlightParser.cpp
RUN_FLIGHT_PARSER_LDADD = $(FAKE_LIBS)
RUN_FLIGHT_PARSER_DEPENDS = IO OS TIME UTIL
$(eval $(call link-program,RunFlightParser,RUN_FLIGHT_PARSER))

RUN_AIRSPACE_PARSER_SOURCES = \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/RunAirspaceParser.cpp
RUN_AIRSPACE_PARSER_LDADD = $(FAKE_LIBS)
RUN_AIRSPACE_PARSER_DEPENDS = AIRSPACE IO OS ZZIP GEO MATH UTIL UNITS
$(eval $(call link-program,RunAirspaceParser,RUN_AIRSPACE_PARSER))

ENUMERATE_PORTS_SOURCES = \
	$(TEST_SRC_DIR)/EnumeratePorts.cpp
ENUMERATE_PORTS_DEPENDS = PORT OS
$(eval $(call link-program,EnumeratePorts,ENUMERATE_PORTS))

READ_PORT_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/ReadPort.cpp
READ_PORT_DEPENDS = PORT ASYNC LIBNET OPERATION IO OS THREAD TIME UTIL
$(eval $(call link-program,ReadPort,READ_PORT))

RUN_PORT_HANDLER_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunPortHandler.cpp
RUN_PORT_HANDLER_DEPENDS = PORT ASYNC LIBNET OPERATION IO OS THREAD TIME UTIL
$(eval $(call link-program,RunPortHandler,RUN_PORT_HANDLER))

LOG_PORT_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/LogPort.cpp
LOG_PORT_DEPENDS = PORT ASYNC LIBNET OPERATION IO OS THREAD TIME UTIL
$(eval $(call link-program,LogPort,LOG_PORT))

SPLICE_PORTS_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/SplicePorts.cpp
SPLICE_PORTS_DEPENDS = PORT ASYNC LIBNET OPERATION IO OS THREAD TIME UTIL
$(eval $(call link-program,SplicePorts,SPLICE_PORTS))

RUN_DEVICE_DRIVER_SOURCES = \
	$(SRC)/FLARM/Id.cpp \
	$(SRC)/Device/Port/Port.cpp \
	$(SRC)/Device/Port/NullPort.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/FLARM/Calculations.cpp \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/RunDeviceDriver.cpp
RUN_DEVICE_DRIVER_DEPENDS = DRIVER OPERATION IO LIBNMEA OS THREAD GEO MATH UTIL TIME
$(eval $(call link-program,RunDeviceDriver,RUN_DEVICE_DRIVER))

RUN_DECLARE_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunDeclare.cpp
RUN_DECLARE_DEPENDS = DRIVER PORT LIBNMEA ASYNC LIBNET OPERATION IO OS THREAD WAYPOINT GEO TIME MATH UTIL
$(eval $(call link-program,RunDeclare,RUN_DECLARE))

RUN_ENABLE_NMEA_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunEnableNMEA.cpp
RUN_ENABLE_NMEA_DEPENDS = DRIVER PORT LIBNMEA GEO MATH ASYNC LIBNET OPERATION IO OS THREAD TIME UTIL
$(eval $(call link-program,RunEnableNMEA,RUN_ENABLE_NMEA))

RUN_VEGA_SETTINGS_SOURCES = \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunVegaSettings.cpp
RUN_VEGA_SETTINGS_DEPENDS = DRIVER PORT LIBNMEA ASYNC LIBNET OPERATION IO OS THREAD GEO MATH TIME UTIL
$(eval $(call link-program,RunVegaSettings,RUN_VEGA_SETTINGS))

RUN_FLARM_UTILS_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunFlarmUtils.cpp
RUN_FLARM_UTILS_DEPENDS = DRIVER PORT LIBNMEA ASYNC LIBNET OPERATION IO OS THREAD GEO MATH TIME UTIL
$(eval $(call link-program,RunFlarmUtils,RUN_FLARM_UTILS))

RUN_LX1600_UTILS_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunLX1600Utils.cpp
RUN_LX1600_UTILS_DEPENDS = DRIVER PORT LIBNMEA ASYNC LIBNET OPERATION IO OS THREAD GEO MATH TIME UTIL
$(eval $(call link-program,RunLX1600Utils,RUN_LX1600_UTILS))

RUN_FLIGHT_LIST_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunFlightList.cpp
RUN_FLIGHT_LIST_DEPENDS = DRIVER PORT LIBNMEA ASYNC LIBNET OPERATION IO OS THREAD GEO TIME MATH UTIL
$(eval $(call link-program,RunFlightList,RUN_FLIGHT_LIST))

RUN_DOWNLOAD_FLIGHT_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunDownloadFlight.cpp
RUN_DOWNLOAD_FLIGHT_DEPENDS = DRIVER PORT ASYNC LIBNMEA LIBNET OPERATION IO OS THREAD GEO TIME MATH UTIL
$(eval $(call link-program,RunDownloadFlight,RUN_DOWNLOAD_FLIGHT))

CAI302_TOOL_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/CAI302Tool.cpp
CAI302_TOOL_DEPENDS = DRIVER PORT LIBNMEA ASYNC LIBNET OPERATION THREAD IO OS TIME GEO MATH UTIL
$(eval $(call link-program,CAI302Tool,CAI302_TOOL))

TEST_LXN_TO_IGC_SOURCES = \
	$(SRC)/Device/Driver/LX/Convert.cpp \
	$(SRC)/Device/Driver/LX/LXN.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestLXNToIGC.cpp
TEST_LXN_TO_IGC_DEPENDS = IO OS UTIL
$(eval $(call link-program,TestLXNToIGC,TEST_LXN_TO_IGC))

LXN2IGC_SOURCES = \
	$(SRC)/Device/Driver/LX/Convert.cpp \
	$(SRC)/Device/Driver/LX/LXN.cpp \
	$(TEST_SRC_DIR)/lxn2igc.cpp
LXN2IGC_DEPENDS = IO OS UTIL
$(eval $(call link-program,lxn2igc,LXN2IGC))

RUN_IGC_WRITER_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Version.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(SRC)/FLARM/Calculations.cpp \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(SRC)/IGC/IGCFix.cpp \
	$(SRC)/IGC/IGCWriter.cpp \
	$(SRC)/IGC/IGCString.cpp \
	$(SRC)/IGC/Generator.cpp \
	$(SRC)/Logger/LoggerFRecord.cpp \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/LoggerEPE.cpp \
	$(SRC)/util/MD5.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/RunIGCWriter.cpp
RUN_IGC_WRITER_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL
$(eval $(call link-program,RunIGCWriter,RUN_IGC_WRITER))

RUN_FLIGHT_LOGGER_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Logger/FlightLogger.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunFlightLogger.cpp
RUN_FLIGHT_LOGGER_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL TIME
$(eval $(call link-program,RunFlightLogger,RUN_FLIGHT_LOGGER))

RUN_FLYING_COMPUTER_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/RunFlyingComputer.cpp
RUN_FLYING_COMPUTER_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL
$(eval $(call link-program,RunFlyingComputer,RUN_FLYING_COMPUTER))

RUN_CIRCLING_WIND_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Computer/Wind/CirclingWind.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/RunCirclingWind.cpp
RUN_CIRCLING_WIND_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL
$(eval $(call link-program,RunCirclingWind,RUN_CIRCLING_WIND))

RUN_WIND_EKF_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Computer/Wind/WindEKF.cpp \
	$(SRC)/Computer/Wind/WindEKFGlue.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/RunWindEKF.cpp
RUN_WIND_EKF_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL TIME
$(eval $(call link-program,RunWindEKF,RUN_WIND_EKF))

RUN_WIND_COMPUTER_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Computer/Wind/Settings.cpp \
	$(SRC)/Computer/Wind/WindEKF.cpp \
	$(SRC)/Computer/Wind/WindEKFGlue.cpp \
	$(SRC)/Computer/Wind/CirclingWind.cpp \
	$(SRC)/Computer/Wind/Computer.cpp \
	$(SRC)/Computer/Wind/MeasurementList.cpp \
	$(SRC)/Computer/Wind/Store.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/RunWindComputer.cpp
RUN_WIND_COMPUTER_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL TIME
$(eval $(call link-program,RunWindComputer,RUN_WIND_COMPUTER))

RUN_EXTERNAL_WIND_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/RunExternalWind.cpp
RUN_EXTERNAL_WIND_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL TIME
$(eval $(call link-program,RunExternalWind,RUN_EXTERNAL_WIND))

RUN_TASK_SOURCES = \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(DEBUG_REPLAY_SOURCES) \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunTask.cpp
RUN_TASK_DEPENDS = $(DEBUG_REPLAY_DEPENDS) TASKFILE WAYPOINTFILE GLIDE GEO MATH UTIL IO TIME
$(eval $(call link-program,RunTask,RUN_TASK))

RUN_TRACE_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideSettings.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/RunTrace.cpp
RUN_TRACE_DEPENDS = $(DEBUG_REPLAY_DEPENDS) UTIL LIBNMEA GEO MATH TIME
$(eval $(call link-program,RunTrace,RUN_TRACE))

RUN_CONTEST_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/ContestPrinting.cpp \
	$(TEST_SRC_DIR)/RunContestAnalysis.cpp
RUN_CONTEST_DEPENDS = $(DEBUG_REPLAY_DEPENDS) CONTEST UTIL GEO MATH TIME
$(eval $(call link-program,RunContestAnalysis,RUN_CONTEST))

RUN_WAVE_COMPUTER_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Computer/WaveComputer.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(TEST_SRC_DIR)/RunWaveComputer.cpp
RUN_WAVE_COMPUTER_DEPENDS = $(DEBUG_REPLAY_DEPENDS) UTIL GEO MATH TIME
$(eval $(call link-program,RunWaveComputer,RUN_WAVE_COMPUTER))

ANALYSE_FLIGHT_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/TransponderCode.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalBand.cpp \
    $(ENGINE_SRC_DIR)/ThermalBand/ThermalSlice.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalEncounterBand.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalEncounterCollection.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/ContestPrinting.cpp \
	$(TEST_SRC_DIR)/FlightPhaseJSON.cpp \
	$(TEST_SRC_DIR)/FlightPhaseDetector.cpp \
	$(TEST_SRC_DIR)/AnalyseFlight.cpp
ANALYSE_FLIGHT_DEPENDS = $(DEBUG_REPLAY_DEPENDS) CONTEST JSON UTIL GEO MATH TIME
$(eval $(call link-program,AnalyseFlight,ANALYSE_FLIGHT))

FLIGHT_PATH_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/TransponderCode.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideSettings.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/FlightPath.cpp
FLIGHT_PATH_DEPENDS = $(DEBUG_REPLAY_DEPENDS) UTIL GEO MATH TIME
$(eval $(call link-program,FlightPath,FLIGHT_PATH))

LOAD_IMAGE_SOURCES = \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/LoadImage.cpp
LOAD_IMAGE_LDADD = $(FAKE_LIBS)
LOAD_IMAGE_DEPENDS = SCREEN RESOURCE EVENT ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,LoadImage,LOAD_IMAGE))

VIEW_IMAGE_SOURCES = \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/ViewImage.cpp
VIEW_IMAGE_LDADD = $(FAKE_LIBS)
VIEW_IMAGE_DEPENDS = SCREEN EVENT ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,ViewImage,VIEW_IMAGE))

RUN_CANVAS_SOURCES = \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Look/ButtonLook.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/RunCanvas.cpp
RUN_CANVAS_LDADD = $(FAKE_LIBS)
RUN_CANVAS_DEPENDS = FORM SCREEN EVENT ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,RunCanvas,RUN_CANVAS))

RUN_MAP_WINDOW_SOURCES = \
	$(CONTEST_SRC_DIR)/Settings.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Engine/Trace/Point.cpp \
	$(SRC)/Engine/Trace/Trace.cpp \
	$(SRC)/Engine/Trace/Vector.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalBand.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalSlice.cpp \
	$(IO_SRC_DIR)/MapFile.cpp \
	$(IO_SRC_DIR)/DataFile.cpp \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(SRC)/FLARM/Id.cpp \
	$(SRC)/FLARM/Friends.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/FLARM/Global.cpp \
	$(SRC)/Airspace/ActivePredicate.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Airspace/AirspaceVisibility.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(SRC)/Renderer/GeoBitmapRenderer.cpp \
	$(SRC)/Renderer/TransparentRendererCache.cpp \
	$(SRC)/Renderer/AirspaceRendererSettings.cpp \
	$(SRC)/Renderer/BackgroundRenderer.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/Projection/CompareProjection.cpp \
	$(SRC)/Weather/Rasp/RaspStore.cpp \
	$(SRC)/Weather/Rasp/RaspCache.cpp \
	$(SRC)/Weather/Rasp/RaspRenderer.cpp \
	$(SRC)/Weather/Rasp/RaspStyle.cpp \
	$(SRC)/Renderer/FAITriangleAreaRenderer.cpp \
	$(SRC)/Renderer/OZRenderer.cpp \
	$(SRC)/Renderer/TaskRenderer.cpp \
	$(SRC)/Renderer/TaskPointRenderer.cpp \
	$(SRC)/Renderer/AircraftRenderer.cpp \
	$(SRC)/Renderer/AirspaceRenderer.cpp \
	$(SRC)/Renderer/AirspaceRendererGL.cpp \
	$(SRC)/Renderer/AirspaceRendererOther.cpp \
	$(SRC)/Renderer/AirspaceLabelList.cpp \
	$(SRC)/Renderer/AirspaceLabelRenderer.cpp \
	$(SRC)/Renderer/BestCruiseArrowRenderer.cpp \
	$(SRC)/Renderer/CompassRenderer.cpp \
	$(SRC)/Renderer/FinalGlideBarRenderer.cpp \
	$(SRC)/Renderer/TrackLineRenderer.cpp \
	$(SRC)/Renderer/TrafficRenderer.cpp \
	$(SRC)/Renderer/TrailRenderer.cpp \
	$(SRC)/Renderer/WaypointIconRenderer.cpp \
	$(SRC)/Renderer/WaypointRenderer.cpp \
	$(SRC)/Renderer/WaypointRendererSettings.cpp \
	$(SRC)/Renderer/WaypointLabelList.cpp \
	$(SRC)/Renderer/WindArrowRenderer.cpp \
	$(SRC)/Renderer/WaveRenderer.cpp \
	$(SRC)/Math/Screen.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Renderer/LabelBlock.cpp \
	$(SRC)/Renderer/TextInBox.cpp \
	$(SRC)/UISettings.cpp \
	$(SRC)/Audio/Settings.cpp \
	$(SRC)/Audio/VarioSettings.cpp \
	$(SRC)/DisplaySettings.cpp \
	$(SRC)/PageSettings.cpp \
	$(SRC)/InfoBoxes/InfoBoxSettings.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Gauge/VarioSettings.cpp \
	$(SRC)/Gauge/TrafficSettings.cpp \
	$(SRC)/MapSettings.cpp \
	$(SRC)/Computer/Settings.cpp \
	$(SRC)/Computer/Wind/Settings.cpp \
	$(SRC)/TeamCode/Settings.cpp \
	$(SRC)/Logger/Settings.cpp \
	$(SRC)/Computer/TraceComputer.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Task/ProtectedRoutePlanner.cpp \
	$(SRC)/Task/RoutePlannerGlue.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Formatter/AirspaceUserUnitsFormatter.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/ComputerProfile.cpp \
	$(SRC)/Profile/TaskProfile.cpp \
	$(SRC)/Profile/RouteProfile.cpp \
	$(SRC)/Profile/ContestProfile.cpp \
	$(SRC)/Profile/AirspaceConfig.cpp \
	$(SRC)/Profile/TrackingProfile.cpp \
	$(SRC)/Profile/WeatherProfile.cpp \
	$(SRC)/Profile/MapProfile.cpp \
	$(SRC)/Profile/TerrainConfig.cpp \
	$(SRC)/Profile/Screen.cpp \
	$(SRC)/Profile/FlarmProfile.cpp \
	$(SRC)/Waypoint/HomeGlue.cpp \
	$(SRC)/Waypoint/LastUsed.cpp \
	$(SRC)/Waypoint/WaypointGlue.cpp \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/RadioFrequency.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunMapWindow.cpp

ifeq ($(HAVE_HTTP),y)
RUN_MAP_WINDOW_SOURCES += \
	$(SRC)/Weather/NOAAGlue.cpp \
	$(SRC)/Weather/NOAAStore.cpp
endif

RUN_MAP_WINDOW_DEPENDS = \
	LIBMAPWINDOW \
	PROFILE TERRAIN TOPO \
	FORM \
	LOOK \
	SCREEN EVENT \
	RESOURCE \
	OPERATION \
	ASYNC OS IO THREAD \
	TASK ROUTE GLIDE WAYPOINT WAYPOINTFILE AIRSPACE \
	JASPER ZZIP LIBNMEA GEO MATH TIME UTIL
$(eval $(call link-program,RunMapWindow,RUN_MAP_WINDOW))

RUN_LIST_CONTROL_SOURCES = \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/UIUtil/KineticManager.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/RunListControl.cpp
RUN_LIST_CONTROL_DEPENDS = FORM SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,RunListControl,RUN_LIST_CONTROL))

RUN_TEXT_ENTRY_SOURCES = \
	$(SRC)/Dialogs/TextEntry.cpp \
	$(SRC)/Dialogs/KnobTextEntry.cpp \
	$(SRC)/Dialogs/TouchTextEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/RunTextEntry.cpp
RUN_TEXT_ENTRY_DEPENDS = GEO FORM WIDGET DATA_FIELD SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunTextEntry,RUN_TEXT_ENTRY))

RUN_NUMBER_ENTRY_SOURCES = \
	$(SRC)/Dialogs/NumberEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunNumberEntry.cpp
RUN_NUMBER_ENTRY_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT RESOURCE ASYNC IO OS THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunNumberEntry,RUN_NUMBER_ENTRY))

RUN_DATE_ENTRY_SOURCES = \
	$(SRC)/Dialogs/DateEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunDateEntry.cpp
RUN_DATE_ENTRY_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunDateEntry,RUN_DATE_ENTRY))

RUN_TIME_ENTRY_SOURCES = \
	$(SRC)/Dialogs/TimeEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunTimeEntry.cpp
RUN_TIME_ENTRY_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunTimeEntry,RUN_TIME_ENTRY))

RUN_ANGLE_ENTRY_SOURCES = \
	$(SRC)/Dialogs/NumberEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunAngleEntry.cpp
RUN_ANGLE_ENTRY_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunAngleEntry,RUN_ANGLE_ENTRY))

RUN_GEOPOINT_ENTRY_SOURCES = \
	$(SRC)/Dialogs/GeoPointEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunGeoPointEntry.cpp
RUN_GEOPOINT_ENTRY_DEPENDS = GEO FORM WIDGET DATA_FIELD SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL ZLIB GEOPOINT
$(eval $(call link-program,RunGeoPointEntry,RUN_GEOPOINT_ENTRY))

RUN_TERMINAL_SOURCES = \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/ui/control/TerminalWindow.cpp \
	$(SRC)/Look/TerminalLook.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunTerminal.cpp
RUN_TERMINAL_DEPENDS = SCREEN EVENT ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,RunTerminal,RUN_TERMINAL))

RUN_RENDER_OZ_SOURCES = \
	$(SRC)/Renderer/OZRenderer.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Renderer/AirspaceRendererSettings.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunRenderOZ.cpp
RUN_RENDER_OZ_DEPENDS = TASK FORM SCREEN EVENT RESOURCE ASYNC OS IO THREAD GEO MATH UTIL
$(eval $(call link-program,RunRenderOZ,RUN_RENDER_OZ))

RUN_CHART_RENDERER_SOURCES = \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Look/ChartLook.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Renderer/ChartRenderer.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunChartRenderer.cpp
RUN_CHART_RENDERER_DEPENDS = FORM SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,RunChartRenderer,RUN_CHART_RENDERER))

RUN_WIND_ARROW_RENDERER_SOURCES = \
	$(SRC)/Math/Screen.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Renderer/LabelBlock.cpp \
	$(SRC)/Renderer/TextInBox.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/WindArrowLook.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Renderer/WindArrowRenderer.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunWindArrowRenderer.cpp
RUN_WIND_ARROW_RENDERER_DEPENDS = FORM SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL UNITS
$(eval $(call link-program,RunWindArrowRenderer,RUN_WIND_ARROW_RENDERER))

RUN_HORIZON_RENDERER_SOURCES = \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Math/Screen.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/HorizonLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Renderer/HorizonRenderer.cpp \
	$(SRC)/Renderer/RadarRenderer.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/RunHorizonRenderer.cpp
RUN_HORIZON_RENDERER_DEPENDS = FORM SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,RunHorizonRenderer,RUN_HORIZON_RENDERER))

RUN_FINAL_GLIDE_BAR_RENDERER_SOURCES = \
	$(SRC)/Math/Screen.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Renderer/LabelBlock.cpp \
	$(SRC)/Renderer/TextInBox.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/FinalGlideBarLook.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Renderer/FinalGlideBarRenderer.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalBand.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalSlice.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunFinalGlideBarRenderer.cpp
RUN_FINAL_GLIDE_BAR_RENDERER_DEPENDS = FORM SCREEN EVENT RESOURCE ASYNC OS IO THREAD TASK GLIDE LIBNMEA GEO MATH UTIL UNITS
$(eval $(call link-program,RunFinalGlideBarRenderer,RUN_FINAL_GLIDE_BAR_RENDERER))

RUN_FAI_TRIANGLE_SECTOR_RENDERER_SOURCES = \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Math/Screen.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Renderer/FAITriangleAreaRenderer.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(ENGINE_SRC_DIR)/Task/Shapes/FAITriangleSettings.cpp \
	$(ENGINE_SRC_DIR)/Task/Shapes/FAITriangleArea.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunFAITriangleSectorRenderer.cpp
RUN_FAI_TRIANGLE_SECTOR_RENDERER_DEPENDS = FORM SCREEN EVENT RESOURCE ASYNC OS IO THREAD GEO MATH UTIL
$(eval $(call link-program,RunFAITriangleSectorRenderer,RUN_FAI_TRIANGLE_SECTOR_RENDERER))

RUN_FLIGHT_LIST_RENDERER_SOURCES = \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Renderer/FlightListRenderer.cpp \
	$(SRC)/FlightInfo.cpp \
	$(SRC)/Logger/FlightParser.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunFlightListRenderer.cpp
RUN_FLIGHT_LIST_RENDERER_DEPENDS = FORM SCREEN EVENT ASYNC IO OS THREAD MATH UTIL TIME
$(eval $(call link-program,RunFlightListRenderer,RUN_FLIGHT_LIST_RENDERER))

RUN_PROGRESS_WINDOW_SOURCES = \
	$(SRC)/Version.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/ProgressWindow.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunProgressWindow.cpp
RUN_PROGRESS_WINDOW_DEPENDS = SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,RunProgressWindow,RUN_PROGRESS_WINDOW))

RUN_JOB_DIALOG_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Job/Thread.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/ProgressWindow.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	$(SRC)/Dialogs/ProgressDialog.cpp \
	$(SRC)/Dialogs/JobDialog.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunJobDialog.cpp
RUN_JOB_DIALOG_DEPENDS = OPERATION FORM SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,RunJobDialog,RUN_JOB_DIALOG))

RUN_ANALYSIS_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Engine/Trace/Point.cpp \
	$(SRC)/Engine/Trace/Trace.cpp \
	$(SRC)/Engine/Trace/Vector.cpp \
	$(ENGINE_SRC_DIR)/ThermalBand/ThermalBand.cpp \
	$(SRC)/UIUtil/GestureManager.cpp \
	$(SRC)/Task/DefaultTask.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Task/ProtectedRoutePlanner.cpp \
	$(SRC)/Task/RoutePlannerGlue.cpp \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Atmosphere/CuSonde.cpp \
	$(SRC)/Computer/Wind/CirclingWind.cpp \
	$(SRC)/Computer/Wind/Store.cpp \
	$(SRC)/Computer/Wind/MeasurementList.cpp \
	$(SRC)/Computer/Wind/WindEKF.cpp \
	$(SRC)/Computer/Wind/WindEKFGlue.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/Projection/MapWindowProjection.cpp \
	$(SRC)/Projection/ChartProjection.cpp \
	$(SRC)/Projection/CompareProjection.cpp \
	$(SRC)/Renderer/BackgroundRenderer.cpp \
	$(SRC)/Renderer/GeoBitmapRenderer.cpp \
	$(SRC)/Renderer/AirspaceRenderer.cpp \
	$(SRC)/Renderer/AirspaceRendererGL.cpp \
	$(SRC)/Renderer/AirspaceRendererOther.cpp \
	$(SRC)/Renderer/TransparentRendererCache.cpp \
	$(SRC)/Renderer/GradientRenderer.cpp \
	$(SRC)/Renderer/ChartRenderer.cpp \
	$(SRC)/Renderer/TaskRenderer.cpp \
	$(SRC)/Renderer/TaskPointRenderer.cpp \
	$(SRC)/Renderer/OZRenderer.cpp \
	$(SRC)/Renderer/AircraftRenderer.cpp \
	$(SRC)/Renderer/TrailRenderer.cpp \
	$(SRC)/MapWindow/MapCanvas.cpp \
	$(SRC)/MapWindow/StencilMapCanvas.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Temperature.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/LocalPath.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Dialogs/dlgAnalysis.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/CrossSection/AirspaceXSRenderer.cpp \
	$(SRC)/CrossSection/TerrainXSRenderer.cpp \
	$(SRC)/CrossSection/CrossSectionRenderer.cpp \
	$(SRC)/CrossSection/CrossSectionWindow.cpp \
	$(SRC)/FlightStatistics.cpp \
	$(SRC)/Renderer/AirspacePreviewRenderer.cpp \
	$(SRC)/Renderer/FlightStatisticsRenderer.cpp \
	$(SRC)/Renderer/BarographRenderer.cpp \
	$(SRC)/Renderer/ClimbChartRenderer.cpp \
	$(SRC)/Renderer/GlidePolarRenderer.cpp \
	$(SRC)/Renderer/GlidePolarInfoRenderer.cpp \
	$(SRC)/Renderer/MacCreadyRenderer.cpp \
	$(SRC)/Renderer/VarioHistogramRenderer.cpp \
	$(SRC)/Renderer/TaskLegRenderer.cpp \
	$(SRC)/Renderer/TaskSpeedRenderer.cpp \
	$(SRC)/Renderer/ThermalBandRenderer.cpp \
	$(SRC)/Renderer/WindChartRenderer.cpp \
	$(SRC)/Renderer/CuRenderer.cpp \
	$(SRC)/Renderer/MapScaleRenderer.cpp \
	$(SRC)/Audio/Settings.cpp \
	$(SRC)/Audio/VarioSettings.cpp \
	$(SRC)/UISettings.cpp \
	$(SRC)/DisplaySettings.cpp \
	$(SRC)/PageSettings.cpp \
	$(SRC)/InfoBoxes/InfoBoxSettings.cpp \
	$(SRC)/Gauge/VarioSettings.cpp \
	$(SRC)/Gauge/TrafficSettings.cpp \
	$(SRC)/TeamCode/TeamCode.cpp \
	$(SRC)/TeamCode/Settings.cpp \
	$(SRC)/Logger/Settings.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/MapSettings.cpp \
	$(SRC)/Blackboard/InterfaceBlackboard.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(SRC)/Airspace/ActivePredicate.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Airspace/AirspaceGlue.cpp \
	$(SRC)/Airspace/AirspaceVisibility.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(SRC)/Renderer/AirspaceRendererSettings.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	$(IO_SRC_DIR)/MapFile.cpp \
	$(SRC)/io/ConfiguredFile.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunAnalysis.cpp
RUN_ANALYSIS_DEPENDS = \
	TERRAIN \
	DRIVER \
	PROFILE \
	FORM WIDGET \
	LOOK \
	OPERATION \
	SCREEN EVENT RESOURCE LIBCOMPUTER LIBNMEA ASYNC IO DATA_FIELD \
	OS THREAD \
	CONTEST TASKFILE ROUTE GLIDE \
	WAYPOINT WAYPOINTFILE \
	ROUTE AIRSPACE ZZIP UTIL GEO MATH TIME
$(eval $(call link-program,RunAnalysis,RUN_ANALYSIS))

RUN_AIRSPACE_WARNING_DIALOG_SOURCES = \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/AirspaceFormatter.cpp \
	$(SRC)/Formatter/AirspaceUserUnitsFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Renderer/TwoTextRowsRenderer.cpp \
	$(SRC)/Dialogs/Airspace/dlgAirspaceWarnings.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Audio/Sound.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(IO_SRC_DIR)/MapFile.cpp \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunAirspaceWarningDialog.cpp
RUN_AIRSPACE_WARNING_DIALOG_LDADD = $(FAKE_LIBS)
RUN_AIRSPACE_WARNING_DIALOG_DEPENDS = FORM WIDGET DATA_FIELD SCREEN AUDIO EVENT RESOURCE ASYNC IO OS THREAD AIRSPACE ZZIP UTIL GEO MATH TIME UNITS
$(eval $(call link-program,RunAirspaceWarningDialog,RUN_AIRSPACE_WARNING_DIALOG))

RUN_PROFILE_LIST_DIALOG_SOURCES = \
	$(SRC)/Renderer/TextRowRenderer.cpp \
	$(SRC)/Dialogs/ProfileListDialog.cpp \
	$(SRC)/Dialogs/ProfilePasswordDialog.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Dialogs/TextEntry.cpp \
	$(SRC)/Dialogs/KnobTextEntry.cpp \
	$(SRC)/Dialogs/TouchTextEntry.cpp \
	$(SRC)/Dialogs/Message.cpp \
	$(SRC)/Dialogs/Error.cpp \
	$(SRC)/Dialogs/HelpDialog.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/RunProfileListDialog.cpp
RUN_PROFILE_LIST_DIALOG_LDADD = $(FAKE_LIBS)
RUN_PROFILE_LIST_DIALOG_DEPENDS = PROFILE FORM WIDGET DATA_FIELD SCREEN EVENT RESOURCE ASYNC OS IO THREAD MATH UTIL
$(eval $(call link-program,RunProfileListDialog,RUN_PROFILE_LIST_DIALOG))

PLAY_TONE_SOURCES = \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/PlayTone.cpp
PLAY_TONE_DEPENDS = AUDIO MATH SCREEN EVENT ASYNC THREAD OS IO UTIL
$(eval $(call link-program,PlayTone,PLAY_TONE))

PLAY_VARIO_SOURCES = \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(SRC)/TransponderCode.cpp \
	$(DEBUG_REPLAY_SOURCES) \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/PlayVario.cpp
PLAY_VARIO_LDADD = $(AUDIO_LDADD) $(SCREEN_LDADD) $(EVENT_LDADD)
PLAY_VARIO_DEPENDS = $(DEBUG_REPLAY_DEPENDS) AUDIO GEO MATH SCREEN EVENT ASYNC THREAD OS TIME UTIL
$(eval $(call link-program,PlayVario,PLAY_VARIO))

DUMP_VARIO_SOURCES = \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(DEBUG_REPLAY_SOURCES) \
	$(TEST_SRC_DIR)/DumpVario.cpp
DUMP_VARIO_DEPENDS = $(DEBUG_REPLAY_DEPENDS) AUDIO GEO MATH SCREEN EVENT UTIL OS TIME
$(eval $(call link-program,DumpVario,DUMP_VARIO))

RUN_TASK_EDITOR_DIALOG_SOURCES = \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Dialogs/Inflate.cpp \
	$(SRC)/Dialogs/ComboPicker.cpp \
	$(SRC)/Dialogs/HelpDialog.cpp \
	$(SRC)/Dialogs/dlgTaskOverview.cpp \
	$(SRC)/Dialogs/WaypointList.cpp \
	$(SRC)/Dialogs/dlgWaypointDetails.cpp \
	$(SRC)/Dialogs/dlgTaskWaypoint.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(MORE_SCREEN_SOURCES) \
	$(SRC)/Look/GlobalFonts.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Waypoint/WaypointGlue.cpp \
	$(SRC)/Waypoint/Factory.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunTaskEditorDialog.cpp
RUN_TASK_EDITOR_DIALOG_LDADD = $(FAKE_LIBS)
RUN_TASK_EDITOR_DIALOG_DEPENDS = WAYPOINTFILE TASKFILE OPERATION FORM WIDGET DATA_FIELD SCREEN EVENT RESOURCE IO OS THREAD ZZIP UTIL GEO
$(eval $(call link-program,RunTaskEditorDialog,RUN_TASK_EDITOR_DIALOG))

TEST_NOTIFY_SOURCES = \
	$(SRC)/Hardware/CPU.cpp \
	$(SRC)/ui/event/Idle.cpp \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestNotify.cpp
TEST_NOTIFY_DEPENDS = EVENT SCREEN MATH ASYNC OS IO THREAD UTIL
$(eval $(call link-program,TestNotify,TEST_NOTIFY))

FEED_NMEA_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/FeedNMEA.cpp
FEED_NMEA_DEPENDS = PORT ASYNC LIBNET OPERATION IO OS THREAD TIME UTIL
$(eval $(call link-program,FeedNMEA,FEED_NMEA))

FEED_VEGA_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/FeedVega.cpp
FEED_VEGA_DEPENDS = PORT ASYNC LIBNET OPERATION IO OS THREAD TIME UTIL
$(eval $(call link-program,FeedVega,FEED_VEGA))

EMULATE_DEVICE_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Util/LineSplitter.cpp \
	$(SRC)/Device/Util/NMEAWriter.cpp \
	$(SRC)/Device/Util/NMEAReader.cpp \
	$(SRC)/Device/Driver/FLARM/BinaryProtocol.cpp \
	$(SRC)/Device/Driver/FLARM/CRC16.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/io/CSVLine.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/ATR833Emulator.cpp \
	$(TEST_SRC_DIR)/FLARMEmulator.cpp \
	$(TEST_SRC_DIR)/VegaEmulator.cpp \
	$(TEST_SRC_DIR)/EmulateDevice.cpp
EMULATE_DEVICE_DEPENDS = PORT ASYNC LIBNET OPERATION IO OS THREAD LIBNMEA GEO MATH TIME UTIL UNITS
$(eval $(call link-program,EmulateDevice,EMULATE_DEVICE))

FEED_FLYNET_DATA_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/FeedFlyNetData.cpp
FEED_FLYNET_DATA_DEPENDS = PORT ASYNC LIBNET OPERATION IO OS THREAD TIME UTIL

$(eval $(call link-program,FeedFlyNetData,FEED_FLYNET_DATA))

TASK_INFO_SOURCES = \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Task/ValidationErrorStrings.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/TaskInfo.cpp
TASK_INFO_DEPENDS = TASKFILE ROUTE GLIDE WAYPOINT IO OS GEO TIME MATH UTIL
$(eval $(call link-program,TaskInfo,TASK_INFO))

DUMP_TASK_FILE_SOURCES = \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(SRC)/Engine/Route/Config.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/DumpTaskFile.cpp
DUMP_TASK_FILE_DEPENDS = TASKFILE GLIDE WAYPOINT WAYPOINTFILE OPERATION IO OS THREAD ZZIP GEO TIME MATH UTIL
$(eval $(call link-program,DumpTaskFile,DUMP_TASK_FILE))

DUMP_FLARM_NET_SOURCES = \
	$(SRC)/FLARM/FlarmNetReader.cpp \
	$(SRC)/FLARM/Id.cpp \
	$(SRC)/FLARM/FlarmNetRecord.cpp \
	$(SRC)/FLARM/FlarmNetDatabase.cpp \
	$(TEST_SRC_DIR)/DumpFlarmNet.cpp
DUMP_FLARM_NET_DEPENDS = IO OS MATH UTIL
$(eval $(call link-program,DumpFlarmNet,DUMP_FLARM_NET))

IGC2NMEA_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Formatter/NMEAFormatter.cpp \
	$(SRC)/TransponderCode.cpp \
	$(TEST_SRC_DIR)/IGC2NMEA.cpp
IGC2NMEA_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL TIME

$(eval $(call link-program,IGC2NMEA,IGC2NMEA))

debug: $(DEBUG_PROGRAMS)

TEST_REPLAY_RETROSPECTIVE_SOURCES = \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/test_replay_retrospective.cpp
TEST_REPLAY_RETROSPECTIVE_DEPENDS = $(TEST1_DEPENDS) OPERATION WAYPOINTFILE
$(eval $(call link-program,test_replay_retrospective,TEST_REPLAY_RETROSPECTIVE))

TEST_PACKED_FLOAT_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestPackedFloat.cpp
TEST_PACKED_FLOAT_DEPENDS = MATH
$(eval $(call link-program,TestPackedFloat,TEST_PACKED_FLOAT))

TEST_VERSION_NUMBER_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestVersionNumber.cpp
TEST_VERSION_NUMBER_DEPENDS = MATH UTILS
$(eval $(call link-program,TestVersionNumber,TEST_VERSION_NUMBER))
