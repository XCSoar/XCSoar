name-to-bin = $(patsubst %,$(TARGET_BIN_DIR)/%$(TARGET_EXEEXT),$(1))

TESTFAST = \
	test_normalise \
	test_fixed \
	TestWaypoints \
	test_pressure \
	test_mc \
	test_task \
	test_modes \
	test_automc \
	test_acfilter \
	test_trees \
	test_vopt

TESTSLOW = \
	test_bestcruisetrack \
	test_airspace \
	test_effectivemc \
	test_cruiseefficiency \
	test_highterrain \
	test_randomtask \
	test_flight \
	test_aat \
	test_replay_olc

HARNESS_PROGRAMS = $(TESTFAST) $(TESTSLOW)

build-harness: $(call name-to-bin,$(HARNESS_PROGRAMS))

testslow: $(call name-to-bin,$(TESTSLOW))
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(addprefix $(TARGET_BIN_DIR)/,$(TESTSLOW))

testfast: $(call name-to-bin,$(TESTFAST))
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(addprefix $(TARGET_BIN_DIR)/,$(TESTFAST))

TEST1_LDADD = $(HARNESS_LIBS) \
	$(TASK_LIBS) \
	$(ROUTE_LIBS) \
	$(GLIDE_LIBS) \
	$(CONTEST_LIBS) \
	$(WAYPOINT_LIBS) \
	$(AIRSPACE_LIBS) \
	$(IO_LIBS) \
	$(OS_LIBS) \
	$(THREAD_LIBS) \
	$(ZZIP_LDADD) \
	$(GEO_LIBS) \
	$(TIME_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
TEST1_LDLIBS = \
	$(ZZIP_LDLIBS)

define link-harness-program
$(1)_SOURCES = \
	$(SRC)/Engine/Trace/Point.cpp \
	$(SRC)/Engine/Trace/Trace.cpp \
	$(SRC)/Engine/Trace/Vector.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Formatter/AirspaceFormatter.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/$(1).cpp
$(1)_LDADD = $(TEST1_LDADD)
$(1)_LDLIBS = $(TEST1_LDLIBS)
$(call link-program,$(1),$(1))
endef

$(foreach name,$(HARNESS_PROGRAMS),$(eval $(call link-harness-program,$(name))))

TEST_NAMES = \
	test_fixed \
	test_normalise \
	TestWaypoints \
	test_pressure \
	test_task \
	TestOverwritingRingBuffer \
	TestDateTime TestRoughTime TestWrapClock \
	TestMathTables \
	TestAngle TestUnits TestEarth TestSunEphemeris \
	TestValidity TestUTM TestProfile \
	TestRadixTree TestGeoBounds TestGeoClip \
	TestLogger TestGRecord TestDriver TestClimbAvCalc \
	TestWaypointReader TestThermalBase \
	TestFlarmNet \
	TestColorRamp TestGeoPoint TestDiffFilter \
	TestFileUtil TestPolars TestCSVLine TestGlidePolar \
	test_replay_task TestProjection TestFlatPoint TestFlatLine TestFlatGeoPoint \
	TestMacCready TestOrderedTask TestAATPoint \
	TestPlanes \
	TestTaskPoint \
	TestTaskWaypoint \
	TestTeamCode \
	TestZeroFinder \
	TestAirspaceParser \
	TestMETARParser \
	TestIGCParser \
	TestByteOrder \
	TestByteOrder2 \
	TestStrings TestUTF8 \
	TestCRC \
	TestUnitsFormatter \
	TestGeoPointFormatter \
	TestHexColorFormatter \
	TestByteSizeFormatter \
	TestTimeFormatter \
	TestIGCFilenameFormatter \
	TestLXNToIGC

TESTS = $(call name-to-bin,$(TEST_NAMES))

TEST_CRC_SOURCES = \
	$(SRC)/Util/CRC.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestCRC.cpp
$(eval $(call link-program,TestCRC,TEST_CRC))

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

TEST_BYTE_ORDER_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestByteOrder.cpp
$(eval $(call link-program,TestByteOrder,TEST_BYTE_ORDER))

TEST_BYTE_ORDER2_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestByteOrder2.cpp
$(eval $(call link-program,TestByteOrder2,TEST_BYTE_ORDER2))

TEST_METAR_PARSER_SOURCES = \
	$(SRC)/Weather/METARParser.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestMETARParser.cpp
TEST_METAR_PARSER_DEPENDS = MATH UTIL
$(eval $(call link-program,TestMETARParser,TEST_METAR_PARSER))

TEST_AIRSPACE_PARSER_SOURCES = \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestAirspaceParser.cpp
TEST_AIRSPACE_PARSER_LDADD = $(FAKE_LIBS)
TEST_AIRSPACE_PARSER_DEPENDS = IO OS AIRSPACE ZZIP GEO MATH UTIL
$(eval $(call link-program,TestAirspaceParser,TEST_AIRSPACE_PARSER))

TEST_DATE_TIME_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestDateTime.cpp
TEST_DATE_TIME_DEPENDS = MATH TIME
$(eval $(call link-program,TestDateTime,TEST_DATE_TIME))

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
	$(SRC)/Profile/ProfileMap.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/TestProfile.cpp
TEST_PROFILE_DEPENDS = MATH IO OS UTIL
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

TEST_PLANES_SOURCES = \
	$(SRC)/Polar/Parser.cpp \
	$(SRC)/Plane/PlaneFileGlue.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestPlanes.cpp
TEST_PLANES_DEPENDS = IO OS MATH UTIL
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
	$(SRC)/XML/Node.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/test_troute.cpp
TEST_TROUTE_DEPENDS = TERRAIN IO ZZIP OS ROUTE GLIDE GEO MATH UTIL
$(eval $(call link-program,test_troute,TEST_TROUTE))

TEST_REACH_SOURCES = \
	$(SRC)/XML/Node.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/test_reach.cpp
TEST_REACH_DEPENDS = TERRAIN IO ZZIP OS ROUTE GLIDE GEO MATH UTIL
$(eval $(call link-program,test_reach,TEST_REACH))

TEST_ROUTE_SOURCES = \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/Formatter/AirspaceFormatter.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/AirspacePrinting.cpp \
	$(TEST_SRC_DIR)/harness_airspace.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/test_route.cpp
TEST_ROUTE_DEPENDS = TERRAIN IO ZZIP OS ROUTE AIRSPACE GLIDE GEO MATH UTIL
$(eval $(call link-program,test_route,TEST_ROUTE))

TEST_REPLAY_TASK_SOURCES = \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/Computer/FlyingComputer.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(SRC)/XML/DataNode.cpp \
	$(SRC)/XML/DataNodeXML.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Replay/IgcReplay.cpp \
	$(SRC)/Replay/TaskAutoPilot.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/TaskPrinting.cpp \
	$(TEST_SRC_DIR)/TaskEventsPrint.cpp \
	$(TEST_SRC_DIR)/harness_task.cpp \
	$(TEST_SRC_DIR)/test_debug.cpp \
	$(TEST_SRC_DIR)/test_replay_task.cpp
TEST_REPLAY_TASK_DEPENDS = TASK ROUTE WAYPOINT GLIDE GEO MATH IO OS UTIL TIME
$(eval $(call link-program,test_replay_task,TEST_REPLAY_TASK))

TEST_MATH_TABLES_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestMathTables.cpp
TEST_MATH_TABLES_DEPENDS = MATH
$(eval $(call link-program,TestMathTables,TEST_MATH_TABLES))

TEST_ANGLE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestAngle.cpp
TEST_ANGLE_DEPENDS = MATH
$(eval $(call link-program,TestAngle,TEST_ANGLE))

TEST_CSV_LINE_SOURCES = \
	$(SRC)/IO/CSVLine.cpp \
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
	$(SRC)/FLARM/FlarmId.cpp \
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
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUnits.cpp
TEST_UNITS_DEPENDS = MATH
$(eval $(call link-program,TestUnits,TEST_UNITS))

TEST_UNITS_FORMATTER_SOURCES = \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUnitsFormatter.cpp
TEST_UNITS_FORMATTER_DEPENDS = MATH UTIL
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
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Polar/Shape.cpp \
	$(SRC)/Polar/Polar.cpp \
	$(SRC)/Polar/Parser.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/PolarCoefficients.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlidePolar.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideResult.cpp \
	$(SRC)/Polar/PolarFileGlue.cpp \
	$(SRC)/Polar/PolarStore.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestPolars.cpp
TEST_POLARS_DEPENDS = IO OS MATH UTIL
$(eval $(call link-program,TestPolars,TEST_POLARS))

TEST_GLIDE_POLAR_SOURCES = \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlidePolar.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/PolarCoefficients.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideResult.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideState.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/MacCready.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGlidePolar.cpp
TEST_GLIDE_POLAR_DEPENDS = GEO MATH IO
$(eval $(call link-program,TestGlidePolar,TEST_GLIDE_POLAR))

TEST_FILE_UTIL_SOURCES = \
	$(SRC)/OS/FileUtil.cpp \
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
	$(SRC)/Screen/Ramp.cpp \
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

TEST_RADIX_TREE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestRadixTree.cpp
TEST_RADIX_TREE_DEPENDS = UTIL
$(eval $(call link-program,TestRadixTree,TEST_RADIX_TREE))

TEST_LOGGER_SOURCES = \
	$(SRC)/IGC/IGCFix.cpp \
	$(SRC)/IGC/IGCWriter.cpp \
	$(SRC)/IGC/IGCString.cpp \
	$(SRC)/Logger/LoggerFRecord.cpp \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/LoggerEPE.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestLogger.cpp
TEST_LOGGER_DEPENDS = IO OS GEO MATH UTIL
$(eval $(call link-program,TestLogger,TEST_LOGGER))

TEST_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/Version.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGRecord.cpp
TEST_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,TestGRecord,TEST_GRECORD))

