#
# This is the XCSoar build script.  To compile XCSoar, you must
# specify the target platform, e.g. for Pocket PC 2003, type:
#
#   make TARGET=PPC2003
#
# The following parameters may be specified on the "make" command
# line:
#
#   TARGET      The name of the target platform.  See the TARGETS variable
#               below for a list of valid target platforms.
#
#   ENABLE_SDL  If set to "y", the UI is drawn with libSDL.
#
#   DEBUG       If set to "y", the debugging version of XCSoar is built.
#
#   V           Verbosity; 1 is the default, and prints terse information.
#               0 means quiet, and 2 prints the full compiler commands.
#

topdir = .

SRC = $(topdir)/Common/Source
HDR = $(topdir)/Common/Header

include $(topdir)/build/targets.mk
include $(topdir)/build/debug.mk
include $(topdir)/build/verbose.mk
include $(topdir)/build/compile.mk

######## output files

OUTPUTS 	:= XCSoar-$(TARGET)$(TARGET_EXEEXT) XCSoarSimulator-$(TARGET)$(TARGET_EXEEXT)
ifeq ($(CONFIG_ALTAIR),y)
OUTPUTS 	:= XCSoar-$(TARGET)$(TARGET_EXEEXT)
endif
ifeq ($(ALTAIR_PORTRAIT),y)
OUTPUTS 	:= XCSoar-$(TARGET)$(TARGET_EXEEXT)
endif
ifeq ($(CONFIG_PNA),y)
OUTPUTS 	:= XCSoar-$(TARGET)$(TARGET_EXEEXT)
endif

######## paths

INCLUDES := $(TARGET_INCLUDES) -I$(HDR) -I$(SRC) 

######## compiler flags

CPPFLAGS := $(INCLUDES) $(TARGET_CPPFLAGS)
CPPFLAGS	+= -DFLARM_AVERAGE 

CXXFLAGS	:=$(OPTIMIZE) -fno-exceptions $(PROFILE)
CFLAGS		:=$(OPTIMIZE) $(PROFILE)

include $(topdir)/build/warnings.mk

####### linker configuration

LDFLAGS = $(TARGET_LDFLAGS) $(PROFILE)
LDLIBS = $(TARGET_LDLIBS)

MAKEFLAGS	+=-r

####### sources

ifeq ($(CONFIG_PC),n)
#CPPFLAGS_Common_Source_ :=-Werror
endif

DEVS	:=\
	$(SRC)/Device/Driver/AltairPro.o \
	$(SRC)/Device/Driver/BorgeltB50.o \
	$(SRC)/Device/Driver/CAI302.o \
	$(SRC)/Device/Driver/CaiGpsNav.o \
	$(SRC)/Device/Driver/Condor.o \
	$(SRC)/Device/Driver/EW.o \
	$(SRC)/Device/Driver/EWMicroRecorder.o \
	$(SRC)/Device/Driver/FlymasterF1.o \
	$(SRC)/Device/Driver/Generic.o \
	$(SRC)/Device/Driver/LX.o \
	$(SRC)/Device/Driver/NmeaOut.o \
	$(SRC)/Device/Driver/PosiGraph.o \
	$(SRC)/Device/Driver/Vega.o \
	$(SRC)/Device/Driver/Volkslogger.o \
	$(SRC)/Device/Driver/XCOM760.o \
	$(SRC)/Device/Driver/Zander.o

