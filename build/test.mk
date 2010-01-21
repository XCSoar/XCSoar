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

DEBUG_PROGRAM_NAMES = RunWayPointParser RunCanvas RunMapWindow RunDialog
DEBUG_PROGRAMS = $(patsubst %,$(TARGET_BIN_DIR)/%$(TARGET_EXEEXT),$(DEBUG_PROGRAM_NAMES))

RUN_WAY_POINT_PARSER_SOURCES = \
	$(SRC)/WayPointParser.cpp \
	$(SRC)/Math/FastMath.c \
	$(SRC)/UtilsText.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/StringUtil.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/RunWayPointParser.cpp
RUN_WAY_POINT_PARSER_OBJS = $(call SRC_TO_OBJ,$(RUN_WAY_POINT_PARSER_SOURCES))
RUN_WAY_POINT_PARSER_LDADD = \
	$(ENGINE_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TARGET_BIN_DIR)/RunWayPointParser$(TARGET_EXEEXT): $(RUN_WAY_POINT_PARSER_OBJS) $(RUN_WAY_POINT_PARSER_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_CANVAS_SOURCES = \
	$(SRC)/Screen/Window.cpp \
	$(SRC)/Screen/PaintWindow.cpp \
	$(SRC)/Screen/ContainerWindow.cpp \
	$(SRC)/Screen/TopWindow.cpp \
	$(SRC)/Screen/SingleWindow.cpp \
	$(SRC)/Screen/ButtonWindow.cpp \
	$(SRC)/Screen/Canvas.cpp \
	$(SRC)/Screen/Color.cpp \
	$(SRC)/Screen/VirtualCanvas.cpp \
	$(SRC)/Screen/BufferCanvas.cpp \
	$(SRC)/Screen/Pen.cpp \
	$(SRC)/Screen/Brush.cpp \
	$(SRC)/Screen/Font.cpp \
	$(SRC)/Screen/Util.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Math/FastMath.c \
	$(SRC)/Screen/shapelib/mapsearch.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/RunCanvas.cpp
ifneq ($(ENABLE_SDL),y)
RUN_CANVAS_SOURCES += $(SRC)/Screen/PaintCanvas.cpp
endif
RUN_CANVAS_OBJS = $(call SRC_TO_OBJ,$(RUN_CANVAS_SOURCES))
RUN_CANVAS_LDADD = \
	$(ENGINE_LIBS) \
	$(COMPAT_LIBS)
$(TARGET_BIN_DIR)/RunCanvas$(TARGET_EXEEXT): $(RUN_CANVAS_OBJS) $(RUN_CANVAS_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_MAP_WINDOW_SOURCES = \
	$(SRC)/Poco/RWLock.cpp \
	$(SRC)/AirspaceParser.cpp \
	$(SRC)/AirspaceVisibility.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/Globals.cpp \
	$(SRC)/LocalPath.cpp \
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
	$(SRC)/Marks.cpp \
	$(SRC)/Math/FastMath.c \
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
	$(SRC)/Screen/Bitmap.cpp \
	$(SRC)/Screen/BitmapCanvas.cpp \
	$(SRC)/Screen/ContainerWindow.cpp \
	$(SRC)/Screen/ButtonWindow.cpp \
	$(SRC)/Screen/Canvas.cpp \
	$(SRC)/Screen/Color.cpp \
	$(SRC)/Screen/VirtualCanvas.cpp \
	$(SRC)/Screen/LabelBlock.cpp \
	$(SRC)/Screen/BufferCanvas.cpp \
	$(SRC)/Screen/Pen.cpp \
	$(SRC)/Screen/Brush.cpp \
	$(SRC)/Screen/Font.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/Graphics.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/Util.cpp \
	$(SRC)/Screen/MaskedPaintWindow.cpp \
	$(SRC)/Screen/PaintWindow.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/STScreenBuffer.cpp \
	$(SRC)/Screen/TextWindow.cpp \
	$(SRC)/Screen/TopWindow.cpp \
	$(SRC)/Screen/SingleWindow.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/Screen/Window.cpp \
	$(SRC)/Screen/shapelib/mapbits.cpp \
	$(SRC)/Screen/shapelib/maperror.cpp \
	$(SRC)/Screen/shapelib/mapprimitive.cpp \
	$(SRC)/Screen/shapelib/mapsearch.cpp \
	$(SRC)/Screen/shapelib/mapshape.cpp \
	$(SRC)/Screen/shapelib/maptree.cpp \
	$(SRC)/Screen/shapelib/mapxbase.cpp \
	$(SRC)/SettingsMapBlackboard.cpp \
	$(SRC)/SettingsComputerBlackboard.cpp \
	$(SRC)/SnailTrail.cpp \
	$(SRC)/StringUtil.cpp \
	$(SRC)/TerrainRenderer.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Topology.cpp \
	$(SRC)/TopologyStore.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/WayPointParser.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(TEST_SRC_DIR)/RunMapWindow.cpp
ifneq ($(ENABLE_SDL),y)
RUN_MAP_WINDOW_SOURCES += $(SRC)/Screen/PaintCanvas.cpp
endif
RUN_MAP_WINDOW_OBJS = $(call SRC_TO_OBJ,$(RUN_MAP_WINDOW_SOURCES))
RUN_MAP_WINDOW_LDADD = \
	$(ENGINE_LIBS) \
	$(JASPER_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS) \
	$(RESOURCE_BINARY)
$(TARGET_BIN_DIR)/RunMapWindow$(TARGET_EXEEXT): $(RUN_MAP_WINDOW_OBJS) $(RUN_MAP_WINDOW_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DIALOG_SOURCES = \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/dlgComboPicker.cpp \
	$(SRC)/Math/FastMath.c \
	$(SRC)/Screen/Animation.cpp \
	$(SRC)/Screen/Bitmap.cpp \
	$(SRC)/Screen/Brush.cpp \
	$(SRC)/Screen/Canvas.cpp \
	$(SRC)/Screen/Color.cpp \
	$(SRC)/Screen/VirtualCanvas.cpp \
	$(SRC)/Screen/BitmapCanvas.cpp \
	$(SRC)/Screen/Font.cpp \
	$(SRC)/Screen/Pen.cpp \
	$(SRC)/Screen/Window.cpp \
	$(SRC)/Screen/BufferWindow.cpp \
	$(SRC)/Screen/BufferCanvas.cpp \
	$(SRC)/Screen/PaintWindow.cpp \
	$(SRC)/Screen/ContainerWindow.cpp \
	$(SRC)/Screen/TextWindow.cpp \
	$(SRC)/Screen/EditWindow.cpp \
	$(SRC)/Screen/TopWindow.cpp \
	$(SRC)/Screen/SingleWindow.cpp \
	$(SRC)/Screen/Util.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/shapelib/mapsearch.cpp \
	$(SRC)/Thread/Debug.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/DataField/Base.cpp \
	$(SRC)/DataField/Boolean.cpp \
	$(SRC)/DataField/ComboList.cpp \
	$(SRC)/DataField/Enum.cpp \
	$(SRC)/DataField/FileReader.cpp \
	$(SRC)/DataField/Float.cpp \
	$(SRC)/DataField/Integer.cpp \
	$(SRC)/DataField/String.cpp \
	$(SRC)/Form/Control.cpp \
	$(SRC)/Form/Container.cpp \
	$(SRC)/Form/Panel.cpp \
	$(SRC)/Form/Form.cpp \
	$(SRC)/Form/Button.cpp \
	$(SRC)/Form/EventButton.cpp \
	$(SRC)/Form/Frame.cpp \
	$(SRC)/Form/Draw.cpp \
	$(SRC)/Form/List.cpp \
	$(SRC)/Form/ScrollBar.cpp \
	$(SRC)/Form/Edit.cpp \
	$(SRC)/Form/Tabbed.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(TEST_SRC_DIR)/RunDialog.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Compatibility/fmode.c
ifeq ($(ENABLE_SDL),y)
RUN_DIALOG_SOURCES += $(SRC)/Screen/Timer.cpp
else
RUN_DIALOG_SOURCES += $(SRC)/Screen/PaintCanvas.cpp
endif
RUN_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_DIALOG_SOURCES))
RUN_DIALOG_LDADD = \
	$(ENGINE_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS)
$(TARGET_BIN_DIR)/RunDialog$(TARGET_EXEEXT): $(RUN_DIALOG_OBJS) $(RUN_DIALOG_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

debug-: $(addprefix call-debug-,$(DEFAULT_TARGETS))
call-debug-%:
	$(MAKE) debug TARGET=$(patsubst call-debug-%,%,$@) DEBUG=$(DEBUG) V=$(V)

$(addprefix debug-,$(TARGETS)): debug-%: $(DEBUG_PROGRAMS)

debug: debug-$(TARGET)
