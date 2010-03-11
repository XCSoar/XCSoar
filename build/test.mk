TESTFAST = \
	$(TARGET_BIN_DIR)/test_replay$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_olc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_fixed$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_waypoints$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_edittp$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_pressure$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_task$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_mc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_modes$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_automc$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_trees$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_acfilter$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test$(TARGET_EXEEXT) \
	$(TARGET_BIN_DIR)/test_airspace$(TARGET_EXEEXT) \
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
	   $(ENGINE_LIBS)

ifeq ($(HAVE_WIN32),n)
TEST_CPPFLAGS += -DDO_PRINT
TEST_CPPFLAGS += -DINSTRUMENT_TASK
CPPFLAGS += -DHAVE_TAP
endif

$(TESTS): CPPFLAGS += $(TEST_CPPFLAGS)
$(TESTS): $(TARGET_BIN_DIR)/%$(TARGET_EXEEXT): $(TARGET_OUTPUT_DIR)/test/src/%.o $(TESTLIBS) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@


BUILDTESTS=\
	$(TARGET_BIN_DIR)/01_test_tap$(TARGET_EXEEXT)

testtap: $(BUILDTESTS)
	cd test && perl tools/testall.pl t/*

# TODO generalise
$(TARGET_BIN_DIR)/01_test_tap$(TARGET_EXEEXT): $(TEST_SRC_DIR)/01_test_tap.c | $(TARGET_BIN_DIR)/dirstamp
	gcc -o $@ $<

DEBUG_PROGRAM_NAMES = \
	DumpTextFile WriteTextFile RunTextWriter \
	RunWayPointParser RunDeviceDriver \
	RunCanvas RunMapWindow RunDialog \
	RunAirspaceWarningDialog \
	RunTaskEditorDialog
DEBUG_PROGRAMS = $(patsubst %,$(TARGET_BIN_DIR)/%$(TARGET_EXEEXT),$(DEBUG_PROGRAM_NAMES))

DUMP_TEXT_FILE_SOURCES = \
	$(SRC)/TextReader.cpp \
	$(TEST_SRC_DIR)/DumpTextFile.cpp
DUMP_TEXT_FILE_OBJS = $(call SRC_TO_OBJ,$(DUMP_TEXT_FILE_SOURCES))
DUMP_TEXT_FILE_LDADD = \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TARGET_BIN_DIR)/DumpTextFile$(TARGET_EXEEXT): $(DUMP_TEXT_FILE_OBJS) $(DUMP_TEXT_FILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

WRITE_TEXT_FILE_SOURCES = \
	$(SRC)/TextWriter.cpp \
	$(TEST_SRC_DIR)/WriteTextFile.cpp
WRITE_TEXT_FILE_OBJS = $(call SRC_TO_OBJ,$(WRITE_TEXT_FILE_SOURCES))
WRITE_TEXT_FILE_LDADD = \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TARGET_BIN_DIR)/WriteTextFile$(TARGET_EXEEXT): $(WRITE_TEXT_FILE_OBJS) $(WRITE_TEXT_FILE_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_TEXT_WRITER_SOURCES = \
	$(SRC)/TextWriter.cpp \
	$(TEST_SRC_DIR)/RunTextWriter.cpp
RUN_TEXT_WRITER_OBJS = $(call SRC_TO_OBJ,$(RUN_TEXT_WRITER_SOURCES))
RUN_TEXT_WRITER_LDADD = \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TARGET_BIN_DIR)/RunTextWriter$(TARGET_EXEEXT): $(RUN_TEXT_WRITER_OBJS) $(RUN_TEXT_WRITER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_WAY_POINT_PARSER_SOURCES = \
	$(SRC)/WayPointFile.cpp \
	$(SRC)/WayPointFileWinPilot.cpp \
	$(SRC)/WayPointFileSeeYou.cpp \
	$(SRC)/WayPointFileZander.cpp \
	$(SRC)/WayPointParser.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsSystem.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/StringUtil.cpp \
	$(SRC)/Simulator.cpp \
	$(SRC)/TextReader.cpp \
	$(SRC)/TextWriter.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProgressDialog.cpp \
	$(TEST_SRC_DIR)/FakeRegistry.cpp \
	$(TEST_SRC_DIR)/RunWayPointParser.cpp
RUN_WAY_POINT_PARSER_OBJS = $(call SRC_TO_OBJ,$(RUN_WAY_POINT_PARSER_SOURCES))
RUN_WAY_POINT_PARSER_LDADD = \
	$(FAKE_LIBS) \
	$(ENGINE_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TARGET_BIN_DIR)/RunWayPointParser$(TARGET_EXEEXT): $(RUN_WAY_POINT_PARSER_OBJS) $(RUN_WAY_POINT_PARSER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DEVICE_DRIVER_SOURCES = \
	$(SRC)/UtilsText.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Internal.cpp \
	$(SRC)/Device/Descriptor.cpp \
	$(SRC)/Device/FLARM.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/FlarmCalculations.cpp \
	$(SRC)/ClimbAverageCalculator.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeMessage.cpp \
	$(TEST_SRC_DIR)/FakeProgressDialog.cpp \
	$(TEST_SRC_DIR)/FakeRegistry.cpp \
	$(TEST_SRC_DIR)/RunDeviceDriver.cpp
RUN_DEVICE_DRIVER_OBJS = $(call SRC_TO_OBJ,$(RUN_DEVICE_DRIVER_SOURCES))
RUN_DEVICE_DRIVER_LDADD = \
	$(ZZIP_LIBS) \
	$(ENGINE_LIBS) \
	$(DRIVER_LIBS) \
	$(COMPAT_LIBS)
$(TARGET_BIN_DIR)/RunDeviceDriver$(TARGET_EXEEXT): $(RUN_DEVICE_DRIVER_OBJS) $(RUN_DEVICE_DRIVER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_CANVAS_SOURCES = \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/shapelib/mapsearch.cpp \
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
	$(ENGINE_LIBS) \
	$(COMPAT_LIBS)
$(RUN_CANVAS_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_CANVAS_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_CANVAS_BIN): $(RUN_CANVAS_OBJS) $(RUN_CANVAS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_MAP_WINDOW_SOURCES = \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/AirspaceParser.cpp \
	$(SRC)/AirspaceVisibility.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/Globals.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/RenderObservationZone.cpp \
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
	$(SRC)/MapDrawHelper.cpp \
	$(SRC)/RenderTask.cpp \
	$(SRC)/RenderTaskPoint.cpp \
	$(SRC)/Marks.cpp \
	$(SRC)/Math/FastRotation.cpp \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/RasterMapJPG2000.cpp \
	$(SRC)/RasterMapRaw.cpp \
	$(SRC)/RasterMapCache.cpp \
	$(SRC)/RasterMap.cpp \
	$(SRC)/RasterTerrain.cpp \
	$(SRC)/RasterWeather.cpp \
	$(SRC)/Registry.cpp \
	$(SRC)/Screen/Animation.cpp \
	$(SRC)/Screen/LabelBlock.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/Graphics.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/Screen/shapelib/mapbits.cpp \
	$(SRC)/Screen/shapelib/maperror.cpp \
	$(SRC)/Screen/shapelib/mapprimitive.cpp \
	$(SRC)/Screen/shapelib/mapsearch.cpp \
	$(SRC)/Screen/shapelib/mapshape.cpp \
	$(SRC)/Screen/shapelib/maptree.cpp \
	$(SRC)/Screen/shapelib/mapxbase.cpp \
	$(SRC)/SettingsMapBlackboard.cpp \
	$(SRC)/SettingsComputerBlackboard.cpp \
	$(SRC)/StringUtil.cpp \
	$(SRC)/TerrainRenderer.cpp \
	$(SRC)/TextReader.cpp \
	$(SRC)/TextWriter.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Topology.cpp \
	$(SRC)/TopologyStore.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/UtilsSystem.cpp \
	$(SRC)/WayPointParser.cpp \
	$(SRC)/WayPointFile.cpp \
	$(SRC)/WayPointFileWinPilot.cpp \
	$(SRC)/WayPointFileSeeYou.cpp \
	$(SRC)/WayPointFileZander.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Simulator.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProgressDialog.cpp \
	$(TEST_SRC_DIR)/RunMapWindow.cpp
RUN_MAP_WINDOW_OBJS = $(call SRC_TO_OBJ,$(RUN_MAP_WINDOW_SOURCES))
RUN_MAP_WINDOW_BIN = $(TARGET_BIN_DIR)/RunMapWindow$(TARGET_EXEEXT)
RUN_MAP_WINDOW_LDADD = \
	$(FAKE_LIBS) \
	$(SCREEN_LIBS) \
	$(ENGINE_LIBS) \
	$(JASPER_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_MAP_WINDOW_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_MAP_WINDOW_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_MAP_WINDOW_BIN): $(RUN_MAP_WINDOW_OBJS) $(RUN_MAP_WINDOW_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DIALOG_SOURCES = \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/dlgComboPicker.cpp \
	$(SRC)/Screen/Animation.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/shapelib/mapsearch.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
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
	$(ENGINE_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(RUN_DIALOG_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_DIALOG_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_DIALOG_BIN): $(RUN_DIALOG_OBJS) $(RUN_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_AIRSPACE_WARNING_DIALOG_SOURCES = \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/dlgComboPicker.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/Dialogs/dlgAirspaceWarning.cpp \
	$(SRC)/AirspaceParser.cpp \
	$(SRC)/Screen/Animation.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/shapelib/mapsearch.cpp \
	$(SRC)/TextReader.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Registry.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/StringUtil.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/UtilsSystem.cpp \
	$(SRC)/Simulator.cpp \
	$(SRC)/Compatibility/string.c \
	$(TEST_SRC_DIR)/FakeAsset.cpp \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProgressDialog.cpp \
	$(TEST_SRC_DIR)/RunAirspaceWarningDialog.cpp
RUN_AIRSPACE_WARNING_DIALOG_BIN = $(TARGET_BIN_DIR)/RunAirspaceWarningDialog$(TARGET_EXEEXT)
RUN_AIRSPACE_WARNING_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_AIRSPACE_WARNING_DIALOG_SOURCES))
RUN_AIRSPACE_WARNING_DIALOG_LDADD = \
	$(FAKE_LIBS) \
	$(DATA_FIELD_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(ENGINE_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_AIRSPACE_WARNING_DIALOG_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_AIRSPACE_WARNING_DIALOG_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_AIRSPACE_WARNING_DIALOG_BIN): $(RUN_AIRSPACE_WARNING_DIALOG_OBJS) $(RUN_AIRSPACE_WARNING_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_TASK_EDITOR_DIALOG_SOURCES = \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/xmlParser.cpp \
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
	$(SRC)/AirspaceParser.cpp \
	$(SRC)/Screen/Animation.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/shapelib/mapsearch.cpp \
	$(SRC)/TextReader.cpp \
	$(SRC)/TextWriter.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Registry.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/StringUtil.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/UtilsFile.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/WayPointParser.cpp \
	$(SRC)/WayPointFile.cpp \
	$(SRC)/WayPointFileWinPilot.cpp \
	$(SRC)/WayPointFileSeeYou.cpp \
	$(SRC)/WayPointFileZander.cpp \
	$(SRC)/Compatibility/string.c \
	$(TEST_SRC_DIR)/FakeDialogs.cpp \
	$(TEST_SRC_DIR)/FakeInterface.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(TEST_SRC_DIR)/FakeLogFile.cpp \
	$(TEST_SRC_DIR)/FakeProfile.cpp \
	$(TEST_SRC_DIR)/FakeProgressDialog.cpp \
	$(TEST_SRC_DIR)/RunTaskEditorDialog.cpp
RUN_TASK_EDITOR_DIALOG_BIN = $(TARGET_BIN_DIR)/RunTaskEditorDialog$(TARGET_EXEEXT)
RUN_TASK_EDITOR_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_TASK_EDITOR_DIALOG_SOURCES))
RUN_TASK_EDITOR_DIALOG_LDADD = \
	$(FAKE_LIBS) \
	$(DATA_FIELD_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(ENGINE_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS) \
	$(RESOURCE_BINARY)
$(RUN_TASK_EDITOR_DIALOG_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(RUN_TASK_EDITOR_DIALOG_BIN): LDLIBS += $(SCREEN_LDLIBS)
$(RUN_TASK_EDITOR_DIALOG_BIN): $(RUN_TASK_EDITOR_DIALOG_OBJS) $(RUN_TASK_EDITOR_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

debug-: $(addprefix call-debug-,$(DEFAULT_TARGETS))
call-debug-%:
	$(MAKE) debug TARGET=$(patsubst call-debug-%,%,$@) DEBUG=$(DEBUG) V=$(V)

$(addprefix debug-,$(TARGETS)): debug-%: $(DEBUG_PROGRAMS)

debug: debug-$(TARGET)