TEST_DRIVER_SOURCES = \
	$(SRC)/Device/Port/NullPort.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoint.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/TestDriver.cpp
TEST_DRIVER_DEPENDS = DRIVER GEO MATH IO OS THREAD UTIL TIME
$(eval $(call link-program,TestDriver,TEST_DRIVER))

TEST_WAY_POINT_FILE_SOURCES = \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Waypoint/WaypointFileType.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderCompeGPS.cpp \
	$(SRC)/Waypoint/WaypointWriter.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestWaypointReader.cpp
TEST_WAY_POINT_FILE_DEPENDS = WAYPOINT GEO MATH IO UTIL ZZIP OS THREAD
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

DEBUG_PROGRAM_NAMES = \
	test_reach \
	test_route \
	test_troute \
	TestTrace \
	FlightTable \
	RunTrace \
	RunOLCAnalysis \
	FlightPath \
	BenchmarkProjection \
	BenchmarkFAITriangleSector \
	DumpTextFile DumpTextZip WriteTextFile RunTextWriter \
	DumpHexColor \
	RunXMLParser \
	ReadMO \
	ReadProfileString ReadProfileInt \
	RunMD5 \
	ReadGRecord VerifyGRecord AppendGRecord FixGRecord \
	AddChecksum \
	KeyCodeDumper \
	LoadTopography LoadTerrain \
	RunHeightMatrix \
	RunInputParser \
	RunWaypointParser RunAirspaceParser \
	EnumeratePorts \
	ReadPort RunPortHandler LogPort \
	RunDeviceDriver RunDeclare RunFlightList RunDownloadFlight \
	RunEnableNMEA \
	CAI302Tool \
	lxn2igc \
	RunIGCWriter \
	RunFlightLogger RunFlyingComputer \
	RunCirclingWind RunWindEKF RunWindComputer \
	RunTask \
	LoadImage ViewImage \
	RunCanvas RunMapWindow \
	RunDialog RunListControl \
	RunTextEntry RunNumberEntry RunTimeEntry RunAngleEntry \
	RunGeoPointEntry \
	RunTerminal \
	RunRenderOZ \
	RunChartRenderer \
	RunWindArrowRenderer \
	RunHorizonRenderer \
	RunFinalGlideBarRenderer \
	RunFAITriangleSectorRenderer \
	RunProgressWindow \
	RunJobDialog \
	RunAnalysis \
	RunAirspaceWarningDialog \
	TestNotify \
	FeedNMEA \
	FeedVega EmulateDevice \
	DebugDisplay \
	RunVegaSettings \
	RunFlarmUtils \
	RunLX1600Utils \
	TaskInfo DumpTaskFile \
	DumpFlarmNet \
	RunRepositoryParser \
	IGC2NMEA \
	NearestWaypoints \
	RunKalmanFilter1d \
	ArcApprox

ifeq ($(TARGET),UNIX)
DEBUG_PROGRAM_NAMES += \
	AnalyseFlight \
	FeedTCP \
	FeedFlyNetData
endif

ifeq ($(TARGET),PC)
DEBUG_PROGRAM_NAMES += FeedTCP \
  FeedFlyNetData
endif

ifeq ($(HAVE_HTTP),y)
DEBUG_PROGRAM_NAMES += DownloadFile RunDownloadToFile RunNOAADownloader RunSkyLinesTracking RunLiveTrack24
endif

ifeq ($(HAVE_PCM_PLAYER),y)
DEBUG_PROGRAM_NAMES += PlayTone PlayVario DumpVario
endif

ifeq ($(HAVE_CE)$(findstring $(TARGET),ALTAIR),y)
DEBUG_PROGRAM_NAMES += TodayInstall
endif

DEBUG_PROGRAMS = $(call name-to-bin,$(DEBUG_PROGRAM_NAMES))

DEBUG_REPLAY_SOURCES = \
	$(SRC)/Device/Port/Port.cpp \
	$(SRC)/Device/Port/NullPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceWarningConfig.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/MoreData.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/Derived.cpp \
	$(SRC)/NMEA/VarioInfo.cpp \
	$(SRC)/NMEA/ClimbInfo.cpp \
	$(SRC)/NMEA/ClimbHistory.cpp \
	$(SRC)/NMEA/CirclingInfo.cpp \
	$(SRC)/NMEA/ThermalBand.cpp \
	$(SRC)/NMEA/ThermalLocator.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/Computer/BasicComputer.cpp \
	$(SRC)/Computer/GroundSpeedComputer.cpp \
	$(SRC)/Computer/FlyingComputer.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/GlideSolvers/GlidePolar.cpp \
	$(SRC)/Engine/GlideSolvers/PolarCoefficients.cpp \
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
DEBUG_REPLAY_LDADD = \
	$(DRIVER_LDADD) \
	$(IO_LIBS) \
	$(THREAD_LIBS) \
	$(OS_LIBS)

BENCHMARK_PROJECTION_SOURCES = \
	$(SRC)/Projection/Projection.cpp \
	$(TEST_SRC_DIR)/BenchmarkProjection.cpp