DLGS	:=\
	$(SRC)/Dialogs/XML.o \
	$(SRC)/Dialogs/Message.o \
	$(SRC)/Dialogs/dlgAirspace.o \
	$(SRC)/Dialogs/dlgAirspaceColours.o \
	$(SRC)/Dialogs/dlgAirspaceDetails.o \
	$(SRC)/Dialogs/dlgAirspacePatterns.o \
	$(SRC)/Dialogs/dlgAirspaceSelect.o \
	$(SRC)/Dialogs/dlgAirspaceWarning.o \
	$(SRC)/Dialogs/dlgBasicSettings.o \
	$(SRC)/Dialogs/dlgBrightness.o \
	$(SRC)/Dialogs/dlgChecklist.o \
	$(SRC)/Dialogs/dlgComboPicker.o \
	$(SRC)/Dialogs/dlgConfiguration.o \
	$(SRC)/Dialogs/dlgConfiguration2.o \
	$(SRC)/Dialogs/dlgConfigurationVario.o \
	$(SRC)/Dialogs/dlgFlarmTraffic.o \
	$(SRC)/Dialogs/dlgHelp.o \
	$(SRC)/Dialogs/dlgLoggerReplay.o \
	$(SRC)/Dialogs/dlgStartPoint.o \
	$(SRC)/Dialogs/dlgStartup.o \
	$(SRC)/Dialogs/dlgStatistics.o \
	$(SRC)/Dialogs/dlgStatus.o \
	$(SRC)/Dialogs/dlgSwitches.o \
	$(SRC)/Dialogs/dlgTarget.o \
	$(SRC)/Dialogs/dlgTaskCalculator.o \
	$(SRC)/Dialogs/dlgTaskOverview.o \
	$(SRC)/Dialogs/dlgTaskRules.o \
	$(SRC)/Dialogs/dlgTaskWaypoint.o \
	$(SRC)/Dialogs/dlgTeamCode.o \
	$(SRC)/Dialogs/dlgTextEntry.o \
	$(SRC)/Dialogs/dlgTextEntry_Keyboard.o \
	$(SRC)/Dialogs/dlgTools.o \
	$(SRC)/Dialogs/dlgHelpers.o \
	$(SRC)/Dialogs/dlgVegaDemo.o \
	$(SRC)/Dialogs/dlgVoice.o \
	$(SRC)/Dialogs/dlgWayPointDetails.o \
	$(SRC)/Dialogs/dlgWaypointEdit.o \
	$(SRC)/Dialogs/dlgWayPointSelect.o \
	$(SRC)/Dialogs/dlgWaypointOutOfTerrain.o \
	$(SRC)/Dialogs/dlgWeather.o \
	$(SRC)/Dialogs/dlgWindSettings.o \
	$(SRC)/Dialogs/dlgStartTask.o \
	$(SRC)/Dialogs/dlgFontEdit.o \

VOLKS	:=\
	$(SRC)/Device/Volkslogger/dbbconv.cpp \
	$(SRC)/Device/Volkslogger/grecord.cpp \
	$(SRC)/Device/Volkslogger/vlapi2.cpp \
	$(SRC)/Device/Volkslogger/vlapihlp.cpp \
	$(SRC)/Device/Volkslogger/vlapisys_win.cpp \
	$(SRC)/Device/Volkslogger/vlconv.cpp \
	$(SRC)/Device/Volkslogger/vlutils.cpp



