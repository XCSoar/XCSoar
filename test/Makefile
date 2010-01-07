#
# The XCSoar test suite.
#

.DEFAULT_GOAL := all

topdir = ..

include $(topdir)/build/common.mk
include $(topdir)/build/targets.mk
include $(topdir)/build/debug.mk

CPPFLAGS += -DFLARM_AVERAGE -DFIXED_MATH

include $(topdir)/build/flags.mk
include $(topdir)/build/warnings.mk
include $(topdir)/build/compile.mk

######## compiler flags

INCLUDES += -I$(SRC) -I$(ENGINE_SRC_DIR)

####### linker configuration

LDFLAGS = $(TARGET_LDFLAGS) $(PROFILE)
LDLIBS = $(TARGET_LDLIBS)

include $(topdir)/build/task.mk
include $(topdir)/build/sdl.mk
include $(topdir)/build/gconf.mk

.phony:

BUILDTESTS=\
	t/01_test_tap.exe

all: $(BUILDTESTS) RunWayPointParser-$(TARGET)$(TARGET_EXEEXT) \
	RunCanvas-$(TARGET)$(TARGET_EXEEXT) \
	RunMapWindow-$(TARGET)$(TARGET_EXEEXT) \
	RunDialog-$(TARGET)$(TARGET_EXEEXT)

test: .phony $(BUILDTESTS)
	perl tools/testall.pl t/*

# TODO generalise
t/01_test_tap.exe: src/01_test_tap.c
	gcc -o $@ $<

RUN_WAY_POINT_PARSER_SOURCES = \
	$(SRC)/WayPointParser.cpp \
	$(SRC)/Math/FastMath.c \
	$(SRC)/UtilsText.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/StringUtil.cpp \
	$(SRC)/Compatibility/fmode.c \
	src/RunWayPointParser.cpp
RUN_WAY_POINT_PARSER_OBJS = $(call SRC_TO_OBJ,$(RUN_WAY_POINT_PARSER_SOURCES))
RUN_WAY_POINT_PARSER_LDADD = \
	$(ENGINE_SRC_DIR)/task-$(TARGET).a \
	$(SRC)/zzip-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a
RunWayPointParser-$(TARGET)$(TARGET_EXEEXT): $(RUN_WAY_POINT_PARSER_OBJS) $(RUN_WAY_POINT_PARSER_LDADD)
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
	src/RunCanvas.cpp
ifneq ($(ENABLE_SDL),y)
RUN_CANVAS_SOURCES += $(SRC)/Screen/PaintCanvas.cpp
endif
RUN_CANVAS_OBJS = $(call SRC_TO_OBJ,$(RUN_CANVAS_SOURCES))
RUN_CANVAS_LDADD = \
	$(ENGINE_SRC_DIR)/task-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a
RunCanvas-$(TARGET)$(TARGET_EXEEXT): $(RUN_CANVAS_OBJS) $(RUN_CANVAS_LDADD)
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
	src/RunMapWindow.cpp
ifneq ($(ENABLE_SDL),y)
RUN_MAP_WINDOW_SOURCES += $(SRC)/Screen/PaintCanvas.cpp
endif
ifeq ($(HAVE_WIN32),y)
RUN_MAP_WINDOW_SOURCES += $(SRC)/XCSoar-$(TARGET).rsc
endif
RUN_MAP_WINDOW_OBJS = $(call SRC_TO_OBJ,$(RUN_MAP_WINDOW_SOURCES))
RUN_MAP_WINDOW_LDADD = \
	$(ENGINE_SRC_DIR)/task-$(TARGET).a \
	$(SRC)/jasper-$(TARGET).a \
	$(SRC)/zzip-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a
RunMapWindow-$(TARGET)$(TARGET_EXEEXT): $(RUN_MAP_WINDOW_OBJS) $(RUN_MAP_WINDOW_LDADD)
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
	$(SRC)/Form/Form.cpp \
	$(SRC)/Form/Button.cpp \
	$(SRC)/Form/EventButton.cpp \
	$(SRC)/Form/Frame.cpp \
	$(SRC)/Form/Draw.cpp \
	$(SRC)/Form/List.cpp \
	$(SRC)/Form/ScrollBar.cpp \
	$(SRC)/Form/Edit.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	src/RunDialog.cpp \
	$(SRC)/Compatibility/string.c \
	$(SRC)/Compatibility/fmode.c
ifeq ($(ENABLE_SDL),y)
RUN_DIALOG_SOURCES += $(SRC)/Screen/Timer.cpp
else
RUN_DIALOG_SOURCES += $(SRC)/Screen/PaintCanvas.cpp
endif
RUN_DIALOG_OBJS = $(call SRC_TO_OBJ,$(RUN_DIALOG_SOURCES))
RUN_DIALOG_LDADD = \
	$(ENGINE_SRC_DIR)/task-$(TARGET).a \
	$(SRC)/zzip-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a
RunDialog-$(TARGET)$(TARGET_EXEEXT): $(RUN_DIALOG_OBJS) $(RUN_DIALOG_LDADD)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

%-$(TARGET).rsc: %.rc
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

include $(topdir)/build/zzip.mk
include $(topdir)/build/jasper.mk
include $(topdir)/build/compat.mk

clean:
	rm -f $(BUILDTESTS) \
		RunWayPointParser-$(TARGET)$(TARGET_EXEEXT) \
		RunCanvas-$(TARGET)$(TARGET_EXEEXT) \
		RunMapWindow-$(TARGET)$(TARGET_EXEEXT) \
		RunDialog-$(TARGET)$(TARGET_EXEEXT)
	rm -f $(RUN_WAY_POINT_PARSER_OBJS:.cpp=-$(TARGET).cpp)
	rm -f $(RUN_DIALOG_OBJS:.cpp=-$(TARGET).cpp)