BENCHMARK_PROJECTION_DEPENDS = MATH
BENCHMARK_PROJECTION_CPPFLAGS = $(SCREEN_CPPFLAGS)
$(eval $(call link-program,BenchmarkProjection,BENCHMARK_PROJECTION))

BENCHMARK_FAI_TRIANGLE_SECTOR_SOURCES = \
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

DUMP_TEXT_ZIP_SOURCES = \
	$(TEST_SRC_DIR)/DumpTextZip.cpp
DUMP_TEXT_ZIP_DEPENDS = IO ZZIP UTIL
$(eval $(call link-program,DumpTextZip,DUMP_TEXT_ZIP))

DUMP_HEX_COLOR_SOURCES = \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/DumpHexColor.cpp
DUMP_HEX_COLOR_DEPENDS = SCREEN EVENT UTIL
$(eval $(call link-program,DumpHexColor,DUMP_HEX_COLOR))

DEBUG_DISPLAY_SOURCES = \
	$(SRC)/Hardware/Display.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/DebugDisplay.cpp
DEBUG_DISPLAY_DEPENDS = IO
$(eval $(call link-program,DebugDisplay,DEBUG_DISPLAY))

WRITE_TEXT_FILE_SOURCES = \
	$(TEST_SRC_DIR)/WriteTextFile.cpp
WRITE_TEXT_FILE_DEPENDS = IO ZZIP
$(eval $(call link-program,WriteTextFile,WRITE_TEXT_FILE))

RUN_TEXT_WRITER_SOURCES = \
	$(TEST_SRC_DIR)/RunTextWriter.cpp
RUN_TEXT_WRITER_DEPENDS = IO ZZIP
$(eval $(call link-program,RunTextWriter,RUN_TEXT_WRITER))

DOWNLOAD_FILE_SOURCES = \
	$(SRC)/Version.cpp \
	$(TEST_SRC_DIR)/DownloadFile.cpp
DOWNLOAD_FILE_DEPENDS = IO LIBNET UTIL
$(eval $(call link-program,DownloadFile,DOWNLOAD_FILE))

RUN_DOWNLOAD_TO_FILE_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/RunDownloadToFile.cpp
RUN_DOWNLOAD_TO_FILE_DEPENDS = LIBNET UTIL
$(eval $(call link-program,RunDownloadToFile,RUN_DOWNLOAD_TO_FILE))

RUN_NOAA_DOWNLOADER_SOURCES = \
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
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(TEST_SRC_DIR)/ConsoleJobRunner.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/RunNOAADownloader.cpp
RUN_NOAA_DOWNLOADER_DEPENDS = GEO IO MATH LIBNET UTIL TIME
$(eval $(call link-program,RunNOAADownloader,RUN_NOAA_DOWNLOADER))

RUN_SL_TRACKING_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/Tracking/SkyLines/Client.cpp \
	$(TEST_SRC_DIR)/RunSkyLinesTracking.cpp
RUN_SL_TRACKING_LDADD = $(ASYNC_LDADD) $(DEBUG_REPLAY_LDADD)
RUN_SL_TRACKING_DEPENDS = LIBNET OS GEO MATH UTIL TIME
$(eval $(call link-program,RunSkyLinesTracking,RUN_SL_TRACKING))

RUN_LIVETRACK24_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Tracking/LiveTrack24.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(TEST_SRC_DIR)/RunLiveTrack24.cpp
RUN_LIVETRACK24_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_LIVETRACK24_DEPENDS = LIBNET GEO MATH UTIL TIME
$(eval $(call link-program,RunLiveTrack24,RUN_LIVETRACK24))

RUN_REPOSITORY_PARSER_SOURCES = \
	$(SRC)/Repository/FileRepository.cpp \
	$(SRC)/Repository/Parser.cpp \
	$(TEST_SRC_DIR)/RunRepositoryParser.cpp
RUN_REPOSITORY_PARSER_DEPENDS = LIBNET IO OS UTIL
$(eval $(call link-program,RunRepositoryParser,RUN_REPOSITORY_PARSER))

RUN_XML_PARSER_SOURCES = \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(TEST_SRC_DIR)/RunXMLParser.cpp
RUN_XML_PARSER_DEPENDS = IO OS UTIL
$(eval $(call link-program,RunXMLParser,RUN_XML_PARSER))

READ_MO_SOURCES = \
	$(SRC)/Language/MOFile.cpp \
	$(SRC)/OS/FileMapping.cpp \
	$(TEST_SRC_DIR)/ReadMO.cpp
READ_MO_DEPENDS = UTIL
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
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/RunMD5.cpp
$(eval $(call link-program,RunMD5,RUN_MD5))

READ_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/ReadGRecord.cpp
READ_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,ReadGRecord,READ_GRECORD))

VERIFY_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/VerifyGRecord.cpp
VERIFY_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,VerifyGRecord,VERIFY_GRECORD))

APPEND_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/AppendGRecord.cpp
APPEND_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,AppendGRecord,APPEND_GRECORD))

FIX_GRECORD_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/FixGRecord.cpp
FIX_GRECORD_DEPENDS = IO OS UTIL
$(eval $(call link-program,FixGRecord,FIX_GRECORD))

ADD_CHECKSUM_SOURCES = \
	$(TEST_SRC_DIR)/AddChecksum.cpp
ADD_CHECKSUM_DEPENDS = IO
$(eval $(call link-program,AddChecksum,ADD_CHECKSUM))

KEY_CODE_DUMPER_SOURCES = \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/ResourceLoader.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/KeyCodeDumper.cpp
KEY_CODE_DUMPER_LDADD = $(FAKE_LIBS)
KEY_CODE_DUMPER_DEPENDS = SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,KeyCodeDumper,KEY_CODE_DUMPER))

LOAD_TOPOGRAPHY_SOURCES = \
	$(SRC)/Topography/TopographyStore.cpp \
	$(SRC)/Topography/TopographyFile.cpp \
	$(SRC)/Topography/XShape.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(TEST_SRC_DIR)/LoadTopography.cpp
ifeq ($(OPENGL),y)
LOAD_TOPOGRAPHY_SOURCES += \
	$(SCREEN_SRC_DIR)/OpenGL/Triangulate.cpp
endif
LOAD_TOPOGRAPHY_DEPENDS = GEO MATH IO UTIL SHAPELIB ZZIP
LOAD_TOPOGRAPHY_CPPFLAGS = $(SCREEN_CPPFLAGS)
$(eval $(call link-program,LoadTopography,LOAD_TOPOGRAPHY))

LOAD_TERRAIN_SOURCES = \
	$(SRC)/Operation/Operation.cpp \
	$(TEST_SRC_DIR)/LoadTerrain.cpp
LOAD_TERRAIN_CPPFLAGS = $(SCREEN_CPPFLAGS)
LOAD_TERRAIN_DEPENDS = TERRAIN GEO MATH IO OS ZZIP UTIL
$(eval $(call link-program,LoadTerrain,LOAD_TERRAIN))

RUN_HEIGHT_MATRIX_SOURCES = \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(TEST_SRC_DIR)/RunHeightMatrix.cpp
RUN_HEIGHT_MATRIX_CPPFLAGS = $(SCREEN_CPPFLAGS)
RUN_HEIGHT_MATRIX_DEPENDS = TERRAIN GEO MATH IO OS ZZIP UTIL
$(eval $(call link-program,RunHeightMatrix,RUN_HEIGHT_MATRIX))

RUN_INPUT_PARSER_SOURCES = \
	$(SRC)/Input/InputKeys.cpp \
	$(SRC)/Input/InputConfig.cpp \
	$(SRC)/Input/InputParser.cpp \
	$(SRC)/Menu/MenuData.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunInputParser.cpp
