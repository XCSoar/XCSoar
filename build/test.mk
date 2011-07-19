TESTFAST = \
	$(TARGET_BIN_DIR)/test_normalise$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_fixed$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_waypoints$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_pressure$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_mc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_task$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_modes$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_automc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_acfilter$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_trees$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_vopt$(TARGET_EXEEXT)

TESTSLOW = \
	$(TARGET_BIN_DIR)/test_bestcruisetrack$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_airspace$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_effectivemc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_cruiseefficiency$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_highterrain$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_randomtask$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_flight$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_olc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_aat$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_replay_olc$(TARGET_EXEEXT)

HARNESS_PROGRAMS = $(TESTFAST) $(TESTSLOW)

build-harness: $(HARNESS_PROGRAMS)

testslow: $(TESTSLOW)
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(TESTSLOW)

testfast: $(TESTFAST)
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(TESTFAST)

TESTLIBS1 = $(HARNESS_LIBS) \
	   $(ZZIP_LIBS) \
	$(ENGINE_CORE_LIBS) \
	$(IO_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)

TESTLIBS = $(TESTLIBS1) \
	$(TARGET_OUTPUT_DIR)/test/src/FakeTerrain.o

$(HARNESS_PROGRAMS): $(TARGET_BIN_DIR)/%$(TARGET_EXEEXT): $(TARGET_OUTPUT_DIR)/test/src/%.o $(TESTLIBS) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_NAMES = \
	test_fixed \
	test_normalise \
	test_waypoints \
	test_pressure \
	test_task \
	TestOverwritingRingBuffer \
	TestDateTime \
	TestMathTables \
	TestAngle TestUnits TestEarth TestSunEphemeris \
	TestValidity TestUTM TestProfile \
	TestRadixTree TestGeoBounds TestGeoClip \
	TestLogger TestDriver TestClimbAvCalc \
	TestWaypointReader TestThermalBase \
	test_load_task TestFlarmNet \
	TestColorRamp TestGeoPoint TestDiffFilter \
	TestFileUtil TestPolars TestCSVLine TestGlidePolar \
	test_replay_task TestProjection TestFlatPoint TestFlatLine TestFlatGeoPoint \
	TestPlanes

TESTS = $(patsubst %,$(TARGET_BIN_DIR)/%$(TARGET_EXEEXT),$(TEST_NAMES))

