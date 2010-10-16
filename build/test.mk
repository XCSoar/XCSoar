TESTFAST = \
	$(TARGET_BIN_DIR)/test_olc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_fixed$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_waypoints$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_pressure$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_task$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_mc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_modes$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_automc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_trees$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_acfilter$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_airspace$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_replay$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_highterrain$(TARGET_EXEEXT)

ifeq ($(HAVE_WIN32),y)
TESTFAST += $(TARGET_BIN_DIR)/test_win32$(TARGET_EXEEXT)
endif

TESTSLOW = \
	$(TARGET_BIN_DIR)/test_effectivemc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_randomtask$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_vopt$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_cruiseefficiency$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_bestcruisetrack$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_aat$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_flight$(TARGET_EXEEXT)

TESTS = $(TESTFAST) $(TESTSLOW)

testslow: $(TESTSLOW)
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(TESTSLOW)

testfast: $(TESTFAST)
	$(Q)perl $(TEST_SRC_DIR)/testall.pl $(TESTFAST)

TESTLIBS = $(HARNESS_LIBS) \
	   $(ZZIP_LIBS) \
	$(ENGINE_CORE_LIBS) \
	$(IO_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)

CPPFLAGS += -DHAVE_TAP

$(TESTS): CPPFLAGS += $(TEST_CPPFLAGS)
$(TESTS): $(TARGET_BIN_DIR)/%$(TARGET_EXEEXT): $(TARGET_OUTPUT_DIR)/test/src/%.o $(TESTLIBS) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_NAMES = \
	test_fixed \
	test_waypoints \
	test_pressure \
	test_task \
	TestAngle TestUnits TestEarth TestSunEphemeris \
	TestRadixTree \
	TestLogger TestDriver \
	TestWayPointFile

TESTS = $(patsubst %,$(TARGET_BIN_DIR)/%$(TARGET_EXEEXT),$(TEST_NAMES))