RUN_INPUT_PARSER_DEPENDS = IO OS UTIL
$(eval $(call link-program,RunInputParser,RUN_INPUT_PARSER))

RUN_WAY_POINT_PARSER_SOURCES = \
	$(SRC)/Waypoint/WaypointFileType.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointReaderCompeGPS.cpp \
	$(SRC)/Waypoint/WaypointWriter.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunWaypointParser.cpp
RUN_WAY_POINT_PARSER_LDADD = $(FAKE_LIBS)
RUN_WAY_POINT_PARSER_DEPENDS = WAYPOINT IO OS THREAD ZZIP GEO MATH UTIL
$(eval $(call link-program,RunWaypointParser,RUN_WAY_POINT_PARSER))

NEAREST_WAYPOINTS_SOURCES = \
	$(SRC)/Waypoint/WaypointFileType.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointReaderCompeGPS.cpp \
	$(SRC)/Waypoint/WaypointWriter.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/NearestWaypoints.cpp
NEAREST_WAYPOINTS_LDADD = $(FAKE_LIBS)
NEAREST_WAYPOINTS_DEPENDS = WAYPOINT IO OS THREAD ZZIP GEO MATH UTIL
$(eval $(call link-program,NearestWaypoints,NEAREST_WAYPOINTS))

RUN_AIRSPACE_PARSER_SOURCES = \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/RunAirspaceParser.cpp
RUN_AIRSPACE_PARSER_LDADD = $(FAKE_LIBS)
RUN_AIRSPACE_PARSER_DEPENDS = IO OS AIRSPACE ZZIP GEO MATH UTIL
$(eval $(call link-program,RunAirspaceParser,RUN_AIRSPACE_PARSER))

ENUMERATE_PORTS_SOURCES = \
	$(TEST_SRC_DIR)/EnumeratePorts.cpp
ENUMERATE_PORTS_DEPENDS = PORT
$(eval $(call link-program,EnumeratePorts,ENUMERATE_PORTS))

READ_PORT_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/ReadPort.cpp
READ_PORT_DEPENDS = PORT ASYNC OS THREAD UTIL
$(eval $(call link-program,ReadPort,READ_PORT))

RUN_PORT_HANDLER_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/OS/LogError.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunPortHandler.cpp
RUN_PORT_HANDLER_DEPENDS = PORT ASYNC OS THREAD UTIL
$(eval $(call link-program,RunPortHandler,RUN_PORT_HANDLER))

LOG_PORT_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/OS/LogError.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/LogPort.cpp
LOG_PORT_DEPENDS = PORT ASYNC OS THREAD UTIL
$(eval $(call link-program,LogPort,LOG_PORT))

RUN_DEVICE_DRIVER_SOURCES = \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Device/Port/Port.cpp \
	$(SRC)/Device/Port/NullPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/RunDeviceDriver.cpp
RUN_DEVICE_DRIVER_DEPENDS = DRIVER IO OS THREAD GEO MATH UTIL TIME
$(eval $(call link-program,RunDeviceDriver,RUN_DEVICE_DRIVER))

RUN_DECLARE_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunDeclare.cpp
RUN_DECLARE_DEPENDS = DRIVER PORT ASYNC IO OS THREAD WAYPOINT GEO TIME MATH UTIL
$(eval $(call link-program,RunDeclare,RUN_DECLARE))

RUN_ENABLE_NMEA_SOURCES = \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunEnableNMEA.cpp
RUN_ENABLE_NMEA_DEPENDS = DRIVER PORT TIME GEO MATH UTIL ASYNC IO OS THREAD
$(eval $(call link-program,RunEnableNMEA,RUN_ENABLE_NMEA))

RUN_VEGA_SETTINGS_SOURCES = \
	$(VEGA_SOURCES) \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunVegaSettings.cpp
RUN_VEGA_SETTINGS_DEPENDS = PORT ASYNC IO OS THREAD MATH UTIL
$(eval $(call link-program,RunVegaSettings,RUN_VEGA_SETTINGS))

RUN_FLARM_UTILS_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunFlarmUtils.cpp
RUN_FLARM_UTILS_DEPENDS = DRIVER PORT ASYNC IO OS THREAD GEO MATH UTIL
$(eval $(call link-program,RunFlarmUtils,RUN_FLARM_UTILS))

RUN_LX1600_UTILS_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunLX1600Utils.cpp
RUN_LX1600_UTILS_DEPENDS = DRIVER PORT ASYNC IO OS THREAD GEO MATH UTIL
$(eval $(call link-program,RunLX1600Utils,RUN_LX1600_UTILS))

RUN_FLIGHT_LIST_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunFlightList.cpp
RUN_FLIGHT_LIST_DEPENDS = DRIVER PORT ASYNC IO OS THREAD GEO TIME MATH UTIL
$(eval $(call link-program,RunFlightList,RUN_FLIGHT_LIST))

RUN_DOWNLOAD_FLIGHT_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/RunDownloadFlight.cpp
RUN_DOWNLOAD_FLIGHT_DEPENDS = DRIVER PORT ASYNC IO OS THREAD GEO TIME MATH UTIL
$(eval $(call link-program,RunDownloadFlight,RUN_DOWNLOAD_FLIGHT))

CAI302_TOOL_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(SRC)/OS/LogError.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/CAI302Tool.cpp
CAI302_TOOL_DEPENDS = CAI302 PORT ASYNC OS THREAD IO TIME MATH UTIL
$(eval $(call link-program,CAI302Tool,CAI302_TOOL))

TEST_LXN_TO_IGC_SOURCES = \
	$(SRC)/Device/Driver/LX/Convert.cpp \
	$(SRC)/Device/Driver/LX/LXN.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestLXNToIGC.cpp
TEST_LXN_TO_IGC_DEPENDS =
$(eval $(call link-program,TestLXNToIGC,TEST_LXN_TO_IGC))

LXN2IGC_SOURCES = \
	$(SRC)/Device/Driver/LX/Convert.cpp \
	$(SRC)/Device/Driver/LX/LXN.cpp \
	$(TEST_SRC_DIR)/lxn2igc.cpp
$(eval $(call link-program,lxn2igc,LXN2IGC))

RUN_IGC_WRITER_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Version.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(SRC)/IGC/IGCFix.cpp \
	$(SRC)/IGC/IGCWriter.cpp \
	$(SRC)/IGC/IGCString.cpp \
	$(SRC)/Logger/LoggerFRecord.cpp \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/LoggerEPE.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(TEST_SRC_DIR)/RunIGCWriter.cpp
RUN_IGC_WRITER_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_IGC_WRITER_DEPENDS = GEO MATH UTIL TIME
$(eval $(call link-program,RunIGCWriter,RUN_IGC_WRITER))

RUN_FLIGHT_LOGGER_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Logger/FlightLogger.cpp \
	$(TEST_SRC_DIR)/RunFlightLogger.cpp
RUN_FLIGHT_LOGGER_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_FLIGHT_LOGGER_DEPENDS = GEO MATH UTIL TIME
$(eval $(call link-program,RunFlightLogger,RUN_FLIGHT_LOGGER))

RUN_FLYING_COMPUTER_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/RunFlyingComputer.cpp
RUN_FLYING_COMPUTER_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_FLYING_COMPUTER_DEPENDS = GEO MATH UTIL TIME
$(eval $(call link-program,RunFlyingComputer,RUN_FLYING_COMPUTER))