TEST_TEST_OVERWRITING_RING_BUFFER_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestOverwritingRingBuffer.cpp
TEST_TEST_OVERWRITING_RING_BUFFER_OBJS = $(call SRC_TO_OBJ,$(TEST_TEST_OVERWRITING_RING_BUFFER_SOURCES))
TEST_TEST_OVERWRITING_RING_BUFFER_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestOverwritingRingBuffer$(TARGET_EXEEXT): $(TEST_TEST_OVERWRITING_RING_BUFFER_OBJS) $(TEST_TEST_OVERWRITING_RING_BUFFER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_DATE_TIME_SOURCES = \
	$(SRC)/DateTime.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestDateTime.cpp
TEST_DATE_TIME_OBJS = $(call SRC_TO_OBJ,$(TEST_DATE_TIME_SOURCES))
TEST_DATE_TIME_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestDateTime$(TARGET_EXEEXT): $(TEST_DATE_TIME_OBJS) $(TEST_DATE_TIME_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_PROFILE_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/ProfileMap.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/TestProfile.cpp
TEST_PROFILE_OBJS = $(call SRC_TO_OBJ,$(TEST_PROFILE_SOURCES))
TEST_PROFILE_LDADD = $(MATH_LIBS) $(IO_LIBS)
$(TARGET_BIN_DIR)/TestProfile$(TARGET_EXEEXT): $(TEST_PROFILE_OBJS) $(TEST_PROFILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_PLANES_SOURCES = \
	$(SRC)/Plane/PlaneFileGlue.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestPlanes.cpp
TEST_PLANES_OBJS = $(call SRC_TO_OBJ,$(TEST_PLANES_SOURCES))
TEST_PLANES_LDADD = $(MATH_LIBS) $(IO_LIBS)
$(TARGET_BIN_DIR)/TestPlanes$(TARGET_EXEEXT): $(TEST_PLANES_OBJS) $(TEST_PLANES_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_TROUTE_SOURCES = \
	$(SRC)/Engine/Util/DataNodeXML.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Geo/GeoClip.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Engine/Math/Earth.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/test_troute.cpp
TEST_TROUTE_OBJS = $(call SRC_TO_OBJ,$(TEST_TROUTE_SOURCES))
TEST_TROUTE_BIN = $(TARGET_BIN_DIR)/test_troute$(TARGET_EXEEXT)
TEST_TROUTE_LDADD = $(TESTLIBS1) \
	$(MATH_LIBS) \
	$(IO_LIBS) \
	$(JASPER_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TEST_TROUTE_BIN): $(TEST_TROUTE_OBJS) $(TEST_TROUTE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) $(ZZIP_LIBS) -o $@

TEST_REACH_SOURCES = \
	$(SRC)/Engine/Util/DataNodeXML.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Geo/GeoClip.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Engine/Math/Earth.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/test_reach.cpp
TEST_REACH_OBJS = $(call SRC_TO_OBJ,$(TEST_REACH_SOURCES))
TEST_REACH_BIN = $(TARGET_BIN_DIR)/test_reach$(TARGET_EXEEXT)
TEST_REACH_LDADD = $(TESTLIBS1) \
	$(MATH_LIBS) \
	$(IO_LIBS) \
	$(JASPER_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TEST_REACH_BIN): $(TEST_REACH_OBJS) $(TEST_REACH_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) $(ZZIP_LIBS) -o $@

TEST_ROUTE_SOURCES = \
	$(SRC)/Engine/Util/DataNodeXML.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Geo/GeoClip.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Engine/Math/Earth.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/test_route.cpp
TEST_ROUTE_OBJS = $(call SRC_TO_OBJ,$(TEST_ROUTE_SOURCES))
TEST_ROUTE_BIN = $(TARGET_BIN_DIR)/test_route$(TARGET_EXEEXT)
TEST_ROUTE_LDADD = $(TESTLIBS1) \
	$(MATH_LIBS) \
	$(IO_LIBS) \
	$(JASPER_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TEST_ROUTE_BIN): $(TEST_ROUTE_OBJS) $(TEST_ROUTE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) $(ZZIP_LIBS) -o $@

TEST_REPLAY_TASK_SOURCES = \
	$(SRC)/Engine/Util/DataNodeXML.cpp \
	$(SRC)/xmlParser.cpp \
	$(TEST_SRC_DIR)/test_replay_task.cpp
TEST_REPLAY_TASK_OBJS = $(call SRC_TO_OBJ,$(TEST_REPLAY_TASK_SOURCES))
TEST_REPLAY_TASK_LDADD = $(TESTLIBS)
$(TARGET_BIN_DIR)/test_replay_task$(TARGET_EXEEXT): $(TEST_REPLAY_TASK_OBJS) $(TEST_REPLAY_TASK_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_MATH_TABLES_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestMathTables.cpp
TEST_MATH_TABLES_OBJS = $(call SRC_TO_OBJ,$(TEST_MATH_TABLES_SOURCES))
TEST_MATH_TABLES_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestMathTables$(TARGET_EXEEXT): $(TEST_MATH_TABLES_OBJS) $(TEST_MATH_TABLES_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_LOAD_TASK_SOURCES = \
	$(SRC)/Engine/Util/DataNodeXML.cpp \
	$(SRC)/xmlParser.cpp \
	$(TEST_SRC_DIR)/test_load_task.cpp
TEST_LOAD_TASK_OBJS = $(call SRC_TO_OBJ,$(TEST_LOAD_TASK_SOURCES))
TEST_LOAD_TASK_LDADD = $(TESTLIBS)
$(TARGET_BIN_DIR)/test_load_task$(TARGET_EXEEXT): $(TEST_LOAD_TASK_OBJS) $(TEST_LOAD_TASK_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_ANGLE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestAngle.cpp
TEST_ANGLE_OBJS = $(call SRC_TO_OBJ,$(TEST_ANGLE_SOURCES))
TEST_ANGLE_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestAngle$(TARGET_EXEEXT): $(TEST_ANGLE_OBJS) $(TEST_ANGLE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_CSV_LINE_SOURCES = \
	$(SRC)/IO/CSVLine.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestCSVLine.cpp
TEST_CSV_LINE_OBJS = $(call SRC_TO_OBJ,$(TEST_CSV_LINE_SOURCES))
TEST_CSV_LINE_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestCSVLine$(TARGET_EXEEXT): $(TEST_CSV_LINE_OBJS) $(TEST_CSV_LINE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_GEO_BOUNDS_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGeoBounds.cpp
TEST_GEO_BOUNDS_OBJS = $(call SRC_TO_OBJ,$(TEST_GEO_BOUNDS_SOURCES))
TEST_GEO_BOUNDS_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestGeoBounds$(TARGET_EXEEXT): $(TEST_GEO_BOUNDS_OBJS) $(TEST_GEO_BOUNDS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_FLARM_NET_SOURCES = \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/FLARM/FlarmNet.cpp \
	$(SRC)/FLARM/FlarmId.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFlarmNet.cpp
TEST_FLARM_NET_OBJS = $(call SRC_TO_OBJ,$(TEST_FLARM_NET_SOURCES))
TEST_FLARM_NET_LDADD = $(MATH_LIBS) $(IO_LIBS)
$(TARGET_BIN_DIR)/TestFlarmNet$(TARGET_EXEEXT): $(TEST_FLARM_NET_OBJS) $(TEST_FLARM_NET_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_GEO_CLIP_SOURCES = \
	$(SRC)/Geo/GeoClip.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGeoClip.cpp
TEST_GEO_CLIP_OBJS = $(call SRC_TO_OBJ,$(TEST_GEO_CLIP_SOURCES))
TEST_GEO_CLIP_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestGeoClip$(TARGET_EXEEXT): $(TEST_GEO_CLIP_OBJS) $(TEST_GEO_CLIP_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_CLIMB_AV_CALC_SOURCES = \
	$(SRC)/ClimbAverageCalculator.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestClimbAvCalc.cpp
TEST_CLIMB_AV_CALC_OBJS = $(call SRC_TO_OBJ,$(TEST_CLIMB_AV_CALC_SOURCES))
TEST_CLIMB_AV_CALC_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestClimbAvCalc$(TARGET_EXEEXT): $(TEST_CLIMB_AV_CALC_OBJS) $(TEST_CLIMB_AV_CALC_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_PROJECTION_SOURCES = \
	$(SRC)/Projection.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestProjection.cpp
TEST_PROJECTION_OBJS = $(call SRC_TO_OBJ,$(TEST_PROJECTION_SOURCES))
TEST_PROJECTION_LDADD = $(MATH_LIBS)
$(TEST_PROJECTION_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/TestProjection$(TARGET_EXEEXT): $(TEST_PROJECTION_OBJS) $(TEST_PROJECTION_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_UNITS_SOURCES = \
	$(SRC)/Units/Units.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUnits.cpp
TEST_UNITS_OBJS = $(call SRC_TO_OBJ,$(TEST_UNITS_SOURCES))
TEST_UNITS_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestUnits$(TARGET_EXEEXT): $(TEST_UNITS_OBJS) $(TEST_UNITS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_POLARS_SOURCES = \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Polar/Polar.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/PolarCoefficients.cpp \
	$(SRC)/Polar/PolarFileGlue.cpp \
	$(SRC)/Polar/PolarStore.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestPolars.cpp
TEST_POLARS_OBJS = $(call SRC_TO_OBJ,$(TEST_POLARS_SOURCES))
TEST_POLARS_LDADD = $(MATH_LIBS) $(IO_LIBS)
$(TARGET_BIN_DIR)/TestPolars$(TARGET_EXEEXT): $(TEST_POLARS_OBJS) $(TEST_POLARS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_GLIDE_POLAR_SOURCES = \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlidePolar.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/PolarCoefficients.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideResult.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideState.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/MacCready.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Geometry/GeoVector.cpp \
	$(ENGINE_SRC_DIR)/Util/ZeroFinder.cpp \
	$(SRC)/Units/Units.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGlidePolar.cpp
TEST_GLIDE_POLAR_OBJS = $(call SRC_TO_OBJ,$(TEST_GLIDE_POLAR_SOURCES))
TEST_GLIDE_POLAR_LDADD = $(MATH_LIBS) $(IO_LIBS)
$(TARGET_BIN_DIR)/TestGlidePolar$(TARGET_EXEEXT): $(TEST_GLIDE_POLAR_OBJS) $(TEST_GLIDE_POLAR_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_FILE_UTIL_SOURCES = \
	$(SRC)/OS/FileUtil.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFileUtil.cpp
TEST_FILE_UTIL_OBJS = $(call SRC_TO_OBJ,$(TEST_FILE_UTIL_SOURCES))
TEST_FILE_UTIL_LDADD = 
$(TARGET_BIN_DIR)/TestFileUtil$(TARGET_EXEEXT): $(TEST_FILE_UTIL_OBJS) $(TEST_FILE_UTIL_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_GEO_POINT_SOURCES = \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestGeoPoint.cpp
TEST_GEO_POINT_OBJS = $(call SRC_TO_OBJ,$(TEST_GEO_POINT_SOURCES))
TEST_GEO_POINT_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestGeoPoint$(TARGET_EXEEXT): $(TEST_GEO_POINT_OBJS) $(TEST_GEO_POINT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_DIFF_FILTER_SOURCES = \
	$(ENGINE_SRC_DIR)/Util/DiffFilter.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestDiffFilter.cpp
TEST_DIFF_FILTER_OBJS = $(call SRC_TO_OBJ,$(TEST_DIFF_FILTER_SOURCES))
TEST_DIFF_FILTER_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestDiffFilter$(TARGET_EXEEXT): $(TEST_DIFF_FILTER_OBJS) $(TEST_DIFF_FILTER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_FLAT_POINT_SOURCES = \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatPoint.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFlatPoint.cpp
TEST_FLAT_POINT_OBJS = $(call SRC_TO_OBJ,$(TEST_FLAT_POINT_SOURCES))
TEST_FLAT_POINT_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestFlatPoint$(TARGET_EXEEXT): $(TEST_FLAT_POINT_OBJS) $(TEST_FLAT_POINT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_FLAT_GEO_POINT_SOURCES = \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatGeoPoint.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFlatGeoPoint.cpp
TEST_FLAT_GEO_POINT_OBJS = $(call SRC_TO_OBJ,$(TEST_FLAT_GEO_POINT_SOURCES))
TEST_FLAT_GEO_POINT_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestFlatGeoPoint$(TARGET_EXEEXT): $(TEST_FLAT_GEO_POINT_OBJS) $(TEST_FLAT_GEO_POINT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_FLAT_LINE_SOURCES = \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatLine.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestFlatLine.cpp
TEST_FLAT_LINE_OBJS = $(call SRC_TO_OBJ,$(TEST_FLAT_LINE_SOURCES))
TEST_FLAT_LINE_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestFlatLine$(TARGET_EXEEXT): $(TEST_FLAT_LINE_OBJS) $(TEST_FLAT_LINE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_THERMALBASE_SOURCES = \
	$(SRC)/ThermalBase.cpp \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestThermalBase.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp
TEST_THERMALBASE_OBJS = $(call SRC_TO_OBJ,$(TEST_THERMALBASE_SOURCES))
TEST_THERMALBASE_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestThermalBase$(TARGET_EXEEXT): $(TEST_THERMALBASE_OBJS) $(TEST_THERMALBASE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_EARTH_SOURCES = \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestEarth.cpp
TEST_EARTH_OBJS = $(call SRC_TO_OBJ,$(TEST_EARTH_SOURCES))
TEST_EARTH_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestEarth$(TARGET_EXEEXT): $(TEST_EARTH_OBJS) $(TEST_EARTH_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_COLOR_RAMP_SOURCES = \
	$(SRC)/Screen/Ramp.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestColorRamp.cpp
TEST_COLOR_RAMP_OBJS = $(call SRC_TO_OBJ,$(TEST_COLOR_RAMP_SOURCES))
TEST_COLOR_RAMP_LDADD = 
$(TEST_COLOR_RAMP_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/TestColorRamp$(TARGET_EXEEXT): $(TEST_COLOR_RAMP_OBJS) $(TEST_COLOR_RAMP_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_SUN_EPHEMERIS_SOURCES = \
	$(SRC)/Math/SunEphemeris.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestSunEphemeris.cpp
TEST_SUN_EPHEMERIS_OBJS = $(call SRC_TO_OBJ,$(TEST_SUN_EPHEMERIS_SOURCES))
TEST_SUN_EPHEMERIS_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestSunEphemeris$(TARGET_EXEEXT): $(TEST_SUN_EPHEMERIS_OBJS) $(TEST_SUN_EPHEMERIS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_UTM_SOURCES = \
	$(SRC)/Geo/UTM.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUTM.cpp
TEST_UTM_OBJS = $(call SRC_TO_OBJ,$(TEST_UTM_SOURCES))
TEST_UTM_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestUTM$(TARGET_EXEEXT): $(TEST_UTM_OBJS) $(TEST_UTM_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_VALIDITY_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestValidity.cpp
TEST_VALIDITY_OBJS = $(call SRC_TO_OBJ,$(TEST_VALIDITY_SOURCES))
TEST_VALIDITY_LDADD =
$(TARGET_BIN_DIR)/TestValidity$(TARGET_EXEEXT): $(TEST_VALIDITY_OBJS) $(TEST_VALIDITY_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_RADIX_TREE_SOURCES = \
	$(SRC)/Util/StringUtil.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestRadixTree.cpp
TEST_RADIX_TREE_OBJS = $(call SRC_TO_OBJ,$(TEST_RADIX_TREE_SOURCES))
$(TARGET_BIN_DIR)/TestRadixTree$(TARGET_EXEEXT): $(TEST_RADIX_TREE_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_LOGGER_SOURCES = \
	$(SRC)/Logger/IGCWriter.cpp \
	$(SRC)/Logger/LoggerFRecord.cpp \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/LoggerEPE.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/Math/fixed.cpp \
	$(SRC)/Math/Angle.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(ENGINE_SRC_DIR)/Atmosphere/Pressure.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Aircraft.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Geometry/GeoVector.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestLogger.cpp
TEST_LOGGER_OBJS = $(call SRC_TO_OBJ,$(TEST_LOGGER_SOURCES))
TEST_LOGGER_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/TestLogger$(TARGET_EXEEXT): $(TEST_LOGGER_OBJS) $(TEST_LOGGER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_DRIVER_SOURCES = \
	$(SRC)/Device/NullPort.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Driver/BorgeltB50.cpp \
	$(SRC)/Device/Driver/Condor.cpp \
	$(SRC)/Device/Driver/EW.cpp \
	$(SRC)/Device/Driver/EWMicroRecorder.cpp \
	$(SRC)/Device/Driver/FlymasterF1.cpp \
	$(SRC)/Device/Driver/Flytec.cpp \
	$(SRC)/Device/Driver/Leonardo.cpp \
	$(SRC)/Device/Driver/ILEC.cpp \
	$(SRC)/Device/Driver/Westerboer.cpp \
	$(SRC)/Device/Driver/PosiGraph.cpp \
	$(SRC)/Device/Driver/Vega.cpp \
	$(SRC)/Device/Driver/Zander.cpp \
	$(CAI302_SOURCES) \
	$(IMI_SOURCES) \
	$(LX_SOURCES) \
	$(VOLKSLOGGER_SOURCES) \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/IO/CSVLine.cpp \
	$(SRC)/FLARM/State.cpp \
	$(SRC)/Math/fixed.cpp \
	$(SRC)/Math/Angle.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/ClimbAverageCalculator.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/Operation.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(ENGINE_SRC_DIR)/Atmosphere/Pressure.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Aircraft.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Geometry/GeoVector.cpp \
	$(ENGINE_SRC_DIR)/Navigation/TaskProjection.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoint.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/TestDriver.cpp
TEST_DRIVER_OBJS = $(call SRC_TO_OBJ,$(TEST_DRIVER_SOURCES))
TEST_DRIVER_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestDriver$(TARGET_EXEEXT): $(TEST_DRIVER_OBJS) $(TEST_DRIVER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_WAY_POINT_FILE_SOURCES = \
	$(SRC)/Units/Units.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Geo/UTM.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointWriter.cpp \
	$(SRC)/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/TaskProjection.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatGeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoint.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoints.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestWaypointReader.cpp
TEST_WAY_POINT_FILE_OBJS = $(call SRC_TO_OBJ,$(TEST_WAY_POINT_FILE_SOURCES))
TEST_WAY_POINT_FILE_LDADD = $(UTIL_LIBS) $(MATH_LIBS) $(IO_LIBS) $(ZZIP_LIBS)
$(TARGET_BIN_DIR)/TestWaypointReader$(TARGET_EXEEXT): $(TEST_WAY_POINT_FILE_OBJS) $(TEST_WAY_POINT_FILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) -o $@

TEST_OLC_SOURCES = \
	$(SRC)/Math/fixed.cpp \
	$(SRC)/Math/Angle.cpp \
	$(SRC)/Math/FastMath.cpp \
	$(SRC)/Replay/IGCParser.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/TestOLC.cpp 
TEST_OLC_OBJS = $(call SRC_TO_OBJ,$(TEST_OLC_SOURCES))
TEST_OLC_LDADD = $(UTIL_LIBS) $(MATH_LIBS) $(IO_LIBS) $(ENGINE_LIBS)
$(TARGET_BIN_DIR)/TestOLC$(TARGET_EXEEXT): CPPFLAGS += -DDO_PRINT
$(TARGET_BIN_DIR)/TestOLC$(TARGET_EXEEXT): $(TEST_OLC_OBJS) $(TEST_OLC_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_TRACE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(SRC)/Math/fixed.cpp \
	$(SRC)/Math/Angle.cpp \
	$(SRC)/Math/FastMath.cpp \
	$(SRC)/Replay/IGCParser.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/TestTrace.cpp 
TEST_TRACE_OBJS = $(call SRC_TO_OBJ,$(TEST_TRACE_SOURCES))
TEST_TRACE_LDADD = $(UTIL_LIBS) $(MATH_LIBS) $(IO_LIBS) $(ENGINE_LIBS)
$(TARGET_BIN_DIR)/TestTrace$(TARGET_EXEEXT): CPPFLAGS += -DDO_PRINT
$(TARGET_BIN_DIR)/TestTrace$(TARGET_EXEEXT): $(TEST_TRACE_OBJS) $(TEST_TRACE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

FLIGHT_TABLE_SOURCES = \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Math/fixed.cpp \
	$(SRC)/Math/Angle.cpp \
	$(SRC)/Math/FastMath.cpp \
	$(SRC)/Replay/IGCParser.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(TEST_SRC_DIR)/FlightTable.cpp
FLIGHT_TABLE_OBJS = $(call SRC_TO_OBJ,$(FLIGHT_TABLE_SOURCES))
FLIGHT_TABLE_LDADD = $(UTIL_LIBS) $(MATH_LIBS) $(IO_LIBS)
$(TARGET_BIN_DIR)/FlightTable$(TARGET_EXEEXT): $(FLIGHT_TABLE_OBJS) $(FLIGHT_TABLE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

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
	TestOLC \
	BenchmarkProjection \
	DumpTextFile DumpTextZip WriteTextFile RunTextWriter \
	RunXMLParser \
	ReadMO \
	ReadProfileString ReadProfileInt \
	WriteProfileString WriteProfileInt \
	ReadGRecord VerifyGRecord AppendGRecord \
	AddChecksum \
	KeyCodeDumper \
	LoadTopography LoadTerrain \
	RunHeightMatrix \
	RunInputParser \
	RunWaypointParser RunAirspaceParser \
	ReadPort RunPortHandler \
	RunDeviceDriver RunDeclare RunFlightList RunDownloadFlight \
	lxn2igc \
	RunIGCWriter \
	RunWindZigZag \
	RunCanvas RunMapWindow RunDialog \
	RunRenderOZ \
	RunProgressWindow \
	RunJobDialog \
	RunAnalysis \
	RunAirspaceWarningDialog \
	TestNotify \
	DebugDisplay

ifeq ($(TARGET),UNIX)
DEBUG_PROGRAM_NAMES += FeedNMEA \
	FeedTCP
endif

ifeq ($(HAVE_NET),y)
DEBUG_PROGRAM_NAMES += RunHTTPReader RunNOAADownloader
endif

ifeq ($(HAVE_CE)$(findstring $(TARGET),ALTAIR),y)
DEBUG_PROGRAM_NAMES += TodayInstall
endif

DEBUG_PROGRAMS = $(patsubst %,$(TARGET_BIN_DIR)/%$(TARGET_EXEEXT),$(DEBUG_PROGRAM_NAMES))

BENCHMARK_PROJECTION_SOURCES = \
	$(SRC)/Projection.cpp \
	$(TEST_SRC_DIR)/BenchmarkProjection.cpp
BENCHMARK_PROJECTION_OBJS = $(call SRC_TO_OBJ,$(BENCHMARK_PROJECTION_SOURCES))
BENCHMARK_PROJECTION_LDADD = \
	$(MATH_LIBS)
$(BENCHMARK_PROJECTION_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/BenchmarkProjection$(TARGET_EXEEXT): $(BENCHMARK_PROJECTION_OBJS) $(BENCHMARK_PROJECTION_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

DUMP_TEXT_FILE_SOURCES = \
	$(TEST_SRC_DIR)/DumpTextFile.cpp
DUMP_TEXT_FILE_OBJS = $(call SRC_TO_OBJ,$(DUMP_TEXT_FILE_SOURCES))
DUMP_TEXT_FILE_LDADD = \
	$(IO_LIBS) \
	$(ZZIP_LIBS)
$(TARGET_BIN_DIR)/DumpTextFile$(TARGET_EXEEXT): $(DUMP_TEXT_FILE_OBJS) $(DUMP_TEXT_FILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

DUMP_TEXT_ZIP_SOURCES = \
	$(TEST_SRC_DIR)/DumpTextZip.cpp
DUMP_TEXT_ZIP_OBJS = $(call SRC_TO_OBJ,$(DUMP_TEXT_ZIP_SOURCES))
DUMP_TEXT_ZIP_LDADD = \
	$(IO_LIBS) \
	$(ZZIP_LIBS)
$(TARGET_BIN_DIR)/DumpTextZip$(TARGET_EXEEXT): $(DUMP_TEXT_ZIP_OBJS) $(DUMP_TEXT_ZIP_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) -o $@

DEBUG_DISPLAY_SOURCES = \
	$(SRC)/Hardware/Display.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/DebugDisplay.cpp
DEBUG_DISPLAY_OBJS = $(call SRC_TO_OBJ,$(DEBUG_DISPLAY_SOURCES))
DEBUG_DISPLAY_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/DebugDisplay$(TARGET_EXEEXT): $(DEBUG_DISPLAY_OBJS) $(DEBUG_DISPLAY_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) -o $@

WRITE_TEXT_FILE_SOURCES = \
	$(TEST_SRC_DIR)/WriteTextFile.cpp
WRITE_TEXT_FILE_OBJS = $(call SRC_TO_OBJ,$(WRITE_TEXT_FILE_SOURCES))
WRITE_TEXT_FILE_LDADD = \
	$(IO_LIBS) \
	$(ZZIP_LIBS)
$(TARGET_BIN_DIR)/WriteTextFile$(TARGET_EXEEXT): $(WRITE_TEXT_FILE_OBJS) $(WRITE_TEXT_FILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_TEXT_WRITER_SOURCES = \
	$(TEST_SRC_DIR)/RunTextWriter.cpp
RUN_TEXT_WRITER_OBJS = $(call SRC_TO_OBJ,$(RUN_TEXT_WRITER_SOURCES))
RUN_TEXT_WRITER_LDADD = \
	$(IO_LIBS) \
	$(ZZIP_LIBS)
$(TARGET_BIN_DIR)/RunTextWriter$(TARGET_EXEEXT): $(RUN_TEXT_WRITER_OBJS) $(RUN_TEXT_WRITER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_HTTP_READER_SOURCES = \
	$(SRC)/Version.cpp \
	$(TEST_SRC_DIR)/RunHTTPReader.cpp
RUN_HTTP_READER_OBJS = $(call SRC_TO_OBJ,$(RUN_HTTP_READER_SOURCES))
RUN_HTTP_READER_BIN = $(TARGET_BIN_DIR)/RunHTTPReader$(TARGET_EXEEXT)
RUN_HTTP_READER_LDADD = $(IO_LIBS) $(LIBNET_LIBS)
$(RUN_HTTP_READER_BIN): LDLIBS += $(LIBNET_LDLIBS)
$(RUN_HTTP_READER_BIN): CPPFLAGS += $(LIBNET_CPPFLAGS)
$(RUN_HTTP_READER_BIN): $(RUN_HTTP_READER_OBJS) $(RUN_HTTP_READER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_NOAA_DOWNLOADER_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/Weather/NOAADownloader.cpp \
	$(SRC)/Weather/NOAAStore.cpp \
	$(SRC)/Operation.cpp \
	$(SRC)/Net/ToBuffer.cpp \
	$(TEST_SRC_DIR)/ConsoleJobRunner.cpp \
	$(TEST_SRC_DIR)/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/RunNOAADownloader.cpp
RUN_NOAA_DOWNLOADER_OBJS = $(call SRC_TO_OBJ,$(RUN_NOAA_DOWNLOADER_SOURCES))
RUN_NOAA_DOWNLOADER_BIN = $(TARGET_BIN_DIR)/RunNOAADownloader$(TARGET_EXEEXT)
RUN_NOAA_DOWNLOADER_LDADD = $(IO_LIBS) $(LIBNET_LIBS)
$(RUN_NOAA_DOWNLOADER_BIN): LDLIBS += $(LIBNET_LDLIBS)
$(RUN_NOAA_DOWNLOADER_BIN): CPPFLAGS += $(LIBNET_CPPFLAGS)
$(RUN_NOAA_DOWNLOADER_BIN): $(RUN_NOAA_DOWNLOADER_OBJS) $(RUN_NOAA_DOWNLOADER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_XML_PARSER_SOURCES = \
	$(SRC)/xmlParser.cpp \
	$(TEST_SRC_DIR)/RunXMLParser.cpp
RUN_XML_PARSER_OBJS = $(call SRC_TO_OBJ,$(RUN_XML_PARSER_SOURCES))
RUN_XML_PARSER_LDADD = \
	$(IO_LIBS)
$(TARGET_BIN_DIR)/RunXMLParser$(TARGET_EXEEXT): $(RUN_XML_PARSER_OBJS) $(RUN_XML_PARSER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

READ_MO_SOURCES = \
	$(SRC)/Language/MOFile.cpp \
	$(SRC)/OS/FileMapping.cpp \
	$(TEST_SRC_DIR)/ReadMO.cpp
READ_MO_OBJS = $(call SRC_TO_OBJ,$(READ_MO_SOURCES))
READ_MO_LDADD =
$(TARGET_BIN_DIR)/ReadMO$(TARGET_EXEEXT): $(READ_MO_OBJS) $(READ_MO_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

READ_PROFILE_STRING_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/ReadProfileString.cpp
READ_PROFILE_STRING_OBJS = $(call SRC_TO_OBJ,$(READ_PROFILE_STRING_SOURCES))
READ_PROFILE_STRING_LDADD = $(PROFILE_LIBS) $(IO_LIBS) $(UTIL_LIBS)
$(TARGET_BIN_DIR)/ReadProfileString$(TARGET_EXEEXT): $(READ_PROFILE_STRING_OBJS) $(READ_PROFILE_STRING_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(PROFILE_LDLIBS) -o $@

READ_PROFILE_INT_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/ReadProfileInt.cpp
READ_PROFILE_INT_OBJS = $(call SRC_TO_OBJ,$(READ_PROFILE_INT_SOURCES))
READ_PROFILE_INT_LDADD = $(PROFILE_LIBS) $(IO_LIBS) $(UTIL_LIBS)
$(TARGET_BIN_DIR)/ReadProfileInt$(TARGET_EXEEXT): $(READ_PROFILE_INT_OBJS) $(READ_PROFILE_INT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(PROFILE_LDLIBS) -o $@

WRITE_PROFILE_STRING_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/WriteProfileString.cpp
WRITE_PROFILE_STRING_OBJS = $(call SRC_TO_OBJ,$(WRITE_PROFILE_STRING_SOURCES))
WRITE_PROFILE_STRING_LDADD = $(PROFILE_LIBS) $(IO_LIBS) $(UTIL_LIBS)
$(TARGET_BIN_DIR)/WriteProfileString$(TARGET_EXEEXT): $(WRITE_PROFILE_STRING_OBJS) $(WRITE_PROFILE_STRING_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(PROFILE_LDLIBS) -o $@

WRITE_PROFILE_INT_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/WriteProfileInt.cpp
WRITE_PROFILE_INT_OBJS = $(call SRC_TO_OBJ,$(WRITE_PROFILE_INT_SOURCES))
WRITE_PROFILE_INT_LDADD = $(PROFILE_LIBS) $(IO_LIBS) $(UTIL_LIBS)
$(TARGET_BIN_DIR)/WriteProfileInt$(TARGET_EXEEXT): $(WRITE_PROFILE_INT_OBJS) $(WRITE_PROFILE_INT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(PROFILE_LDLIBS) -o $@

READ_GRECORD_SOURCES = \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/ReadGRecord.cpp
READ_GRECORD_OBJS = $(call SRC_TO_OBJ,$(READ_GRECORD_SOURCES))
READ_GRECORD_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/ReadGRecord$(TARGET_EXEEXT): $(READ_GRECORD_OBJS) $(READ_GRECORD_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

VERIFY_GRECORD_SOURCES = \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/VerifyGRecord.cpp
VERIFY_GRECORD_OBJS = $(call SRC_TO_OBJ,$(VERIFY_GRECORD_SOURCES))
VERIFY_GRECORD_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/VerifyGRecord$(TARGET_EXEEXT): $(VERIFY_GRECORD_OBJS) $(VERIFY_GRECORD_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

APPEND_GRECORD_SOURCES = \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/AppendGRecord.cpp
APPEND_GRECORD_OBJS = $(call SRC_TO_OBJ,$(APPEND_GRECORD_SOURCES))
APPEND_GRECORD_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/AppendGRecord$(TARGET_EXEEXT): $(APPEND_GRECORD_OBJS) $(APPEND_GRECORD_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

ADD_CHECKSUM_SOURCES = \
	$(TEST_SRC_DIR)/AddChecksum.cpp
ADD_CHECKSUM_OBJS = $(call SRC_TO_OBJ,$(ADD_CHECKSUM_SOURCES))
ADD_CHECKSUM_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/AddChecksum$(TARGET_EXEEXT): $(ADD_CHECKSUM_OBJS) $(ADD_CHECKSUM_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

KEY_CODE_DUMPER_SOURCES = \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/KeyCodeDumper.cpp
KEY_CODE_DUMPER_OBJS = $(call SRC_TO_OBJ,$(KEY_CODE_DUMPER_SOURCES))
KEY_CODE_DUMPER_BIN = $(TARGET_BIN_DIR)/KeyCodeDumper$(TARGET_EXEEXT)
KEY_CODE_DUMPER_LDADD = \
	$(FAKE_LIBS) \
	$(SCREEN_LIBS) \
	$(MATH_LIBS)
$(KEY_CODE_DUMPER_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(KEY_CODE_DUMPER_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(KEY_CODE_DUMPER_BIN): $(KEY_CODE_DUMPER_OBJS) $(KEY_CODE_DUMPER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

LOAD_TOPOGRAPHY_SOURCES = \
	$(SRC)/Topography/TopographyStore.cpp \
	$(SRC)/Topography/TopographyFile.cpp \
	$(SRC)/Topography/XShape.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/WindowProjection.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Engine/Math/Earth.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/LoadTopography.cpp
LOAD_TOPOGRAPHY_OBJS = $(call SRC_TO_OBJ,$(LOAD_TOPOGRAPHY_SOURCES))
LOAD_TOPOGRAPHY_BIN = $(TARGET_BIN_DIR)/LoadTopography$(TARGET_EXEEXT)
LOAD_TOPOGRAPHY_LDADD = \
	$(UTIL_LIBS) \
	$(MATH_LIBS) \
	$(IO_LIBS) \
	$(SHAPELIB_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(LOAD_TOPOGRAPHY_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(LOAD_TOPOGRAPHY_BIN): $(LOAD_TOPOGRAPHY_OBJS) $(LOAD_TOPOGRAPHY_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) -o $@

LOAD_TERRAIN_SOURCES = \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Engine/Math/Earth.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/LoadTerrain.cpp
LOAD_TERRAIN_OBJS = $(call SRC_TO_OBJ,$(LOAD_TERRAIN_SOURCES))
LOAD_TERRAIN_BIN = $(TARGET_BIN_DIR)/LoadTerrain$(TARGET_EXEEXT)
LOAD_TERRAIN_LDADD = \
	$(MATH_LIBS) \
	$(IO_LIBS) \
	$(JASPER_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(LOAD_TERRAIN_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(LOAD_TERRAIN_BIN): $(LOAD_TERRAIN_OBJS) $(LOAD_TERRAIN_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) -o $@

RUN_HEIGHT_MATRIX_SOURCES = \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/HeightMatrix.cpp \
	$(SRC)/Geo/GeoClip.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/WindowProjection.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Engine/Math/Earth.cpp \
	$(SRC)/Operation.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(TEST_SRC_DIR)/RunHeightMatrix.cpp
RUN_HEIGHT_MATRIX_OBJS = $(call SRC_TO_OBJ,$(RUN_HEIGHT_MATRIX_SOURCES))
RUN_HEIGHT_MATRIX_BIN = $(TARGET_BIN_DIR)/RunHeightMatrix$(TARGET_EXEEXT)
RUN_HEIGHT_MATRIX_LDADD = \
	$(MATH_LIBS) \
	$(IO_LIBS) \
	$(JASPER_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(RUN_HEIGHT_MATRIX_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_HEIGHT_MATRIX_BIN): $(RUN_HEIGHT_MATRIX_OBJS) $(RUN_HEIGHT_MATRIX_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) -o $@

RUN_INPUT_PARSER_SOURCES = \
	$(SRC)/InputParser.cpp \
	$(SRC)/MenuData.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/Compatibility/string.c \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunInputParser.cpp
RUN_INPUT_PARSER_OBJS = $(call SRC_TO_OBJ,$(RUN_INPUT_PARSER_SOURCES))
RUN_INPUT_PARSER_LDADD = \
	$(IO_LIBS) \
	$(UTIL_LIBS)
$(TARGET_BIN_DIR)/RunInputParser$(TARGET_EXEEXT): $(RUN_INPUT_PARSER_OBJS) $(RUN_INPUT_PARSER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_WAY_POINT_PARSER_SOURCES = \
	$(SRC)/Geo/UTM.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointWriter.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunWaypointParser.cpp
RUN_WAY_POINT_PARSER_OBJS = $(call SRC_TO_OBJ,$(RUN_WAY_POINT_PARSER_SOURCES))
RUN_WAY_POINT_PARSER_LDADD = \
	$(FAKE_LIBS) \
	$(ENGINE_LIBS) \
	$(IO_LIBS) \
	$(ZZIP_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(TARGET_BIN_DIR)/RunWaypointParser$(TARGET_EXEEXT): $(RUN_WAY_POINT_PARSER_OBJS) $(RUN_WAY_POINT_PARSER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) -o $@

RUN_AIRSPACE_PARSER_SOURCES = \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/RunAirspaceParser.cpp
RUN_AIRSPACE_PARSER_OBJS = $(call SRC_TO_OBJ,$(RUN_AIRSPACE_PARSER_SOURCES))
RUN_AIRSPACE_PARSER_LDADD = \
	$(FAKE_LIBS) \
	$(ENGINE_LIBS) \
	$(IO_LIBS) \
	$(ZZIP_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(TARGET_BIN_DIR)/RunAirspaceParser$(TARGET_EXEEXT): $(RUN_AIRSPACE_PARSER_OBJS) $(RUN_AIRSPACE_PARSER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

READ_PORT_SOURCES = \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/StoppableThread.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Compatibility/string.c \
	$(TEST_SRC_DIR)/ReadPort.cpp
ifeq ($(HAVE_POSIX),y)
READ_PORT_SOURCES += \
	$(SRC)/Device/TTYPort.cpp
else
READ_PORT_SOURCES += \
	$(SRC)/Device/SerialPort.cpp
endif
ifeq ($(HAVE_CE),y)
READ_PORT_SOURCES += \
	$(SRC)/Device/Widcomm.cpp
endif
READ_PORT_OBJS = $(call SRC_TO_OBJ,$(READ_PORT_SOURCES))
READ_PORT_LDADD =
$(READ_PORT_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/ReadPort$(TARGET_EXEEXT): $(READ_PORT_OBJS) $(READ_PORT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_PORT_HANDLER_SOURCES = \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/StoppableThread.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Compatibility/string.c \
	$(TEST_SRC_DIR)/RunPortHandler.cpp
ifeq ($(HAVE_POSIX),y)
RUN_PORT_HANDLER_SOURCES += \
	$(SRC)/Device/TTYPort.cpp
else
RUN_PORT_HANDLER_SOURCES += \
	$(SRC)/Device/SerialPort.cpp
endif
ifeq ($(HAVE_CE),y)
RUN_PORT_HANDLER_SOURCES += \
	$(SRC)/Device/Widcomm.cpp
endif
RUN_PORT_HANDLER_OBJS = $(call SRC_TO_OBJ,$(RUN_PORT_HANDLER_SOURCES))
RUN_PORT_HANDLER_LDADD =
$(RUN_PORT_HANDLER_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/RunPortHandler$(TARGET_EXEEXT): $(RUN_PORT_HANDLER_OBJS) $(RUN_PORT_HANDLER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DEVICE_DRIVER_SOURCES = \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/NullPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/FLARM/State.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/IO/CSVLine.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/ClimbAverageCalculator.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Operation.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(ENGINE_SRC_DIR)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/RunDeviceDriver.cpp
RUN_DEVICE_DRIVER_OBJS = $(call SRC_TO_OBJ,$(RUN_DEVICE_DRIVER_SOURCES))
RUN_DEVICE_DRIVER_LDADD = \
	$(DRIVER_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(TARGET_BIN_DIR)/RunDeviceDriver$(TARGET_EXEEXT): $(RUN_DEVICE_DRIVER_OBJS) $(RUN_DEVICE_DRIVER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DECLARE_SOURCES = \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/IO/CSVLine.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/StoppableThread.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/RunDeclare.cpp
ifeq ($(HAVE_POSIX),y)
RUN_DECLARE_SOURCES += \
	$(SRC)/Device/TTYPort.cpp
else
RUN_DECLARE_SOURCES += \
	$(SRC)/Device/SerialPort.cpp
endif
ifeq ($(HAVE_CE),y)
RUN_DECLARE_SOURCES += \
	$(SRC)/Device/Widcomm.cpp
endif
RUN_DECLARE_OBJS = $(call SRC_TO_OBJ,$(RUN_DECLARE_SOURCES))
RUN_DECLARE_LDADD = \
	$(ZZIP_LIBS) \
	$(DRIVER_LIBS) \
	$(ENGINE_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(RUN_DECLARE_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/RunDeclare$(TARGET_EXEEXT): $(RUN_DECLARE_OBJS) $(RUN_DECLARE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_FLIGHT_LIST_SOURCES = \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/IO/CSVLine.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/StoppableThread.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/RunFlightList.cpp
ifeq ($(HAVE_POSIX),y)
RUN_FLIGHT_LIST_SOURCES += \
	$(SRC)/Device/TTYPort.cpp
else
RUN_FLIGHT_LIST_SOURCES += \
	$(SRC)/Device/SerialPort.cpp
endif
ifeq ($(HAVE_CE),y)
RUN_FLIGHT_LIST_SOURCES += \
	$(SRC)/Device/Widcomm.cpp
endif
RUN_FLIGHT_LIST_OBJS = $(call SRC_TO_OBJ,$(RUN_FLIGHT_LIST_SOURCES))
RUN_FLIGHT_LIST_LDADD = \
	$(ZZIP_LIBS) \
	$(DRIVER_LIBS) \
	$(ENGINE_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(RUN_FLIGHT_LIST_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/RunFlightList$(TARGET_EXEEXT): $(RUN_FLIGHT_LIST_OBJS) $(RUN_FLIGHT_LIST_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DOWNLOAD_FLIGHT_SOURCES = \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/IO/CSVLine.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/StoppableThread.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/ConsoleOperationEnvironment.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/RunDownloadFlight.cpp
ifeq ($(HAVE_POSIX),y)
RUN_DOWNLOAD_FLIGHT_SOURCES += \
	$(SRC)/Device/TTYPort.cpp
else
RUN_DOWNLOAD_FLIGHT_SOURCES += \
	$(SRC)/Device/SerialPort.cpp
endif
ifeq ($(HAVE_CE),y)
RUN_DOWNLOAD_FLIGHT_SOURCES += \
	$(SRC)/Device/Widcomm.cpp
endif
RUN_DOWNLOAD_FLIGHT_OBJS = $(call SRC_TO_OBJ,$(RUN_DOWNLOAD_FLIGHT_SOURCES))
RUN_DOWNLOAD_FLIGHT_LDADD = \
	$(ZZIP_LIBS) \
	$(DRIVER_LIBS) \
	$(ENGINE_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(RUN_DOWNLOAD_FLIGHT_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/RunDownloadFlight$(TARGET_EXEEXT): $(RUN_DOWNLOAD_FLIGHT_OBJS) $(RUN_DOWNLOAD_FLIGHT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

LXN2IGC_SOURCES = \
	$(SRC)/Device/Driver/LX/Convert.cpp \
	$(SRC)/Device/Driver/LX/LXN.cpp \
	$(TEST_SRC_DIR)/lxn2igc.cpp
LXN2IGC_OBJS = $(call SRC_TO_OBJ,$(LXN2IGC_SOURCES))
LXN2IGC_LDADD =
$(LXN2IGC_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TARGET_BIN_DIR)/lxn2igc$(TARGET_EXEEXT): $(LXN2IGC_OBJS) $(LXN2IGC_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_IGC_WRITER_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/FLARM/State.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/NullPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/IO/CSVLine.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/ClimbAverageCalculator.cpp \
	$(SRC)/Logger/IGCWriter.cpp \
	$(SRC)/Logger/LoggerFRecord.cpp \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/LoggerEPE.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/RunIGCWriter.cpp
RUN_IGC_WRITER_OBJS = $(call SRC_TO_OBJ,$(RUN_IGC_WRITER_SOURCES))
RUN_IGC_WRITER_LDADD = \
	$(IO_LIBS) \
	$(ENGINE_LIBS) \
	$(DRIVER_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(TARGET_BIN_DIR)/RunIGCWriter$(TARGET_EXEEXT): $(RUN_IGC_WRITER_OBJS) $(RUN_IGC_WRITER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_WIND_ZIG_ZAG_SOURCES = \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/NullPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/BasicComputer.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/MoreData.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/FLARM/State.cpp \
	$(SRC)/IO/CSVLine.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/ClimbAverageCalculator.cpp \
	$(SRC)/Wind/WindZigZag.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeGeoid.cpp \
	$(TEST_SRC_DIR)/RunWindZigZag.cpp
RUN_WIND_ZIG_ZAG_OBJS = $(call SRC_TO_OBJ,$(RUN_WIND_ZIG_ZAG_SOURCES))
RUN_WIND_ZIG_ZAG_LDADD = \
	$(ENGINE_LIBS) \
	$(DRIVER_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(TARGET_BIN_DIR)/RunWindZigZag$(TARGET_EXEEXT): $(RUN_WIND_ZIG_ZAG_OBJS) $(RUN_WIND_ZIG_ZAG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_CANVAS_SOURCES = \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/RunCanvas.cpp
RUN_CANVAS_OBJS = $(call SRC_TO_OBJ,$(RUN_CANVAS_SOURCES))
RUN_CANVAS_BIN = $(TARGET_BIN_DIR)/RunCanvas$(TARGET_EXEEXT)
RUN_CANVAS_LDADD = \
	$(FAKE_LIBS) \
	$(SCREEN_LIBS) \
	$(MATH_LIBS)
$(RUN_CANVAS_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_CANVAS_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_CANVAS_BIN): $(RUN_CANVAS_OBJS) $(RUN_CANVAS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_MAP_WINDOW_SOURCES = \
	$(IO_SRC_DIR)/DataFile.cpp \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(SRC)/DateTime.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/MoreData.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/FLARM/State.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Airspace/AirspaceVisibility.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(SRC)/Airspace/AirspaceRendererSettings.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/WindowProjection.cpp \
	$(SRC)/CompareProjection.cpp \
	$(SRC)/Geo/GeoClip.cpp \
	$(SRC)/BackgroundDrawHelper.cpp \
	$(SRC)/MapWindow/MapWindow.cpp \
	$(SRC)/MapWindow/AirspaceRenderer.cpp \
	$(SRC)/MapWindow/MapWindowBlackboard.cpp \
	$(SRC)/MapWindow/MapWindowEvents.cpp \
	$(SRC)/MapWindow/MapWindowGlideRange.cpp \
	$(SRC)/MapWindow/MapWindowLabels.cpp \
	$(SRC)/MapWindow/MapWindowProjection.cpp \
	$(SRC)/MapWindow/MapWindowRender.cpp \
	$(SRC)/MapWindow/MapWindowSymbols.cpp \
	$(SRC)/MapWindow/MapWindowTask.cpp \
	$(SRC)/MapWindow/MapWindowThermal.cpp \
	$(SRC)/MapWindow/MapWindowTimer.cpp \
	$(SRC)/MapWindow/MapWindowTraffic.cpp \
	$(SRC)/MapWindow/MapWindowTrail.cpp \
	$(SRC)/MapWindow/MapWindowWaypoints.cpp \
	$(SRC)/MapWindow/MapCanvas.cpp \
	$(SRC)/MapWindow/MapDrawHelper.cpp \
	$(SRC)/Renderer/RenderObservationZone.cpp \
	$(SRC)/Renderer/RenderTask.cpp \
	$(SRC)/Renderer/RenderTaskPoint.cpp \
	$(SRC)/Renderer/AircraftRenderer.cpp \
	$(SRC)/Marks.cpp \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/RasterTerrain.cpp \
	$(SRC)/Terrain/RasterWeather.cpp \
	$(SRC)/Terrain/HeightMatrix.cpp \
	$(SRC)/Terrain/RasterRenderer.cpp \
	$(SRC)/Terrain/TerrainRenderer.cpp \
	$(SRC)/Terrain/TerrainSettings.cpp \
	$(SRC)/Terrain/WeatherTerrainRenderer.cpp \
	$(SRC)/Screen/LabelBlock.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/Graphics.cpp \
	$(SRC)/Screen/TextInBox.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/Look/WaypointLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Look/AircraftLook.cpp \
	$(SRC)/Look/TrafficLook.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/SettingsMapBlackboard.cpp \
	$(SRC)/SettingsComputer.cpp \
	$(SRC)/SettingsComputerBlackboard.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Task/RoutePlannerGlue.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/Topography/TopographyFile.cpp \
	$(SRC)/Topography/TopographyStore.cpp \
	$(SRC)/Topography/TopographyRenderer.cpp \
	$(SRC)/Topography/TopographyGlue.cpp \
	$(SRC)/Topography/XShape.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/UnitsFormatter.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Profile/Earth.cpp \
	$(SRC)/Geo/UTM.cpp \
	$(SRC)/Waypoint/WaypointGlue.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointIconRenderer.cpp \
	$(SRC)/Waypoint/WaypointRenderer.cpp \
	$(SRC)/Waypoint/WaypointWriter.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfileGlue.cpp \
	$(TEST_SRC_DIR)/RunMapWindow.cpp
RUN_MAP_WINDOW_OBJS = $(call SRC_TO_OBJ,$(RUN_MAP_WINDOW_SOURCES))
RUN_MAP_WINDOW_BIN = $(TARGET_BIN_DIR)/RunMapWindow$(TARGET_EXEEXT)
RUN_MAP_WINDOW_LDADD = \
	$(FAKE_LIBS) \
	$(PROFILE_LIBS) \
	$(SCREEN_LIBS) \
	$(SHAPELIB_LIBS) \
	$(ENGINE_LIBS) \
	$(JASPER_LIBS) \
	$(IO_LIBS) \
	$(ZZIP_LIBS) \
	$(UTIL_LIBS) \
	$(MATH_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_MAP_WINDOW_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_MAP_WINDOW_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_MAP_WINDOW_BIN): $(RUN_MAP_WINDOW_OBJS) $(RUN_MAP_WINDOW_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(PROFILE_LDLIBS) $(ZZIP_LDFLAGS) -o $@

RUN_DIALOG_SOURCES = \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/ListPicker.cpp \
	$(SRC)/Dialogs/ComboPicker.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/Clock.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunDialog.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Compatibility/fmode.c
RUN_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_DIALOG_SOURCES))
RUN_DIALOG_BIN = $(TARGET_BIN_DIR)/RunDialog$(TARGET_EXEEXT)
RUN_DIALOG_LDADD = \
	$(RESOURCE_BINARY) \
	$(FAKE_LIBS) \
	$(IO_LIBS) \
	$(DATA_FIELD_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(MATH_LIBS) \
	$(ZZIP_LIBS)
$(RUN_DIALOG_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_DIALOG_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_DIALOG_BIN): $(RUN_DIALOG_OBJS) $(RUN_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_RENDER_OZ_SOURCES = \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/Renderer/RenderObservationZone.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Airspace/AirspaceRendererSettings.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/RunRenderOZ.cpp
RUN_RENDER_OZ_OBJS = $(call SRC_TO_OBJ,$(RUN_RENDER_OZ_SOURCES))
RUN_RENDER_OZ_BIN = $(TARGET_BIN_DIR)/RunRenderOZ$(TARGET_EXEEXT)
RUN_RENDER_OZ_LDADD = \
	$(ENGINE_CORE_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(MATH_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_RENDER_OZ_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_RENDER_OZ_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_RENDER_OZ_BIN): $(RUN_RENDER_OZ_OBJS) $(RUN_RENDER_OZ_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_PROGRESS_WINDOW_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/Screen/ProgressWindow.cpp \
	$(SRC)/Screen/ProgressBar.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/RunProgressWindow.cpp
RUN_PROGRESS_WINDOW_OBJS = $(call SRC_TO_OBJ,$(RUN_PROGRESS_WINDOW_SOURCES))
RUN_PROGRESS_WINDOW_BIN = $(TARGET_BIN_DIR)/RunProgressWindow$(TARGET_EXEEXT)
RUN_PROGRESS_WINDOW_LDADD = \
	$(SCREEN_LIBS) \
	$(MATH_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_PROGRESS_WINDOW_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_PROGRESS_WINDOW_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_PROGRESS_WINDOW_BIN): $(RUN_PROGRESS_WINDOW_OBJS) $(RUN_PROGRESS_WINDOW_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_JOB_DIALOG_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Operation.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/Operation.cpp \
	$(SRC)/Thread/JobThread.cpp \
	$(SRC)/Screen/ProgressWindow.cpp \
	$(SRC)/Screen/ProgressBar.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Util/StringUtil.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Dialogs/JobDialog.cpp \
	$(SRC)/Form/Form.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/RunJobDialog.cpp
RUN_JOB_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_JOB_DIALOG_SOURCES))
RUN_JOB_DIALOG_BIN = $(TARGET_BIN_DIR)/RunJobDialog$(TARGET_EXEEXT)
RUN_JOB_DIALOG_LDADD = \
	$(SCREEN_LIBS) \
	$(MATH_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_JOB_DIALOG_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_JOB_DIALOG_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_JOB_DIALOG_BIN): $(RUN_JOB_DIALOG_OBJS) $(RUN_JOB_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_ANALYSIS_SOURCES = \
	$(SRC)/DateTime.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/MoreData.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/Derived.cpp \
	$(SRC)/NMEA/VarioInfo.cpp \
	$(SRC)/NMEA/ClimbInfo.cpp \
	$(SRC)/NMEA/CirclingInfo.cpp \
	$(SRC)/NMEA/ThermalBand.cpp \
	$(SRC)/NMEA/ThermalLocator.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/NMEA/ClimbHistory.cpp \
	$(SRC)/FLARM/State.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/GestureManager.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Task/RoutePlannerGlue.cpp \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Atmosphere/CuSonde.cpp \
	$(SRC)/Wind/WindAnalyser.cpp \
	$(SRC)/Wind/WindStore.cpp \
	$(SRC)/Wind/WindMeasurementList.cpp \
	$(SRC)/Wind/WindZigZag.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/WindowProjection.cpp \
	$(SRC)/MapWindow/MapWindowProjection.cpp \
	$(SRC)/ChartProjection.cpp \
	$(SRC)/Renderer/RenderTask.cpp \
	$(SRC)/Renderer/RenderTaskPoint.cpp \
	$(SRC)/Renderer/RenderObservationZone.cpp \
	$(SRC)/Renderer/AircraftRenderer.cpp \
	$(SRC)/Geo/GeoClip.cpp \
	$(SRC)/MapWindow/MapCanvas.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/UnitsFormatter.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/Chart.cpp \
	$(SRC)/Screen/Graphics.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/Util.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/Look/Look.cpp \
	$(SRC)/Look/VarioLook.cpp \
	$(SRC)/Look/ChartLook.cpp \
	$(SRC)/Look/ThermalBandLook.cpp \
	$(SRC)/Look/TraceHistoryLook.cpp \
	$(SRC)/Look/CrossSectionLook.cpp \
	$(SRC)/Look/WaypointLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Look/AircraftLook.cpp \
	$(SRC)/Look/TrafficLook.cpp \
	$(SRC)/Look/InfoBoxLook.cpp \
	$(SRC)/Gauge/FlarmTrafficLook.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Profile/FontConfig.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/RasterTerrain.cpp \
	$(SRC)/Terrain/TerrainSettings.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/dlgAnalysis.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/Dialogs/ComboPicker.cpp \
	$(SRC)/Dialogs/ListPicker.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/CrossSection/CrossSectionWindow.cpp \
	$(SRC)/FlightStatistics.cpp \
	$(SRC)/FlightStatisticsRenderer.cpp \
	$(SRC)/ThermalBandRenderer.cpp \
	$(SRC)/GlideRatio.cpp \
	$(SRC)/AutoQNH.cpp \
	$(SRC)/BasicComputer.cpp \
	$(SRC)/GlideComputer.cpp \
	$(SRC)/GlideComputerBlackboard.cpp \
	$(SRC)/GlideComputerTask.cpp \
	$(SRC)/GlideComputerInterface.cpp \
	$(SRC)/GlideComputerAirData.cpp \
	$(SRC)/GlideComputerStats.cpp \
	$(SRC)/SettingsComputer.cpp \
	$(SRC)/Replay/IGCParser.cpp \
	$(SRC)/SettingsComputerBlackboard.cpp \
	$(SRC)/SettingsMapBlackboard.cpp \
	$(SRC)/InterfaceBlackboard.cpp \
	$(SRC)/Audio/VegaVoice.cpp \
	$(SRC)/TeamCodeCalculation.cpp \
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Airspace/AirspaceGlue.cpp \
	$(SRC)/Airspace/AirspaceVisibility.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(SRC)/Airspace/AirspaceRendererSettings.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/IO/ConfiguredFile.cpp \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunAnalysis.cpp
RUN_ANALYSIS_OBJS = $(call SRC_TO_OBJ,$(RUN_ANALYSIS_SOURCES))
RUN_ANALYSIS_BIN = $(TARGET_BIN_DIR)/RunAnalysis$(TARGET_EXEEXT)
RUN_ANALYSIS_LDADD = \
	$(PROFILE_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(DATA_FIELD_LIBS) \
	$(ENGINE_LIBS) \
	$(JASPER_LIBS) \
	$(IO_LIBS) \
	$(ZZIP_LIBS) \
	$(UTIL_LIBS) \
	$(MATH_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_ANALYSIS_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_ANALYSIS_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_ANALYSIS_BIN): $(RUN_ANALYSIS_OBJS) $(RUN_ANALYSIS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(PROFILE_LDLIBS) $(ZZIP_LDFLAGS) -o $@

RUN_AIRSPACE_WARNING_DIALOG_SOURCES = \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/ListPicker.cpp \
	$(SRC)/Dialogs/ComboPicker.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/Dialogs/dlgAirspaceWarnings.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Audio/Sound.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/CustomFonts.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Operation.cpp \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProfileGlue.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunAirspaceWarningDialog.cpp
RUN_AIRSPACE_WARNING_DIALOG_BIN = $(TARGET_BIN_DIR)/RunAirspaceWarningDialog$(TARGET_EXEEXT)
RUN_AIRSPACE_WARNING_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_AIRSPACE_WARNING_DIALOG_SOURCES))
RUN_AIRSPACE_WARNING_DIALOG_LDADD = \
	$(FAKE_LIBS) \
	$(DATA_FIELD_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(ENGINE_LIBS) \
	$(IO_LIBS) \
	$(ZZIP_LIBS) \
	$(UTIL_LIBS) \
	$(MATH_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_AIRSPACE_WARNING_DIALOG_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_AIRSPACE_WARNING_DIALOG_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_AIRSPACE_WARNING_DIALOG_BIN): $(RUN_AIRSPACE_WARNING_DIALOG_OBJS) $(RUN_AIRSPACE_WARNING_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(ZZIP_LDFLAGS) -o $@

RUN_TASK_EDITOR_DIALOG_SOURCES = \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/ComboPicker.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/Dialogs/dlgTaskOverview.cpp \
	$(SRC)/Dialogs/dlgWaypointSelect.cpp \
	$(SRC)/Dialogs/dlgWaypointDetails.cpp \
	$(SRC)/Dialogs/dlgTaskWaypoint.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	$(SRC)/LocalTime.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/UnitsFormatter.cpp \
	$(SRC)/Geo/UTM.cpp \
	$(SRC)/Waypoint/WaypointGlue.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Operation.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunTaskEditorDialog.cpp
RUN_TASK_EDITOR_DIALOG_BIN = $(TARGET_BIN_DIR)/RunTaskEditorDialog$(TARGET_EXEEXT)
RUN_TASK_EDITOR_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_TASK_EDITOR_DIALOG_SOURCES))
RUN_TASK_EDITOR_DIALOG_LDADD = \
	$(FAKE_LIBS) \
	$(DATA_FIELD_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(ENGINE_LIBS) \
	$(IO_LIBS) \
	$(ZZIP_LIBS) \
	$(UTIL_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_TASK_EDITOR_DIALOG_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_TASK_EDITOR_DIALOG_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_TASK_EDITOR_DIALOG_BIN): $(RUN_TASK_EDITOR_DIALOG_OBJS) $(RUN_TASK_EDITOR_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_NOTIFY_SOURCES = \
	$(SRC)/OS/Clock.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/Notify.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeBlank.cpp \
	$(TEST_SRC_DIR)/TestNotify.cpp
TEST_NOTIFY_OBJS = $(call SRC_TO_OBJ,$(TEST_NOTIFY_SOURCES))
TEST_NOTIFY_BIN = $(TARGET_BIN_DIR)/TestNotify$(TARGET_EXEEXT)
TEST_NOTIFY_LDADD = \
	$(SCREEN_LIBS) \
	$(TESTLIBS1)
$(TEST_NOTIFY_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(TEST_NOTIFY_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(TEST_NOTIFY_BIN): $(TEST_NOTIFY_OBJS) $(TEST_NOTIFY_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(PROFILE_LDLIBS) $(ZZIP_LDFLAGS) -o $@

FEED_NMEA_SOURCES = \
	$(TEST_SRC_DIR)/FeedNMEA.cpp
FEED_NMEA_BIN = $(TARGET_BIN_DIR)/FeedNMEA$(TARGET_EXEEXT)
FEED_NMEA_OBJS = $(call SRC_TO_OBJ,$(FEED_NMEA_SOURCES))
FEED_NMEA_LDADD =
$(FEED_NMEA_BIN): $(FEED_NMEA_OBJS) $(FEED_NMEA_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

FEED_TCP_SOURCES = \
	$(TEST_SRC_DIR)/FeedTCP.cpp
FEED_TCP_BIN = $(TARGET_BIN_DIR)/FeedTCP$(TARGET_EXEEXT)
FEED_TCP_OBJS = $(call SRC_TO_OBJ,$(FEED_TCP_SOURCES))
FEED_TCP_LDADD =
$(FEED_TCP_BIN): $(FEED_TCP_OBJS) $(FEED_TCP_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TODAY_INSTALL_SOURCES = \
	$(TEST_SRC_DIR)/TodayInstall.cpp
TODAY_INSTALL_BIN = $(TARGET_BIN_DIR)/TodayInstall$(TARGET_EXEEXT)
TODAY_INSTALL_OBJS = $(call SRC_TO_OBJ,$(TODAY_INSTALL_SOURCES))
TODAY_INSTALL_LDADD =
$(TODAY_INSTALL_BIN): $(TODAY_INSTALL_OBJS) $(TODAY_INSTALL_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(LINK) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@


debug-: $(addprefix call-debug-,$(DEFAULT_TARGETS))
call-debug-%:
	$(MAKE) debug TARGET=$(patsubst call-debug-%,%,$@) DEBUG=$(DEBUG) V=$(V)

$(addprefix debug-,$(TARGETS)): debug-%: $(DEBUG_PROGRAMS)

debug: debug-$(TARGET)