OBJS	:=\
	$(SRC)/Globals.o 		\
	\
	$(SRC)/Poco/RWLock.o		\
	\
	$(SRC)/AATDistance.o 		\
	$(SRC)/Abort.o 			\
	$(SRC)/Airspace.o 		\
	$(SRC)/AirspaceDatabase.o \
	$(SRC)/AirspaceParser.o 	\
	$(SRC)/AirspaceWarning.o 	\
	$(SRC)/Atmosphere.o 		\
	$(SRC)/BestAlternate.o 		\
	$(SRC)/ClimbAverageCalculator.o \
	$(SRC)/ConditionMonitor.o 	\
	$(SRC)/Calibration.o 		\
	$(SRC)/Calculations.o 		\
	$(SRC)/FlarmIdFile.o 		\
	$(SRC)/FlarmCalculations.o 	\
	$(SRC)/GlideComputer.o 		\
	$(SRC)/GlideComputerBlackboard.o 	\
	$(SRC)/GlideComputerAirData.o 	\
	$(SRC)/GlideComputerInterface.o \
	$(SRC)/GlideComputerStats.o 	\
	$(SRC)/GlideComputerTask.o 	\
	$(SRC)/GlideRatio.o 		\
	$(SRC)/GlideSolvers.o 		\
	$(SRC)/Logger.o 		\
	$(SRC)/LoggerImpl.o 		\
	$(SRC)/LoggerSign.o 		\
	$(SRC)/ReplayLogger.o 		\
	$(SRC)/McReady.o 		\
	$(SRC)/OnLineContest.o 		\
	$(SRC)/SnailTrail.o 		\
	$(SRC)/Task.o			\
	$(SRC)/TaskImpl.o		\
	$(SRC)/TaskFile.o		\
	$(SRC)/TaskVisitor.o		\
	$(SRC)/TeamCodeCalculation.o 	\
	$(SRC)/ThermalLocator.o 	\
	$(SRC)/Waypointparser.o 	\
	$(SRC)/WayPoint.o 		\
	$(SRC)/WayPointList.o 		\
	$(SRC)/windanalyser.o 		\
	$(SRC)/windmeasurementlist.o 	\
	$(SRC)/windstore.o 		\
	$(SRC)/WindZigZag.o 		\
	\
	$(SRC)/Gauge/GaugeCDI.o \
	$(SRC)/Gauge/GaugeFLARM.o \
	$(SRC)/Gauge/GaugeVario.o \
	\
	$(SRC)/AirfieldDetails.o 	\
	$(SRC)/ButtonLabel.o 		\
	$(SRC)/DataField/Base.o 	\
	$(SRC)/DataField/Boolean.o 	\
	$(SRC)/DataField/ComboList.o 	\
	$(SRC)/DataField/Enum.o 	\
	$(SRC)/DataField/FileReader.o 	\
	$(SRC)/DataField/Float.o 	\
	$(SRC)/DataField/Integer.o 	\
	$(SRC)/DataField/String.o 	\
	$(SRC)/Dialogs.o 		\
	$(SRC)/ExpandMacros.o 		\
	$(SRC)/Formatter/Base.o 	\
	$(SRC)/Formatter/TeamCode.o 	\
	$(SRC)/Formatter/WayPoint.o 	\
	$(SRC)/Formatter/LowWarning.o 	\
	$(SRC)/Formatter/Time.o 	\
	$(SRC)/InfoBox.o 		\
	$(SRC)/InfoBoxLayout.o 		\
	$(SRC)/InfoBoxEvents.o 		\
	$(SRC)/InfoBoxManager.o 	\
	$(SRC)/InputEvents.o 		\
	$(SRC)/InputEventsActions.o 	\
	$(SRC)/StatusMessage.o \
	$(SRC)/PopupMessage.o \
	$(SRC)/WindowControls.o 	\
	$(SRC)/LogFile.o 		\
	\
	$(SRC)/MapWindow.o 		\
	$(SRC)/MapWindowAirspace.o 	\
	$(SRC)/MapWindowEvents.o	\
	$(SRC)/MapWindowGlideRange.o 	\
	$(SRC)/MapWindowLabels.o 	\
	$(SRC)/MapWindowProjection.o 	\
	$(SRC)/MapWindowRender.o 	\
	$(SRC)/MapWindowScale.o 	\
	$(SRC)/MapWindowSymbols.o 	\
	$(SRC)/MapWindowTask.o 		\
	$(SRC)/MapWindowTarget.o	\
	$(SRC)/MapWindowThermal.o	\
	$(SRC)/MapWindowTimer.o 	\
	$(SRC)/MapWindowTraffic.o 	\
	$(SRC)/MapWindowTrail.o 	\
	$(SRC)/MapWindowWaypoints.o 	\
	$(SRC)/DrawThread.o \
	\
	$(SRC)/DeviceBlackboard.o 	\
	$(SRC)/InstrumentBlackboard.o 	\
	$(SRC)/InterfaceBlackboard.o 	\
	$(SRC)/MapProjectionBlackboard.o 	\
	$(SRC)/MapWindowBlackboard.o 	\
	$(SRC)/SettingsMapBlackboard.o 	\
	$(SRC)/SettingsComputerBlackboard.o 	\
	$(SRC)/CalculationThread.o 	\
	$(SRC)/InstrumentThread.o 	\
	\
	$(SRC)/Topology.o		\
	$(SRC)/TopologyStore.o		\
	$(SRC)/RasterMap.o 		\
	$(SRC)/RasterMapCache.o 	\
	$(SRC)/RasterMapJPG2000.o 	\
	$(SRC)/RasterMapRaw.o 		\
	$(SRC)/RasterTerrain.o 		\
	$(SRC)/RasterWeather.o 		\
	$(SRC)/TerrainRenderer.o	\
	$(SRC)/Marks.o 			\
	\
	$(SRC)/Persist.o 		\
	$(SRC)/FlightStatistics.o 	\
	\
	$(SRC)/Asset.o 			\
	$(SRC)/Appearance.o 		\
	$(SRC)/Battery.o 		\
	$(SRC)/Language.o 		\
	$(SRC)/LocalPath.o 		\
	$(SRC)/Interface.o		\
	$(SRC)/LocalTime.o		\
	$(SRC)/Units.o 			\
	$(SRC)/StringUtil.o \
	$(SRC)/UtilsAirspace.o		\
	$(SRC)/UtilsFLARM.o		\
	$(SRC)/UtilsFont.o		\
	$(SRC)/UtilsProfile.o		\
	$(SRC)/UtilsSettings.o		\
	$(SRC)/UtilsSystem.o		\
	$(SRC)/UtilsText.o		\
	$(SRC)/Version.o 		\
	$(SRC)/Audio/Sound.o \
	$(SRC)/Audio/VegaVoice.o	\
	$(SRC)/Compatibility/string.o 	\
	$(SRC)/Registry.o 		\
	$(SRC)/Profile.o \
	$(SRC)/xmlParser.o 		\
	$(SRC)/Thread/Thread.o \
	\
	$(SRC)/Math/Earth.o 		\
	$(SRC)/Math/FastMath.o 		\
	$(SRC)/Math/Geometry.o 		\
	$(SRC)/Math/leastsqs.o 		\
	$(SRC)/Math/LowPassFilter.o 	\
	$(SRC)/Math/NavFunctions.o	\
	$(SRC)/Math/Pressure.o 		\
	$(SRC)/Math/Screen.o 		\
	$(SRC)/Math/SunEphemeris.o 	\
	\
	$(SRC)/Screen/Animation.o 	\
	$(SRC)/Screen/Blank.o 		\
	$(SRC)/Screen/ButtonWindow.o \
	$(SRC)/Screen/Chart.o 		\
	$(SRC)/Screen/Fonts.o 		\
	$(SRC)/Screen/UnitSymbol.o \
	$(SRC)/Screen/Graphics.o 	\
	$(SRC)/Screen/Ramp.o 		\
	$(SRC)/Screen/STScreenBuffer.o 	\
	$(SRC)/Screen/Util.o 		\
	$(SRC)/Screen/VOIMAGE.o 	\
	$(SRC)/Screen/Bitmap.o \
	$(SRC)/Screen/Brush.o \
	$(SRC)/Screen/Canvas.o \
	$(SRC)/Screen/VirtualCanvas.o \
	$(SRC)/Screen/BitmapCanvas.o \
	$(SRC)/Screen/Font.o \
	$(SRC)/Screen/Pen.o \
	$(SRC)/Screen/LabelBlock.o \
	$(SRC)/Screen/Window.o \
	$(SRC)/Screen/BufferWindow.o \
	$(SRC)/Screen/PaintWindow.o \
	$(SRC)/Screen/MaskedPaintWindow.o \
	$(SRC)/Screen/ContainerWindow.o \
	$(SRC)/Screen/TextWindow.o \
	$(SRC)/Screen/EditWindow.o \
	$(SRC)/Screen/TopWindow.o \
	$(SRC)/Screen/Dialog.o \
	$(SRC)/Screen/ProgressWindow.o \
	\
	$(SRC)/Screen/shapelib/mapbits.o 	\
	$(SRC)/Screen/shapelib/maperror.o 	\
	$(SRC)/Screen/shapelib/mapprimitive.o 	\
	$(SRC)/Screen/shapelib/mapsearch.o 	\
	$(SRC)/Screen/shapelib/mapshape.o 	\
	$(SRC)/Screen/shapelib/maptree.o 	\
	$(SRC)/Screen/shapelib/mapxbase.o 	\
	\
	$(SRC)/Polar/Polar.o \
	$(SRC)/Polar/Loader.o \
	$(SRC)/Polar/WinPilot.o 	\
	$(SRC)/Polar/BuiltIn.o 		\
	$(SRC)/Polar/Historical.o 	\
	\
	$(SRC)/Blackboard.o 		\
	$(SRC)/Protection.o 		\
	$(SRC)/ProcessTimer.o 		\
	$(SRC)/MainWindow.o \
	$(SRC)/Components.o 		\
	$(SRC)/XCSoar.o 		\
	\
	$(SRC)/Device/Driver.o \
	$(SRC)/Device/device.o 		\
	$(SRC)/Device/Geoid.o 		\
	$(SRC)/Device/Parser.o		\
	$(SRC)/Device/Port.o 		\
	$(SRC)/Device/FLARM.o \
	$(SRC)/Device/Internal.o \
	$(DEVS) 			\
	\
	$(DLGS:.cpp=.o) 		\
	$(VOLKS:.cpp=.o) 		\
	$(SRC)/XCSoar-$(TARGET).rsc \
	$(SRC)/jasper-$(TARGET).a \
	$(SRC)/zzip-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a