RUN_CIRCLING_WIND_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Computer/Wind/CirclingWind.cpp \
	$(TEST_SRC_DIR)/RunCirclingWind.cpp
RUN_CIRCLING_WIND_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_CIRCLING_WIND_DEPENDS = GEO MATH UTIL TIME
$(eval $(call link-program,RunCirclingWind,RUN_CIRCLING_WIND))

RUN_WIND_EKF_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Computer/Wind/WindEKF.cpp \
	$(SRC)/Computer/Wind/WindEKFGlue.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(TEST_SRC_DIR)/RunWindEKF.cpp
RUN_WIND_EKF_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_WIND_EKF_DEPENDS = GEO MATH UTIL TIME
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
	$(TEST_SRC_DIR)/RunWindComputer.cpp
RUN_WIND_COMPUTER_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_WIND_COMPUTER_DEPENDS = GEO MATH UTIL TIME
$(eval $(call link-program,RunWindComputer,RUN_WIND_COMPUTER))

RUN_TASK_SOURCES = \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/TaskFileIGC.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(SRC)/XML/DataNode.cpp \
	$(SRC)/XML/DataNodeXML.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(DEBUG_REPLAY_SOURCES) \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunTask.cpp
RUN_TASK_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_TASK_DEPENDS = TASK WAYPOINT GLIDE GEO MATH UTIL IO TIME
$(eval $(call link-program,RunTask,RUN_TASK))

RUN_TRACE_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideSettings.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/RunTrace.cpp
RUN_TRACE_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_TRACE_DEPENDS = UTIL GEO MATH TIME
$(eval $(call link-program,RunTrace,RUN_TRACE))

RUN_OLC_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/ContestPrinting.cpp \
	$(TEST_SRC_DIR)/RunOLCAnalysis.cpp
RUN_OLC_LDADD = $(DEBUG_REPLAY_LDADD)
RUN_OLC_DEPENDS = CONTEST UTIL GEO MATH TIME
$(eval $(call link-program,RunOLCAnalysis,RUN_OLC))

ANALYSE_FLIGHT_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/JSON/Writer.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/ContestPrinting.cpp \
	$(TEST_SRC_DIR)/FlightPhaseJSON.cpp \
	$(TEST_SRC_DIR)/FlightPhaseDetector.cpp \
	$(TEST_SRC_DIR)/AnalyseFlight.cpp
ANALYSE_FLIGHT_LDADD = $(DEBUG_REPLAY_LDADD)
ANALYSE_FLIGHT_DEPENDS = CONTEST UTIL GEO MATH TIME
$(eval $(call link-program,AnalyseFlight,ANALYSE_FLIGHT))

FLIGHT_PATH_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideSettings.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/FlightPath.cpp
FLIGHT_PATH_LDADD = $(DEBUG_REPLAY_LDADD)
FLIGHT_PATH_DEPENDS = UTIL GEO MATH TIME
$(eval $(call link-program,FlightPath,FLIGHT_PATH))

LOAD_IMAGE_SOURCES = \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/ResourceLoader.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/LoadImage.cpp
LOAD_IMAGE_LDADD = $(FAKE_LIBS)
LOAD_IMAGE_DEPENDS = SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,LoadImage,LOAD_IMAGE))

VIEW_IMAGE_SOURCES = \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/ResourceLoader.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/ViewImage.cpp
VIEW_IMAGE_LDADD = $(FAKE_LIBS)
VIEW_IMAGE_DEPENDS = SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,ViewImage,VIEW_IMAGE))

RUN_CANVAS_SOURCES = \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/ResourceLoader.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/RunCanvas.cpp
RUN_CANVAS_LDADD = $(FAKE_LIBS)
RUN_CANVAS_DEPENDS = SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,RunCanvas,RUN_CANVAS))

RUN_MAP_WINDOW_SOURCES = \
	$(CONTEST_SRC_DIR)/Settings.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Engine/Trace/Point.cpp \
	$(SRC)/Engine/Trace/Trace.cpp \
	$(SRC)/Engine/Trace/Vector.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(IO_SRC_DIR)/DataFile.cpp \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/MoreData.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/ThermalLocator.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/NMEA/Derived.cpp \
	$(SRC)/NMEA/VarioInfo.cpp \
	$(SRC)/NMEA/ClimbInfo.cpp \
	$(SRC)/NMEA/ClimbHistory.cpp \
	$(SRC)/NMEA/CirclingInfo.cpp \
	$(SRC)/NMEA/SwitchState.cpp \
	$(SRC)/NMEA/ThermalBand.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/FLARM/Friends.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/FLARM/Global.cpp \
	$(SRC)/Airspace/ActivePredicate.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Airspace/AirspaceVisibility.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(SRC)/Renderer/AirspaceRendererSettings.cpp \
	$(SRC)/Renderer/BackgroundRenderer.cpp \
	$(SRC)/Renderer/MarkerRenderer.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/Projection/CompareProjection.cpp \
	$(SRC)/MapWindow/MapWindow.cpp \
	$(SRC)/MapWindow/MapWindowBlackboard.cpp \
	$(SRC)/MapWindow/MapWindowEvents.cpp \
	$(SRC)/MapWindow/MapWindowGlideRange.cpp \
	$(SRC)/Projection/MapWindowProjection.cpp \
	$(SRC)/MapWindow/MapWindowRender.cpp \
	$(SRC)/MapWindow/MapWindowSymbols.cpp \
	$(SRC)/MapWindow/MapWindowContest.cpp \
	$(SRC)/MapWindow/MapWindowTask.cpp \
	$(SRC)/MapWindow/MapWindowThermal.cpp \
	$(SRC)/MapWindow/MapWindowTraffic.cpp \
	$(SRC)/MapWindow/MapWindowTrail.cpp \
	$(SRC)/MapWindow/MapWindowWaypoints.cpp \
	$(SRC)/MapWindow/MapCanvas.cpp \
	$(SRC)/MapWindow/MapDrawHelper.cpp \
	$(SRC)/Renderer/FAITriangleAreaRenderer.cpp \
	$(SRC)/Renderer/OZRenderer.cpp \
	$(SRC)/Renderer/TaskRenderer.cpp \
	$(SRC)/Renderer/TaskPointRenderer.cpp \
	$(SRC)/Renderer/AircraftRenderer.cpp \
	$(SRC)/Renderer/AirspaceRenderer.cpp \
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
	$(SRC)/Markers/Markers.cpp \
	$(SRC)/Markers/ProtectedMarkers.cpp \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Renderer/LabelBlock.cpp \
	$(SRC)/Look/Fonts.cpp \
	$(SRC)/Renderer/TextInBox.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/Look/MapLook.cpp \
	$(SRC)/Look/WindArrowLook.cpp \
	$(SRC)/Look/WaypointLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(SRC)/Look/TrailLook.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Look/AircraftLook.cpp \
	$(SRC)/Look/TrafficLook.cpp \
	$(SRC)/Look/MarkerLook.cpp \
	$(SRC)/Look/NOAALook.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/MapSettings.cpp \
	$(SRC)/Computer/Settings.cpp \
	$(SRC)/Computer/Wind/Settings.cpp \
	$(SRC)/Audio/VegaVoiceSettings.cpp \
	$(SRC)/TeamCode/Settings.cpp \
	$(SRC)/Logger/Settings.cpp \
	$(SRC)/Tracking/TrackingSettings.cpp \
	$(SRC)/Computer/TraceComputer.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileIGC.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Task/ProtectedRoutePlanner.cpp \
	$(SRC)/Task/RoutePlannerGlue.cpp \
	$(SRC)/Topography/TopographyFile.cpp \
	$(SRC)/Topography/TopographyStore.cpp \
	$(SRC)/Topography/TopographyFileRenderer.cpp \
	$(SRC)/Topography/TopographyRenderer.cpp \
	$(SRC)/Topography/TopographyGlue.cpp \
	$(SRC)/Topography/XShape.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Profile/Earth.cpp \
	$(SRC)/Profile/ComputerProfile.cpp \
	$(SRC)/Profile/TaskProfile.cpp \
	$(SRC)/Profile/RouteProfile.cpp \
	$(SRC)/Profile/ContestProfile.cpp \
	$(SRC)/Profile/AirspaceConfig.cpp \
	$(SRC)/Profile/TrackingProfile.cpp \
	$(SRC)/Profile/MapProfile.cpp \
	$(SRC)/Profile/TerrainConfig.cpp \
	$(SRC)/Profile/Screen.cpp \
	$(SRC)/Profile/FlarmProfile.cpp \
	$(SRC)/Waypoint/HomeGlue.cpp \
	$(SRC)/Waypoint/LastUsed.cpp \
	$(SRC)/Waypoint/WaypointFileType.cpp \
	$(SRC)/Waypoint/WaypointGlue.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointReaderCompeGPS.cpp \
	$(SRC)/Waypoint/WaypointWriter.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(SRC)/XML/DataNode.cpp \
	$(SRC)/XML/DataNodeXML.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Atmosphere/AirDensity.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfileGlue.cpp \
	$(TEST_SRC_DIR)/RunMapWindow.cpp

