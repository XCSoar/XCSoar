#
# The XCSoar test suite.
#

.DEFAULT_GOAL := all

topdir = ..

include $(topdir)/build/common.mk
include $(topdir)/build/targets.mk
include $(topdir)/build/debug.mk
include $(topdir)/build/flags.mk
include $(topdir)/build/warnings.mk
include $(topdir)/build/compile.mk

######## compiler flags

INCLUDES += $(TARGET_INCLUDES) -I$(HDR) -I$(SRC)
CPPFLAGS += -DFLARM_AVERAGE

####### linker configuration

LDFLAGS = $(TARGET_LDFLAGS) $(PROFILE)
LDLIBS = $(TARGET_LDLIBS)

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
	# prove
	perl tools/testall.pl t/*

# TODO generalise
t/01_test_tap.exe: src/01_test_tap.c
	gcc -o $@ $<

RUN_WAY_POINT_PARSER_OBJS = \
	$(SRC)/WayPointParser.o \
	$(SRC)/WayPointList.o \
	$(SRC)/Math/Earth.o \
	$(SRC)/Math/FastMath.o \
	$(SRC)/Math/Geometry.o \
	$(SRC)/UtilsText.o \
	$(SRC)/zzip-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a \
	$(SRC)/LocalPath.o \
	$(SRC)/StringUtil.o \
	$(SRC)/Compatibility/fmode.o \
	src/RunWayPointParser.o
RunWayPointParser-$(TARGET)$(TARGET_EXEEXT): $(RUN_WAY_POINT_PARSER_OBJS:.o=-$(TARGET).o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_CANVAS_OBJS = \
	$(SRC)/Screen/Window.o \
	$(SRC)/Screen/PaintWindow.o \
	$(SRC)/Screen/ContainerWindow.o \
	$(SRC)/Screen/TopWindow.o \
	$(SRC)/Screen/SingleWindow.o \
	$(SRC)/Screen/ButtonWindow.o \
	$(SRC)/Screen/Canvas.o \
	$(SRC)/Screen/Color.o \
	$(SRC)/Screen/VirtualCanvas.o \
	$(SRC)/Screen/BufferCanvas.o \
	$(SRC)/Screen/Pen.o \
	$(SRC)/Screen/Brush.o \
	$(SRC)/Screen/Font.o \
	$(SRC)/Screen/Util.o \
	$(SRC)/Screen/Layout.o \
	$(SRC)/Math/FastMath.o \
	$(SRC)/Math/Geometry.o \
	$(SRC)/Screen/shapelib/mapsearch.o \
	$(SRC)/Thread/Debug.o \
	$(SRC)/Thread/Mutex.o \
	$(SRC)/Compatibility/fmode.o \
	src/RunCanvas.o \
	$(SRC)/compat-$(TARGET).a
ifneq ($(ENABLE_SDL),y)
RUN_CANVAS_OBJS += $(SRC)/Screen/PaintCanvas.o
endif
RunCanvas-$(TARGET)$(TARGET_EXEEXT): $(RUN_CANVAS_OBJS:.o=-$(TARGET).o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_MAP_WINDOW_OBJS = \
	$(SRC)/Poco/RWLock.o \
	$(SRC)/Airspace.o \
	$(SRC)/AirspaceDatabase.o \
	$(SRC)/AirspaceParser.o \
	$(SRC)/Appearance.o \
	$(SRC)/Globals.o \
	$(SRC)/LocalPath.o \
	$(SRC)/MapWindow.o \
	$(SRC)/MapWindowAirspace.o \
	$(SRC)/MapWindowBlackboard.o \
	$(SRC)/MapWindowEvents.o \
	$(SRC)/MapWindowGlideRange.o \
	$(SRC)/MapWindowLabels.o \
	$(SRC)/MapWindowProjection.o \
	$(SRC)/MapWindowRender.o \
	$(SRC)/MapWindowScale.o \
	$(SRC)/MapWindowSymbols.o \
	$(SRC)/MapWindowTask.o \
	$(SRC)/MapWindowThermal.o \
	$(SRC)/MapWindowTimer.o \
	$(SRC)/MapWindowTraffic.o \
	$(SRC)/MapWindowTrail.o \
	$(SRC)/MapWindowWaypoints.o \
	$(SRC)/Marks.o \
	$(SRC)/Math/Earth.o \
	$(SRC)/Math/FastMath.o \
	$(SRC)/Math/Geometry.o \
	$(SRC)/Math/Pressure.o \
	$(SRC)/Math/Screen.o \
	$(SRC)/MacCready.o \
	$(SRC)/OnLineContest.o \
	$(SRC)/RasterMapJPG2000.o \
	$(SRC)/RasterMapRaw.o \
	$(SRC)/RasterMapCache.o \
	$(SRC)/RasterMap.o \
	$(SRC)/RasterTerrain.o \
	$(SRC)/RasterWeather.o \
	$(SRC)/Registry.o \
	$(SRC)/Screen/Animation.o \
	$(SRC)/Screen/Bitmap.o \
	$(SRC)/Screen/BitmapCanvas.o \
	$(SRC)/Screen/ContainerWindow.o \
	$(SRC)/Screen/ButtonWindow.o \
	$(SRC)/Screen/Canvas.o \
	$(SRC)/Screen/Color.o \
	$(SRC)/Screen/VirtualCanvas.o \
	$(SRC)/Screen/LabelBlock.o \
	$(SRC)/Screen/BufferCanvas.o \
	$(SRC)/Screen/Pen.o \
	$(SRC)/Screen/Brush.o \
	$(SRC)/Screen/Font.o \
	$(SRC)/Screen/Fonts.o \
	$(SRC)/Screen/Graphics.o \
	$(SRC)/Screen/Layout.o \
	$(SRC)/Screen/Util.o \
	$(SRC)/Screen/MaskedPaintWindow.o \
	$(SRC)/Screen/PaintWindow.o \
	$(SRC)/Screen/Ramp.o \
	$(SRC)/Screen/STScreenBuffer.o \
	$(SRC)/Screen/TextWindow.o \
	$(SRC)/Screen/TopWindow.o \
	$(SRC)/Screen/SingleWindow.o \
	$(SRC)/Screen/UnitSymbol.o \
	$(SRC)/Screen/Window.o \
	$(SRC)/Screen/shapelib/mapbits.o \
	$(SRC)/Screen/shapelib/maperror.o \
	$(SRC)/Screen/shapelib/mapprimitive.o \
	$(SRC)/Screen/shapelib/mapsearch.o \
	$(SRC)/Screen/shapelib/mapshape.o \
	$(SRC)/Screen/shapelib/maptree.o \
	$(SRC)/Screen/shapelib/mapxbase.o \
	$(SRC)/SettingsMapBlackboard.o \
	$(SRC)/SettingsComputerBlackboard.o \
	$(SRC)/SnailTrail.o \
	$(SRC)/StringUtil.o \
	$(SRC)/Task.o \
	$(SRC)/TaskImpl.o \
	$(SRC)/TaskVisitor.o \
	$(SRC)/TaskFile.o \
	$(SRC)/TerrainRenderer.o \
	$(SRC)/Thread/Debug.o \
	$(SRC)/Thread/Mutex.o \
	$(SRC)/Topology.o \
	$(SRC)/TopologyStore.o \
	$(SRC)/Units.o \
	$(SRC)/UtilsText.o \
	$(SRC)/UtilsFont.o \
	$(SRC)/WayPointList.o \
	$(SRC)/WayPointParser.o \
	$(SRC)/Compatibility/fmode.o \
	$(SRC)/jasper-$(TARGET).a \
	$(SRC)/zzip-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a \
	src/RunMapWindow.o
ifneq ($(ENABLE_SDL),y)
RUN_MAP_WINDOW_OBJS += $(SRC)/Screen/PaintCanvas.o
endif
ifeq ($(HAVE_WIN32),y)
RUN_MAP_WINDOW_OBJS += $(SRC)/XCSoar-$(TARGET).rsc
endif
RunMapWindow-$(TARGET)$(TARGET_EXEEXT): $(RUN_MAP_WINDOW_OBJS:.o=-$(TARGET).o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DIALOG_OBJS = \
	$(SRC)/xmlParser.o \
	$(SRC)/Dialogs/XML.o \
	$(SRC)/Dialogs/dlgComboPicker.o \
	$(SRC)/Math/FastMath.o \
	$(SRC)/Math/Geometry.o \
	$(SRC)/Screen/Animation.o \
	$(SRC)/Screen/Bitmap.o \
	$(SRC)/Screen/Brush.o \
	$(SRC)/Screen/Canvas.o \
	$(SRC)/Screen/Color.o \
	$(SRC)/Screen/VirtualCanvas.o \
	$(SRC)/Screen/BitmapCanvas.o \
	$(SRC)/Screen/Font.o \
	$(SRC)/Screen/Pen.o \
	$(SRC)/Screen/Window.o \
	$(SRC)/Screen/BufferWindow.o \
	$(SRC)/Screen/BufferCanvas.o \
	$(SRC)/Screen/PaintWindow.o \
	$(SRC)/Screen/ContainerWindow.o \
	$(SRC)/Screen/TextWindow.o \
	$(SRC)/Screen/EditWindow.o \
	$(SRC)/Screen/TopWindow.o \
	$(SRC)/Screen/SingleWindow.o \
	$(SRC)/Screen/Util.o \
	$(SRC)/Screen/Layout.o \
	$(SRC)/Screen/shapelib/mapsearch.o \
	$(SRC)/Thread/Debug.o \
	$(SRC)/Thread/Mutex.o \
	$(SRC)/DataField/Base.o \
	$(SRC)/DataField/Boolean.o \
	$(SRC)/DataField/ComboList.o \
	$(SRC)/DataField/Enum.o \
	$(SRC)/DataField/FileReader.o \
	$(SRC)/DataField/Float.o \
	$(SRC)/DataField/Integer.o \
	$(SRC)/DataField/String.o \
	$(SRC)/Form/Control.o \
	$(SRC)/Form/Form.o \
	$(SRC)/Form/Button.o \
	$(SRC)/Form/EventButton.o \
	$(SRC)/Form/Frame.o \
	$(SRC)/Form/Draw.o \
	$(SRC)/Form/List.o \
	$(SRC)/Form/ScrollBar.o \
	$(SRC)/Form/Edit.o \
	$(SRC)/UtilsText.o \
	$(SRC)/Dialogs/dlgHelp.o \
	$(SRC)/zzip-$(TARGET).a \
	src/RunDialog.o \
	$(SRC)/Compatibility/string.o \
	$(SRC)/Compatibility/fmode.o \
	$(SRC)/compat-$(TARGET).a
ifeq ($(ENABLE_SDL),y)
RUN_DIALOG_OBJS += $(SRC)/Screen/Timer.o
else
RUN_DIALOG_OBJS += $(SRC)/Screen/PaintCanvas.o
endif
RunDialog-$(TARGET)$(TARGET_EXEEXT): $(RUN_DIALOG_OBJS:.o=-$(TARGET).o)
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
	rm -f $(RUN_WAY_POINT_PARSER_OBJS:.o=-$(TARGET).o)
	rm -f $(RUN_DIALOG_OBJS:.o=-$(TARGET).o)