#	$(SRC)/VarioSound.o \
#	$(SRC)/WaveThread.o \


XCSOARSETUP_OBJS=\
	$(SRC)/XcSoarSetup.o

XCSOARLAUNCH_OBJS=\
	$(SRC)/XCSoarLaunch.o

include $(topdir)/build/sdl.mk

all: all-$(TARGET)

# if no TARGET is set, build all targets
all-: $(addprefix call-,$(DEFAULT_TARGETS))
call-%:
	$(MAKE) TARGET=$(patsubst call-%,%,$@) DEBUG=$(DEBUG) V=$(V)

$(addprefix all-,$(TARGETS)): all-%: $(OUTPUTS)

####### products

install: XCSoar-$(TARGET).exe XCSoarSimulator-$(TARGET).exe
	@echo Copying to device...
	-$(SYNCE_PRM) ':/Program Files/XCSoar/XCSoar.exe'
	-$(SYNCE_PRM) ':/Program Files/XCSoar/XCSoarSimulator.exe'
	$(SYNCE_PCP) XCSoar-$(TARGET).exe ':/Program Files/XCSoar/XCSoar.exe'
	$(SYNCE_PCP) XCSoarSimulator-$(TARGET).exe ':/Program Files/XCSoar/XCSoarSimulator.exe'