ifeq ($(HAVE_HTTP),y)
RUN_MAP_WINDOW_SOURCES += \
	$(SRC)/Weather/NOAAGlue.cpp \
	$(SRC)/Weather/NOAAStore.cpp
endif

RUN_MAP_WINDOW_LDADD = $(RESOURCE_BINARY)
RUN_MAP_WINDOW_DEPENDS = PROFILE TERRAIN SCREEN EVENT SHAPELIB IO OS THREAD TASK ROUTE GLIDE WAYPOINT AIRSPACE JASPER ZZIP UTIL GEO MATH TIME
$(eval $(call link-program,RunMapWindow,RUN_MAP_WINDOW))

RUN_DIALOG_SOURCES = \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/Inflate.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Dialogs/HelpDialog.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunDialog.cpp \
	$(SRC)/Compatibility/fmode.c
RUN_DIALOG_LDADD = \
	$(RESOURCE_BINARY) \
	$(FAKE_LIBS)
RUN_DIALOG_DEPENDS = GEO IO FORM WIDGET DATA_FIELD SCREEN EVENT OS THREAD MATH ZZIP UTIL TIME
$(eval $(call link-program,RunDialog,RUN_DIALOG))

RUN_LIST_CONTROL_SOURCES = \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/UIUtil/KineticManager.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/RunListControl.cpp
RUN_LIST_CONTROL_DEPENDS = FORM SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,RunListControl,RUN_LIST_CONTROL))

RUN_TEXT_ENTRY_SOURCES = \
	$(SRC)/Dialogs/TextEntry.cpp \
	$(SRC)/Dialogs/KnobTextEntry.cpp \
	$(SRC)/Dialogs/TouchTextEntry.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/Inflate.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/UIUtil/KineticManager.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunTextEntry.cpp
RUN_TEXT_ENTRY_LDADD = $(RESOURCE_BINARY)
RUN_TEXT_ENTRY_DEPENDS = GEO FORM WIDGET DATA_FIELD SCREEN EVENT IO OS THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunTextEntry,RUN_TEXT_ENTRY))

RUN_NUMBER_ENTRY_SOURCES = \
	$(SRC)/Dialogs/NumberEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunNumberEntry.cpp
RUN_NUMBER_ENTRY_LDADD = $(RESOURCE_BINARY)
RUN_NUMBER_ENTRY_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT IO OS THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunNumberEntry,RUN_NUMBER_ENTRY))

RUN_TIME_ENTRY_SOURCES = \
	$(SRC)/Dialogs/TimeEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunTimeEntry.cpp
RUN_TIME_ENTRY_LDADD = $(RESOURCE_BINARY)
RUN_TIME_ENTRY_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT IO OS THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunTimeEntry,RUN_TIME_ENTRY))

RUN_ANGLE_ENTRY_SOURCES = \
	$(SRC)/Dialogs/NumberEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunAngleEntry.cpp
RUN_ANGLE_ENTRY_LDADD = $(RESOURCE_BINARY)
RUN_ANGLE_ENTRY_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT IO OS THREAD MATH UTIL ZLIB TIME
$(eval $(call link-program,RunAngleEntry,RUN_ANGLE_ENTRY))

RUN_GEOPOINT_ENTRY_SOURCES = \
	$(SRC)/Dialogs/GeoPointEntry.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/RunGeoPointEntry.cpp
RUN_GEOPOINT_ENTRY_LDADD = $(RESOURCE_BINARY)
RUN_GEOPOINT_ENTRY_DEPENDS = GEO FORM WIDGET DATA_FIELD SCREEN EVENT IO OS THREAD MATH UTIL ZLIB GEOPOINT
$(eval $(call link-program,RunGeoPointEntry,RUN_GEOPOINT_ENTRY))

RUN_TERMINAL_SOURCES = \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/TerminalWindow.cpp \
	$(SRC)/Look/TerminalLook.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunTerminal.cpp
RUN_TERMINAL_DEPENDS = SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,RunTerminal,RUN_TERMINAL))

RUN_RENDER_OZ_SOURCES = \
	$(SRC)/Renderer/OZRenderer.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Renderer/AirspaceRendererSettings.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunRenderOZ.cpp
RUN_RENDER_OZ_LDADD = $(RESOURCE_BINARY)
RUN_RENDER_OZ_DEPENDS = TASK FORM SCREEN EVENT OS THREAD GEO MATH UTIL
$(eval $(call link-program,RunRenderOZ,RUN_RENDER_OZ))

RUN_CHART_RENDERER_SOURCES = \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/ChartLook.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Renderer/ChartRenderer.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunChartRenderer.cpp
RUN_CHART_RENDERER_LDADD = $(RESOURCE_BINARY)
RUN_CHART_RENDERER_DEPENDS = FORM SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,RunChartRenderer,RUN_CHART_RENDERER))

RUN_WIND_ARROW_RENDERER_SOURCES = \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Renderer/LabelBlock.cpp \
	$(SRC)/Renderer/TextInBox.cpp \
	$(SRC)/Look/WindArrowLook.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Renderer/WindArrowRenderer.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunWindArrowRenderer.cpp
RUN_WIND_ARROW_RENDERER_LDADD = $(RESOURCE_BINARY)
RUN_WIND_ARROW_RENDERER_DEPENDS = FORM SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,RunWindArrowRenderer,RUN_WIND_ARROW_RENDERER))

RUN_HORIZON_RENDERER_SOURCES = \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Look/HorizonLook.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Renderer/HorizonRenderer.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/RunHorizonRenderer.cpp
RUN_HORIZON_RENDERER_LDADD = $(RESOURCE_BINARY)
RUN_HORIZON_RENDERER_DEPENDS = FORM SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,RunHorizonRenderer,RUN_HORIZON_RENDERER))