TEST_ANGLE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestAngle.cpp
TEST_ANGLE_OBJS = $(call SRC_TO_OBJ,$(TEST_ANGLE_SOURCES))
TEST_ANGLE_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestAngle$(TARGET_EXEEXT): $(TEST_ANGLE_OBJS) $(TEST_ANGLE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_UNITS_SOURCES = \
	$(SRC)/Units.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestUnits.cpp
TEST_UNITS_OBJS = $(call SRC_TO_OBJ,$(TEST_UNITS_SOURCES))
TEST_UNITS_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestUnits$(TARGET_EXEEXT): $(TEST_UNITS_OBJS) $(TEST_UNITS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_EARTH_SOURCES = \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestEarth.cpp
TEST_EARTH_OBJS = $(call SRC_TO_OBJ,$(TEST_EARTH_SOURCES))
TEST_EARTH_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestEarth$(TARGET_EXEEXT): $(TEST_EARTH_OBJS) $(TEST_EARTH_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_SUN_EPHEMERIS_SOURCES = \
	$(SRC)/Math/SunEphemeris.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestSunEphemeris.cpp
TEST_SUN_EPHEMERIS_OBJS = $(call SRC_TO_OBJ,$(TEST_SUN_EPHEMERIS_SOURCES))
TEST_SUN_EPHEMERIS_LDADD = $(MATH_LIBS)
$(TARGET_BIN_DIR)/TestSunEphemeris$(TARGET_EXEEXT): $(TEST_SUN_EPHEMERIS_OBJS) $(TEST_SUN_EPHEMERIS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_RADIX_TREE_SOURCES = \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestRadixTree.cpp
TEST_RADIX_TREE_OBJS = $(call SRC_TO_OBJ,$(TEST_RADIX_TREE_SOURCES))
$(TARGET_BIN_DIR)/TestRadixTree$(TARGET_EXEEXT): $(TEST_RADIX_TREE_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

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
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_DRIVER_SOURCES = \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/NullPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Driver/CAI302.cpp \
	$(SRC)/Device/Driver/LX.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/Math/fixed.cpp \
	$(SRC)/Math/Angle.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(ENGINE_SRC_DIR)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestDriver.cpp
TEST_DRIVER_OBJS = $(call SRC_TO_OBJ,$(TEST_DRIVER_SOURCES))
$(TARGET_BIN_DIR)/TestDriver$(TARGET_EXEEXT): $(TEST_DRIVER_OBJS) $(TEST_DRIVER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_WAY_POINT_FILE_SOURCES = \
	$(SRC)/Units.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/WayPoint/WayPointFile.cpp \
	$(SRC)/WayPoint/WayPointFileWinPilot.cpp \
	$(SRC)/WayPoint/WayPointFileSeeYou.cpp \
	$(SRC)/WayPoint/WayPointFileZander.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/TaskProjection.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatGeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoint.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/WaypointEnvelope.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoints.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProgressGlue.cpp \
	$(TEST_SRC_DIR)/tap.c \
	$(TEST_SRC_DIR)/TestWayPointFile.cpp
TEST_WAY_POINT_FILE_OBJS = $(call SRC_TO_OBJ,$(TEST_WAY_POINT_FILE_SOURCES))
TEST_WAY_POINT_FILE_LDADD = $(UTIL_LIBS) $(MATH_LIBS) $(IO_LIBS) $(ZZIP_LIBS)
$(TARGET_BIN_DIR)/TestWayPointFile$(TARGET_EXEEXT): $(TEST_WAY_POINT_FILE_OBJS) $(TEST_WAY_POINT_FILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

TEST_OLC_SOURCES = \
	$(SRC)/Math/fixed.cpp \
	$(SRC)/Math/Angle.cpp \
	$(SRC)/Math/FastMath.cpp \
	$(SRC)/Replay/IgcReplay.cpp \
	$(TEST_SRC_DIR)/TestOLC.cpp 
TEST_OLC_OBJS = $(call SRC_TO_OBJ,$(TEST_OLC_SOURCES))
TEST_OLC_LDADD = $(UTIL_LIBS) $(MATH_LIBS) $(IO_LIBS) $(ENGINE_LIBS)
$(TARGET_BIN_DIR)/TestOLC$(TARGET_EXEEXT): $(TEST_OLC_OBJS) $(TEST_OLC_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

build-check: $(TESTS)

check: $(TESTS) | $(OUT)/test/dirstamp
	@$(NQ)echo "  TEST    $(notdir $(patsubst %$(TARGET_EXEEXT),%,$^))"
	$(Q)$(PERL) $(TEST_SRC_DIR)/testall.pl $(TESTS)

DEBUG_PROGRAM_NAMES = \
	TestOLC \
	DumpTextFile DumpTextZip WriteTextFile RunTextWriter \
	ReadMO \
	ReadProfileString ReadProfileInt \
	WriteProfileString WriteProfileInt \
	ReadGRecord VerifyGRecord AppendGRecord \
	KeyCodeDumper \
	LoadTopology \
	RunWayPointParser RunDeviceDriver \
	RunCanvas RunMapWindow RunDialog \
	RunAirspaceWarningDialog

ifeq ($(TARGET),UNIX)
DEBUG_PROGRAM_NAMES += FeedNMEA
endif

DEBUG_PROGRAMS = $(patsubst %,$(TARGET_BIN_DIR)/%$(TARGET_EXEEXT),$(DEBUG_PROGRAM_NAMES))

DUMP_TEXT_FILE_SOURCES = \
	$(TEST_SRC_DIR)/DumpTextFile.cpp
DUMP_TEXT_FILE_OBJS = $(call SRC_TO_OBJ,$(DUMP_TEXT_FILE_SOURCES))
DUMP_TEXT_FILE_LDADD = \
	$(IO_LIBS) \
	$(ZZIP_LIBS)
$(TARGET_BIN_DIR)/DumpTextFile$(TARGET_EXEEXT): $(DUMP_TEXT_FILE_OBJS) $(DUMP_TEXT_FILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

DUMP_TEXT_ZIP_SOURCES = \
	$(TEST_SRC_DIR)/DumpTextZip.cpp
DUMP_TEXT_ZIP_OBJS = $(call SRC_TO_OBJ,$(DUMP_TEXT_ZIP_SOURCES))
DUMP_TEXT_ZIP_LDADD = \
	$(IO_LIBS) \
	$(ZZIP_LIBS)
$(TARGET_BIN_DIR)/DumpTextZip$(TARGET_EXEEXT): $(DUMP_TEXT_ZIP_OBJS) $(DUMP_TEXT_ZIP_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

WRITE_TEXT_FILE_SOURCES = \
	$(TEST_SRC_DIR)/WriteTextFile.cpp
WRITE_TEXT_FILE_OBJS = $(call SRC_TO_OBJ,$(WRITE_TEXT_FILE_SOURCES))
WRITE_TEXT_FILE_LDADD = \
	$(IO_LIBS) \
	$(ZZIP_LIBS)
$(TARGET_BIN_DIR)/WriteTextFile$(TARGET_EXEEXT): $(WRITE_TEXT_FILE_OBJS) $(WRITE_TEXT_FILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_TEXT_WRITER_SOURCES = \
	$(TEST_SRC_DIR)/RunTextWriter.cpp
RUN_TEXT_WRITER_OBJS = $(call SRC_TO_OBJ,$(RUN_TEXT_WRITER_SOURCES))
RUN_TEXT_WRITER_LDADD = \
	$(IO_LIBS) \
	$(ZZIP_LIBS)
$(TARGET_BIN_DIR)/RunTextWriter$(TARGET_EXEEXT): $(RUN_TEXT_WRITER_OBJS) $(RUN_TEXT_WRITER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

READ_MO_SOURCES = \
	$(SRC)/MOFile.cpp \
	$(SRC)/OS/FileMapping.cpp \
	$(TEST_SRC_DIR)/ReadMO.cpp
READ_MO_OBJS = $(call SRC_TO_OBJ,$(READ_MO_SOURCES))
READ_MO_LDADD =
$(TARGET_BIN_DIR)/ReadMO$(TARGET_EXEEXT): $(READ_MO_OBJS) $(READ_MO_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

READ_PROFILE_STRING_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/Writer.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/ReadProfileString.cpp
READ_PROFILE_STRING_OBJS = $(call SRC_TO_OBJ,$(READ_PROFILE_STRING_SOURCES))
READ_PROFILE_STRING_LDADD = $(PROFILE_LIBS) $(IO_LIBS) $(UTIL_LIBS)
$(TARGET_BIN_DIR)/ReadProfileString$(TARGET_EXEEXT): $(READ_PROFILE_STRING_OBJS) $(READ_PROFILE_STRING_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

READ_PROFILE_INT_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/Writer.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/ReadProfileInt.cpp
READ_PROFILE_INT_OBJS = $(call SRC_TO_OBJ,$(READ_PROFILE_INT_SOURCES))
READ_PROFILE_INT_LDADD = $(PROFILE_LIBS) $(IO_LIBS) $(UTIL_LIBS)
$(TARGET_BIN_DIR)/ReadProfileInt$(TARGET_EXEEXT): $(READ_PROFILE_INT_OBJS) $(READ_PROFILE_INT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

WRITE_PROFILE_STRING_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/Writer.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/WriteProfileString.cpp
WRITE_PROFILE_STRING_OBJS = $(call SRC_TO_OBJ,$(WRITE_PROFILE_STRING_SOURCES))
WRITE_PROFILE_STRING_LDADD = $(PROFILE_LIBS) $(IO_LIBS) $(UTIL_LIBS)
$(TARGET_BIN_DIR)/WriteProfileString$(TARGET_EXEEXT): $(WRITE_PROFILE_STRING_OBJS) $(WRITE_PROFILE_STRING_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

WRITE_PROFILE_INT_SOURCES = \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/Writer.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/WriteProfileInt.cpp
WRITE_PROFILE_INT_OBJS = $(call SRC_TO_OBJ,$(WRITE_PROFILE_INT_SOURCES))
WRITE_PROFILE_INT_LDADD = $(PROFILE_LIBS) $(IO_LIBS) $(UTIL_LIBS)
$(TARGET_BIN_DIR)/WriteProfileInt$(TARGET_EXEEXT): $(WRITE_PROFILE_INT_OBJS) $(WRITE_PROFILE_INT_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

READ_GRECORD_SOURCES = \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/ReadGRecord.cpp
READ_GRECORD_OBJS = $(call SRC_TO_OBJ,$(READ_GRECORD_SOURCES))
READ_GRECORD_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/ReadGRecord$(TARGET_EXEEXT): $(READ_GRECORD_OBJS) $(READ_GRECORD_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

VERIFY_GRECORD_SOURCES = \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/VerifyGRecord.cpp
VERIFY_GRECORD_OBJS = $(call SRC_TO_OBJ,$(VERIFY_GRECORD_SOURCES))
VERIFY_GRECORD_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/VerifyGRecord$(TARGET_EXEEXT): $(VERIFY_GRECORD_OBJS) $(VERIFY_GRECORD_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

APPEND_GRECORD_SOURCES = \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(TEST_SRC_DIR)/AppendGRecord.cpp
APPEND_GRECORD_OBJS = $(call SRC_TO_OBJ,$(APPEND_GRECORD_SOURCES))
APPEND_GRECORD_LDADD = $(IO_LIBS)
$(TARGET_BIN_DIR)/AppendGRecord$(TARGET_EXEEXT): $(APPEND_GRECORD_OBJS) $(APPEND_GRECORD_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

KEY_CODE_DUMPER_SOURCES = \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
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
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

LOAD_TOPOLOGY_SOURCES = \
	$(SRC)/Topology/TopologyStore.cpp \
	$(SRC)/Topology/TopologyFile.cpp \
	$(SRC)/Topology/XShape.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/LabelBlock.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Engine/Math/Earth.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/LoadTopology.cpp
LOAD_TOPOLOGY_OBJS = $(call SRC_TO_OBJ,$(LOAD_TOPOLOGY_SOURCES))
LOAD_TOPOLOGY_BIN = $(TARGET_BIN_DIR)/LoadTopology$(TARGET_EXEEXT)
LOAD_TOPOLOGY_LDADD = \
	$(MATH_LIBS) \
	$(IO_LIBS) \
	$(SCREEN_LIBS) \
	$(SHAPELIB_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(LOAD_TOPOLOGY_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(LOAD_TOPOLOGY_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(LOAD_TOPOLOGY_BIN): $(LOAD_TOPOLOGY_OBJS) $(LOAD_TOPOLOGY_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_WAY_POINT_PARSER_SOURCES = \
	$(SRC)/WayPoint/WayPointFile.cpp \
	$(SRC)/WayPoint/WayPointFileWinPilot.cpp \
	$(SRC)/WayPoint/WayPointFileSeeYou.cpp \
	$(SRC)/WayPoint/WayPointFileZander.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/FakeProgressGlue.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(TEST_SRC_DIR)/RunWayPointParser.cpp
RUN_WAY_POINT_PARSER_OBJS = $(call SRC_TO_OBJ,$(RUN_WAY_POINT_PARSER_SOURCES))
RUN_WAY_POINT_PARSER_LDADD = \
	$(FAKE_LIBS) \
	$(ENGINE_LIBS) \
	$(IO_LIBS) \
	$(ZZIP_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(TARGET_BIN_DIR)/RunWayPointParser$(TARGET_EXEEXT): $(RUN_WAY_POINT_PARSER_OBJS) $(RUN_WAY_POINT_PARSER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DEVICE_DRIVER_SOURCES = \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/NullPort.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Descriptor.cpp \
	$(SRC)/Device/FLARM.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/ClimbAverageCalculator.cpp \
	$(SRC)/Compatibility/string.c \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeNMEALogger.cpp \
	$(TEST_SRC_DIR)/FakeProgressGlue.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/RunDeviceDriver.cpp
RUN_DEVICE_DRIVER_OBJS = $(call SRC_TO_OBJ,$(RUN_DEVICE_DRIVER_SOURCES))
RUN_DEVICE_DRIVER_LDADD = \
	$(ZZIP_LIBS) \
	$(ENGINE_LIBS) \
	$(DRIVER_LIBS) \
	$(MATH_LIBS) \
	$(UTIL_LIBS)
$(TARGET_BIN_DIR)/RunDeviceDriver$(TARGET_EXEEXT): $(RUN_DEVICE_DRIVER_OBJS) $(RUN_DEVICE_DRIVER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_CANVAS_SOURCES = \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
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
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_MAP_WINDOW_SOURCES = \
	$(IO_SRC_DIR)/DataFile.cpp \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Airspace/AirspaceVisibility.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/RenderObservationZone.cpp \
	$(SRC)/BackgroundDrawHelper.cpp \
	$(SRC)/MapWindow.cpp \
	$(SRC)/MapWindowAirspace.cpp \
	$(SRC)/MapWindowBlackboard.cpp \
	$(SRC)/MapWindowEvents.cpp \
	$(SRC)/MapWindowGlideRange.cpp \
	$(SRC)/MapWindowLabels.cpp \
	$(SRC)/MapWindowProjection.cpp \
	$(SRC)/MapWindowRender.cpp \
	$(SRC)/MapWindowScale.cpp \
	$(SRC)/MapWindowSymbols.cpp \
	$(SRC)/MapWindowTask.cpp \
	$(SRC)/MapWindowThermal.cpp \
	$(SRC)/MapWindowTimer.cpp \
	$(SRC)/MapWindowTraffic.cpp \
	$(SRC)/MapWindowTrail.cpp \
	$(SRC)/MapWindowWaypoints.cpp \
	$(SRC)/MapCanvas.cpp \
	$(SRC)/MapDrawHelper.cpp \
	$(SRC)/RenderTask.cpp \
	$(SRC)/RenderTaskPoint.cpp \
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
	$(SRC)/Terrain/WeatherTerrainRenderer.cpp \
	$(SRC)/Profile/Writer.cpp \
	$(SRC)/Screen/LabelBlock.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/Graphics.cpp \
	$(SRC)/Screen/TextInBox.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/SettingsMapBlackboard.cpp \
	$(SRC)/SettingsComputerBlackboard.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Topology/TopologyFile.cpp \
	$(SRC)/Topology/TopologyStore.cpp \
	$(SRC)/Topology/TopologyGlue.cpp \
	$(SRC)/Topology/XShape.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/WayPoint/WayPointGlue.cpp \
	$(SRC)/WayPoint/WayPointFile.cpp \
	$(SRC)/WayPoint/WayPointFileWinPilot.cpp \
	$(SRC)/WayPoint/WayPointFileSeeYou.cpp \
	$(SRC)/WayPoint/WayPointFileZander.cpp \
	$(SRC)/WayPoint/WayPointRenderer.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Simulator.cpp \
	$(SRC)/xmlParser.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInfoBoxLayout.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfileGlue.cpp \
	$(TEST_SRC_DIR)/FakeProgressGlue.cpp \
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
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DIALOG_SOURCES = \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/ListPicker.cpp \
	$(SRC)/Dialogs/dlgComboPicker.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/OS/PathName.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/RunDialog.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Compatibility/fmode.c
RUN_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_DIALOG_SOURCES))
RUN_DIALOG_BIN = $(TARGET_BIN_DIR)/RunDialog$(TARGET_EXEEXT)
RUN_DIALOG_LDADD = \
	$(FAKE_LIBS) \
	$(DATA_FIELD_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(MATH_LIBS) \
	$(ZZIP_LIBS)
$(RUN_DIALOG_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_DIALOG_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_DIALOG_BIN): $(RUN_DIALOG_OBJS) $(RUN_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_AIRSPACE_WARNING_DIALOG_SOURCES = \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/ListPicker.cpp \
	$(SRC)/Dialogs/dlgComboPicker.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/Dialogs/dlgAirspaceWarning.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/OS/PathName.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Simulator.cpp \
	$(SRC)/Compatibility/string.c \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInfoBoxLayout.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProfileGlue.cpp \
	$(TEST_SRC_DIR)/FakeProgressGlue.cpp \
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
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_TASK_EDITOR_DIALOG_SOURCES = \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/dlgComboPicker.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/Dialogs/dlgTaskOverview.cpp \
	$(SRC)/Dialogs/dlgWayPointSelect.cpp \
	$(SRC)/Dialogs/dlgWayPointDetails.cpp \
	$(SRC)/Dialogs/dlgTaskWaypoint.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	$(SRC)/LocalTime.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/OS/FileUtil.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/WayPointGlue.cpp \
	$(SRC)/WayPointFile.cpp \
	$(SRC)/WayPointFileWinPilot.cpp \
	$(SRC)/WayPointFileSeeYou.cpp \
	$(SRC)/WayPointFileZander.cpp \
	$(SRC)/Compatibility/string.c \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProgressGlue.cpp \
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
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

FEED_NMEA_SOURCES = \
	$(TEST_SRC_DIR)/FeedNMEA.cpp
FEED_NMEA_BIN = $(TARGET_BIN_DIR)/FeedNMEA$(TARGET_EXEEXT)
FEED_NMEA_OBJS = $(call SRC_TO_OBJ,$(FEED_NMEA_SOURCES))
FEED_NMEA_LDADD =
$(FEED_NMEA_BIN): $(FEED_NMEA_OBJS) $(FEED_NMEA_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

debug-: $(addprefix call-debug-,$(DEFAULT_TARGETS))
call-debug-%:
	$(MAKE) debug TARGET=$(patsubst call-debug-%,%,$@) DEBUG=$(DEBUG) V=$(V)

$(addprefix debug-,$(TARGETS)): debug-%: $(DEBUG_PROGRAMS)

debug: debug-$(TARGET)