cab:	XCSoar-$(TARGET).exe XCSoarSimulator-$(TARGET).exe
	@echo Making cabs
	cp XCSoar-$(TARGET).exe $(GTARGET)/XCSoar/gcc/XCSoar.exe
	cp XCSoarSimulator-$(TARGET).exe $(GTARGET)/XCSoarSimulator/gcc/XCSoarSimulator.exe
	wine $(GTARGET)/Cabwiz.exe XCSoar$(TARGET)-gcc.inf /cpu $(PCPU)
	mv XCSoar$(TARGET)-gcc.$(PCPU).CAB XCSoar$(TARGET).$(PCPU).CAB

#	wine ezsetup.exe -l english -i XCSoar$(TARGET).ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-$(TARGET).exe

ifneq ($(NOSTRIP_SUFFIX),)
XCSoar-$(TARGET)$(TARGET_EXEEXT): XCSoar-$(TARGET)$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT)
	@$(NQ)echo "  STRIP   $@"
	$(Q)$(STRIP) $< -o $@
	$(Q)$(SIZE) $@

XCSoarSimulator-$(TARGET)$(TARGET_EXEEXT): XCSoarSimulator-$(TARGET)$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT)
	@$(NQ)echo "  STRIP   $@"
	$(Q)$(STRIP) $< -o $@
	$(Q)$(SIZE) $@
endif

XCSoar-$(TARGET)$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT): $(OBJS:.o=-$(TARGET).o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

XCSoarSimulator-$(TARGET)$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT): $(OBJS:.o=-$(TARGET)-Simulator.o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

XCSoarSetup.dll: $(XCSOARSETUP_OBJS)
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
# JMW not tested yet, probably need to use dlltool?

XCSoarLaunch.dll: $(XCSOARLAUNCH_OBJS)
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

#
# Create libraries for zzip, jasper and compatibility stuff
#

include $(topdir)/build/zzip.mk
include $(topdir)/build/jasper.mk
include $(topdir)/build/compat.mk

####### shared objects
#
# Tell make how to create a compiled resource object (rsc)
#
%-$(TARGET).rsc: %.rc
	@sed -e 's,[Bb]itmaps\\\\,Bitmaps/,g' \
	    -e 's,XCSoar.ICO,xcsoar.ico,g' \
	    -e 's,\.\.\\\\Data\\\\Dialogs\\\\,../Data/Dialogs/,g' \
	    -e 's,small\.bmp,Small.bmp,g' \
		< $< > $<.tmp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) $<.tmp $@
	@$(RM) $<.tmp

IGNORE	:= \( -name .svn -o -name CVS -o -name .git \) -prune -o

clean-WINE:
	$(RM) XCSoar-WINE XCSoarSimulator-WINE
	$(RM) XCSoar-WINE.exe.so XCSoarSimulator-WINE.exe.so

clean-%: TARGET=$(patsubst clean-%,%,$@)
$(addprefix clean-,$(filter-out WINE,$(TARGETS))): clean-%:
	$(RM) XCSoar-$(TARGET)$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT) XCSoarSimulator-$(TARGET)$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT)
	$(RM) XCSoar-$(TARGET)$(TARGET_EXEEXT) XCSoarSimulator-$(TARGET)$(TARGET_EXEEXT)
	$(RM) TAGS

clean-: $(addprefix clean-,$(TARGETS))

clean: clean-$(TARGET) cleani FORCE
	find Common $(IGNORE) \( -name '*.[oa]' -o -name '*.rsc' -o -name '.*.d' \) \
	-type f -print | xargs -r $(RM)

cleani: FORCE
	find Common $(IGNORE) \( -name '*.i' \) \
		-type f -print | xargs -r $(RM)

.PHONY: FORCE

ifneq ($(wildcard $(SRC)/.*.d),)
include $(wildcard $(SRC)/.*.d)
endif
ifneq ($(wildcard $(SRC)/*/.*.d),)
include $(wildcard $(SRC)/*/.*.d)
endif

test: .PHONY
	$(MAKE) -C test test