RUN_FINAL_GLIDE_BAR_RENDERER_SOURCES = \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Renderer/LabelBlock.cpp \
	$(SRC)/Renderer/TextInBox.cpp \
	$(SRC)/Look/FinalGlideBarLook.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Renderer/FinalGlideBarRenderer.cpp \
	$(SRC)/NMEA/Derived.cpp \
	$(SRC)/NMEA/VarioInfo.cpp \
	$(SRC)/NMEA/CirclingInfo.cpp \
	$(SRC)/NMEA/ClimbHistory.cpp \
	$(SRC)/NMEA/ThermalBand.cpp \
	$(SRC)/NMEA/ClimbInfo.cpp \
	$(SRC)/NMEA/ThermalLocator.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunFinalGlideBarRenderer.cpp
RUN_FINAL_GLIDE_BAR_RENDERER_LDADD = $(RESOURCE_BINARY)
RUN_FINAL_GLIDE_BAR_RENDERER_DEPENDS = FORM SCREEN EVENT OS THREAD TASK GLIDE GEO MATH UTIL
$(eval $(call link-program,RunFinalGlideBarRenderer,RUN_FINAL_GLIDE_BAR_RENDERER))

RUN_FAI_TRIANGLE_SECTOR_RENDERER_SOURCES = \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Renderer/FAITriangleAreaRenderer.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(ENGINE_SRC_DIR)/Task/Shapes/FAITriangleArea.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunFAITriangleSectorRenderer.cpp
RUN_FAI_TRIANGLE_SECTOR_RENDERER_LDADD = $(RESOURCE_BINARY)
RUN_FAI_TRIANGLE_SECTOR_RENDERER_DEPENDS = FORM SCREEN EVENT OS THREAD GEO MATH UTIL
$(eval $(call link-program,RunFAITriangleSectorRenderer,RUN_FAI_TRIANGLE_SECTOR_RENDERER))

RUN_PROGRESS_WINDOW_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/ProgressWindow.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunProgressWindow.cpp
RUN_PROGRESS_WINDOW_LDADD = $(RESOURCE_BINARY)
RUN_PROGRESS_WINDOW_DEPENDS = SCREEN EVENT OS THREAD MATH UTIL
$(eval $(call link-program,RunProgressWindow,RUN_PROGRESS_WINDOW))

RUN_JOB_DIALOG_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ThreadedOperationEnvironment.cpp \
	$(SRC)/Job/Thread.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/ProgressWindow.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Dialogs/JobDialog.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunJobDialog.cpp
RUN_JOB_DIALOG_LDADD = $(RESOURCE_BINARY)
RUN_JOB_DIALOG_DEPENDS = THREAD FORM SCREEN EVENT OS MATH UTIL
$(eval $(call link-program,RunJobDialog,RUN_JOB_DIALOG))

RUN_ANALYSIS_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Engine/Trace/Point.cpp \
	$(SRC)/Engine/Trace/Trace.cpp \
	$(SRC)/Engine/Trace/Vector.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/UIUtil/GestureManager.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Task/ProtectedRoutePlanner.cpp \
	$(SRC)/Task/RoutePlannerGlue.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/TaskFileIGC.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
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
	$(SRC)/Renderer/GradientRenderer.cpp \
	$(SRC)/Renderer/ChartRenderer.cpp \
	$(SRC)/Renderer/TaskRenderer.cpp \
	$(SRC)/Renderer/TaskPointRenderer.cpp \
	$(SRC)/Renderer/OZRenderer.cpp \
	$(SRC)/Renderer/AircraftRenderer.cpp \
	$(SRC)/Renderer/TrailRenderer.cpp \
	$(SRC)/MapWindow/MapCanvas.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/Look/Look.cpp \
	$(SRC)/Look/UnitsLook.cpp \
	$(SRC)/Look/IconLook.cpp \
	$(SRC)/Look/MapLook.cpp \
	$(SRC)/Look/WindArrowLook.cpp \
	$(SRC)/Look/VarioLook.cpp \
	$(SRC)/Look/ChartLook.cpp \
	$(SRC)/Look/ThermalBandLook.cpp \
	$(SRC)/Look/TraceHistoryLook.cpp \
	$(SRC)/Look/CrossSectionLook.cpp \
	$(SRC)/Look/HorizonLook.cpp \
	$(SRC)/Look/WaypointLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Look/AircraftLook.cpp \
	$(SRC)/Look/TrafficLook.cpp \
	$(SRC)/Look/GestureLook.cpp \
	$(SRC)/Look/InfoBoxLook.cpp \
	$(SRC)/Look/MarkerLook.cpp \
	$(SRC)/Look/NOAALook.cpp \
	$(SRC)/Look/TerminalLook.cpp \
	$(SRC)/Look/TrailLook.cpp \
	$(SRC)/Look/FinalGlideBarLook.cpp \
	$(SRC)/Look/FlarmTrafficLook.cpp \
	$(SRC)/Look/ThermalAssistantLook.cpp \
	$(SRC)/Look/VarioBarLook.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Profile/FontConfig.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(SRC)/XML/DataNode.cpp \
	$(SRC)/XML/DataNodeXML.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/Inflate.cpp \
	$(SRC)/Dialogs/dlgAnalysis.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
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
	$(SRC)/Renderer/ThermalBandRenderer.cpp \
	$(SRC)/Renderer/WindChartRenderer.cpp \
	$(SRC)/Renderer/CuRenderer.cpp \
	$(SRC)/Computer/ThermalLocator.cpp \
	$(SRC)/Computer/ThermalBase.cpp \
	$(SRC)/Computer/ThermalBandComputer.cpp \
	$(SRC)/Computer/GlideRatioCalculator.cpp \
	$(SRC)/Computer/AutoQNH.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Computer/Wind/Computer.cpp \
	$(SRC)/Computer/Wind/Settings.cpp \
	$(SRC)/Computer/ContestComputer.cpp \
	$(SRC)/Computer/TraceComputer.cpp \
	$(SRC)/Computer/WarningComputer.cpp \
	$(SRC)/Computer/LiftDatabaseComputer.cpp \
	$(SRC)/Computer/AverageVarioComputer.cpp \
	$(SRC)/Computer/GlideRatioComputer.cpp \
	$(SRC)/Computer/GlideComputer.cpp \
	$(SRC)/Computer/GlideComputerBlackboard.cpp \
	$(SRC)/Computer/TaskComputer.cpp \
	$(SRC)/Computer/RouteComputer.cpp \
	$(SRC)/Computer/GlideComputerAirData.cpp \
	$(SRC)/Computer/StatsComputer.cpp \
	$(SRC)/Computer/GlideComputerInterface.cpp \
	$(SRC)/Computer/LogComputer.cpp \
	$(SRC)/Computer/CuComputer.cpp \
	$(SRC)/Audio/Settings.cpp \
	$(SRC)/Audio/VarioSettings.cpp \
	$(SRC)/Audio/VegaVoiceSettings.cpp \
	$(SRC)/UISettings.cpp \
	$(SRC)/DisplaySettings.cpp \
	$(SRC)/PageSettings.cpp \
	$(SRC)/InfoBoxes/InfoBoxSettings.cpp \
	$(SRC)/Gauge/VarioSettings.cpp \
	$(SRC)/Gauge/TrafficSettings.cpp \
	$(SRC)/Computer/Settings.cpp \
	$(SRC)/TeamCode/TeamCode.cpp \
	$(SRC)/TeamCode/Settings.cpp \
	$(SRC)/Logger/Settings.cpp \
	$(SRC)/Tracking/TrackingSettings.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/MapSettings.cpp \
	$(SRC)/Blackboard/InterfaceBlackboard.cpp \
	$(SRC)/Audio/VegaVoice.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(SRC)/Airspace/ActivePredicate.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Airspace/AirspaceGlue.cpp \
	$(SRC)/Airspace/AirspaceVisibility.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(SRC)/Renderer/AirspaceRendererSettings.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	$(SRC)/IO/ConfiguredFile.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunAnalysis.cpp
RUN_ANALYSIS_LDADD = \
	$(RESOURCE_BINARY)
RUN_ANALYSIS_DEPENDS = TERRAIN DRIVER PROFILE FORM WIDGET SCREEN EVENT IO DATA_FIELD OS THREAD CONTEST TASK ROUTE GLIDE WAYPOINT ROUTE AIRSPACE ZZIP UTIL GEO MATH TIME
$(eval $(call link-program,RunAnalysis,RUN_ANALYSIS))

RUN_AIRSPACE_WARNING_DIALOG_SOURCES = \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/AirspaceFormatter.cpp \
	$(SRC)/Formatter/AirspaceUserUnitsFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Dialogs/Airspace/dlgAirspaceWarnings.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Audio/Sound.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeHelpDialog.cpp \
	$(TEST_SRC_DIR)/FakeListPicker.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProfileGlue.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/RunAirspaceWarningDialog.cpp
RUN_AIRSPACE_WARNING_DIALOG_LDADD = \
	$(FAKE_LIBS) \
	$(RESOURCE_BINARY)
RUN_AIRSPACE_WARNING_DIALOG_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT IO OS THREAD AIRSPACE ZZIP UTIL GEO MATH TIME
$(eval $(call link-program,RunAirspaceWarningDialog,RUN_AIRSPACE_WARNING_DIALOG))

PLAY_TONE_SOURCES = \
	$(TEST_SRC_DIR)/PlayTone.cpp
PLAY_TONE_DEPENDS = AUDIO MATH SCREEN EVENT OS
$(eval $(call link-program,PlayTone,PLAY_TONE))

PLAY_VARIO_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(TEST_SRC_DIR)/PlayVario.cpp
PLAY_VARIO_LDADD = $(filter-out $(OS_LIBS),$(DEBUG_REPLAY_LDADD))
PLAY_VARIO_DEPENDS = AUDIO GEO MATH SCREEN EVENT UTIL OS TIME
$(eval $(call link-program,PlayVario,PLAY_VARIO))

DUMP_VARIO_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(TEST_SRC_DIR)/DumpVario.cpp
DUMP_VARIO_LDADD = $(DEBUG_REPLAY_LDADD)
DUMP_VARIO_DEPENDS = AUDIO GEO MATH SCREEN EVENT UTIL OS TIME
$(eval $(call link-program,DumpVario,DUMP_VARIO))

RUN_TASK_EDITOR_DIALOG_SOURCES = \
	$(SRC)/XML/Node.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/Inflate.cpp \
	$(SRC)/Dialogs/ComboPicker.cpp \
	$(SRC)/Dialogs/HelpDialog.cpp \
	$(SRC)/Dialogs/dlgTaskOverview.cpp \
	$(SRC)/Dialogs/WaypointList.cpp \
	$(SRC)/Dialogs/dlgWaypointDetails.cpp \
	$(SRC)/Dialogs/dlgTaskWaypoint.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Look/Fonts.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileIGC.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Waypoint/WaypointFileType.cpp \
	$(SRC)/Waypoint/WaypointGlue.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointReaderCompeGPS.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunTaskEditorDialog.cpp
RUN_TASK_EDITOR_DIALOG_LDADD = \
	$(FAKE_LIBS) \
	$(RESOURCE_BINARY)
RUN_TASK_EDITOR_DIALOG_DEPENDS = FORM WIDGET DATA_FIELD SCREEN EVENT IO OS THREAD ZZIP UTIL GEO
$(eval $(call link-program,RunTaskEditorDialog,RUN_TASK_EDITOR_DIALOG))

TEST_NOTIFY_SOURCES = \
	$(SRC)/Event/Idle.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestNotify.cpp
TEST_NOTIFY_DEPENDS = EVENT SCREEN MATH UTIL OS THREAD
$(eval $(call link-program,TestNotify,TEST_NOTIFY))

FEED_NMEA_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/FeedNMEA.cpp
FEED_NMEA_DEPENDS = PORT ASYNC OS THREAD UTIL
$(eval $(call link-program,FeedNMEA,FEED_NMEA))

FEED_VEGA_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/FeedVega.cpp
FEED_VEGA_DEPENDS = PORT ASYNC OS THREAD UTIL
$(eval $(call link-program,FeedVega,FEED_VEGA))

EMULATE_DEVICE_SOURCES = \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Port/LineSplitter.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Driver/FLARM/BinaryProtocol.cpp \
	$(SRC)/Device/Driver/FLARM/CRC16.cpp \
	$(SRC)/Device/Config.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/IO/CSVLine.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/DebugPort.cpp \
	$(TEST_SRC_DIR)/EmulateDevice.cpp
EMULATE_DEVICE_DEPENDS = PORT ASYNC OS THREAD UTIL
$(eval $(call link-program,EmulateDevice,EMULATE_DEVICE))

FEED_TCP_SOURCES = \
	$(TEST_SRC_DIR)/FeedTCP.cpp
FEED_TCP_DEPENDS = OS

$(eval $(call link-program,FeedTCP,FEED_TCP))

FEED_FLYNET_DATA_SOURCES = \
	$(SRC)/Math/fixed.cpp \
	$(TEST_SRC_DIR)/FeedFlyNetData.cpp
FEED_FLYNET_DATA_DEPENDS = OS UTIL

$(eval $(call link-program,FeedFlyNetData,FEED_FLYNET_DATA))

TASK_INFO_SOURCES = \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(SRC)/XML/DataNode.cpp \
	$(SRC)/XML/DataNodeXML.cpp \
	$(TEST_SRC_DIR)/TaskInfo.cpp
TASK_INFO_DEPENDS = TASK ROUTE GLIDE WAYPOINT IO OS GEO TIME MATH UTIL
$(eval $(call link-program,TaskInfo,TASK_INFO))

DUMP_TASK_FILE_SOURCES = \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(SRC)/XML/DataNode.cpp \
	$(SRC)/XML/DataNodeXML.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileIGC.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/DumpTaskFile.cpp
DUMP_TASK_FILE_DEPENDS = TASK GLIDE WAYPOINT IO OS THREAD ZZIP GEO TIME MATH UTIL
$(eval $(call link-program,DumpTaskFile,DUMP_TASK_FILE))

DUMP_FLARM_NET_SOURCES = \
	$(SRC)/FLARM/FlarmNetReader.cpp \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/FLARM/FlarmNetRecord.cpp \
	$(SRC)/FLARM/FlarmNetDatabase.cpp \
	$(TEST_SRC_DIR)/DumpFlarmNet.cpp
DUMP_FLARM_NET_DEPENDS = IO OS MATH UTIL
$(eval $(call link-program,DumpFlarmNet,DUMP_FLARM_NET))

IGC2NMEA_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(TEST_SRC_DIR)/IGC2NMEA.cpp
IGC2NMEA_DEPENDS = GEO MATH UTIL TIME
IGC2NMEA_LDADD = $(DEBUG_REPLAY_LDADD)

$(eval $(call link-program,IGC2NMEA,IGC2NMEA))

TODAY_INSTALL_SOURCES = \
	$(TEST_SRC_DIR)/TodayInstall.cpp
$(eval $(call link-program,TodayInstall,TODAY_INSTALL))

debug: $(DEBUG_PROGRAMS)
