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
#   DEBUG       If set to "y", the debugging version of XCSoar is built
#               (default is "y")
#
#   WERROR      Make all compiler warnings fatal (default is $DEBUG)
#
#   V           Verbosity; 1 is the default, and prints terse information.
#               0 means quiet, and 2 prints the full compiler commands.
#
#   FIXED       "y" means use fixed point math (for FPU-less platforms)
#

.DEFAULT_GOAL := all

topdir = .

include $(topdir)/build/bool.mk
include $(topdir)/build/common.mk
include $(topdir)/build/targets.mk
include $(topdir)/build/debug.mk
include $(topdir)/build/coverage.mk
include $(topdir)/build/options.mk
include $(topdir)/build/sdl.mk
include $(topdir)/build/gconf.mk

CPPFLAGS += -DFLARM_AVERAGE -DDRAWLOAD

ifeq ($(HAVE_WIN32),n)
ifneq ($(TARGET),CYGWIN)
  CPPFLAGS += -DHAVE_BOOST -DHAVE_VASPRINTF
else
  CPPFLAGS += -D__STDC_VERSION__=199901L
endif
endif

include $(topdir)/build/flags.mk
include $(topdir)/build/warnings.mk
include $(topdir)/build/compile.mk
include $(topdir)/build/tools.mk
include $(topdir)/build/resource.mk
include $(topdir)/build/generate.mk
include $(topdir)/build/doco.mk

# Create libraries for zzip, jasper and compatibility stuff
include $(topdir)/build/zzip.mk
include $(topdir)/build/jasper.mk
include $(topdir)/build/compat.mk
include $(topdir)/build/shapelib.mk
include $(topdir)/build/task.mk
include $(topdir)/build/datafield.mk
include $(topdir)/build/screen.mk
include $(topdir)/build/form.mk
include $(topdir)/build/harness.mk

include $(topdir)/build/test.mk

######## output files

OUTPUTS := $(TARGET_BIN_DIR)/XCSoar$(TARGET_EXEEXT)

include $(topdir)/build/dist.mk

######## compiler flags

INCLUDES += -I$(SRC) -I$(ENGINE_SRC_DIR)
CPPFLAGS += $(GCONF_CPPFLAGS)

####### linker configuration

LDFLAGS = $(TARGET_LDFLAGS) $(FLAGS_PROFILE)
LDLIBS = $(TARGET_LDLIBS) $(GCONF_LDLIBS)

####### sources

DRIVER_SOURCES = \
	$(SRC)/Device/Driver/AltairPro.cpp \
	$(SRC)/Device/Driver/BorgeltB50.cpp \
	$(SRC)/Device/Driver/CAI302.cpp \
	$(SRC)/Device/Driver/CaiGpsNav.cpp \
	$(SRC)/Device/Driver/Condor.cpp \
	$(SRC)/Device/Driver/EW.cpp \
	$(SRC)/Device/Driver/EWMicroRecorder.cpp \
	$(SRC)/Device/Driver/FlymasterF1.cpp \
	$(SRC)/Device/Driver/Generic.cpp \
	$(SRC)/Device/Driver/LX.cpp \
	$(SRC)/Device/Driver/NmeaOut.cpp \
	$(SRC)/Device/Driver/PosiGraph.cpp \
	$(SRC)/Device/Driver/Vega.cpp \
	$(SRC)/Device/Driver/Volkslogger.cpp \
	$(SRC)/Device/Driver/XCOM760.cpp \
	$(SRC)/Device/Driver/Zander.cpp

DIALOG_SOURCES = \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/Message.cpp \
	$(SRC)/Dialogs/dlgAirspace.cpp \
	$(SRC)/Dialogs/dlgAirspaceColours.cpp \
	$(SRC)/Dialogs/dlgAirspacePatterns.cpp \
	$(SRC)/Dialogs/dlgAirspaceDetails.cpp \
	$(SRC)/Dialogs/dlgAirspaceSelect.cpp \
	$(SRC)/Dialogs/dlgAirspaceWarning.cpp \
	$(SRC)/Dialogs/dlgAnalysis.cpp \
	$(SRC)/Dialogs/dlgBasicSettings.cpp \
	$(SRC)/Dialogs/dlgBrightness.cpp \
	$(SRC)/Dialogs/dlgChecklist.cpp \
	$(SRC)/Dialogs/dlgComboPicker.cpp \
	$(SRC)/Dialogs/dlgConfiguration.cpp \
	$(SRC)/Dialogs/dlgConfiguration2.cpp \
	$(SRC)/Dialogs/dlgConfigurationVario.cpp \
	$(SRC)/Dialogs/dlgFlarmTraffic.cpp \
	$(SRC)/Dialogs/dlgHelp.cpp \
	$(SRC)/Dialogs/dlgLoggerReplay.cpp \
	$(SRC)/Dialogs/dlgStartup.cpp \
	$(SRC)/Dialogs/dlgStatus.cpp \
	$(SRC)/Dialogs/dlgSwitches.cpp \
	$(SRC)/Dialogs/dlgTaskCalculator.cpp \
	$(SRC)/Dialogs/dlgTeamCode.cpp \
	$(SRC)/Dialogs/dlgTextEntry.cpp \
	$(SRC)/Dialogs/dlgTextEntry_Keyboard.cpp \
	$(SRC)/Dialogs/dlgHelpers.cpp \
	$(SRC)/Dialogs/dlgVegaDemo.cpp \
	$(SRC)/Dialogs/dlgVoice.cpp \
	$(SRC)/Dialogs/dlgWeather.cpp \
	$(SRC)/Dialogs/dlgWaypointOutOfTerrain.cpp \
	$(SRC)/Dialogs/dlgWayPointDetails.cpp \
	$(SRC)/Dialogs/dlgWaypointEdit.cpp \
	$(SRC)/Dialogs/dlgWayPointSelect.cpp \
	$(SRC)/Dialogs/dlgWindSettings.cpp \
	$(SRC)/Dialogs/dlgFontEdit.cpp \

VOLKSLOGGER_SOURCES = \
	$(SRC)/Device/Volkslogger/dbbconv.cpp \
	$(SRC)/Device/Volkslogger/grecord.cpp \
	$(SRC)/Device/Volkslogger/vlapi2.cpp \
	$(SRC)/Device/Volkslogger/vlapihlp.cpp \
	$(SRC)/Device/Volkslogger/vlapisys_win.cpp \
	$(SRC)/Device/Volkslogger/vlconv.cpp

ifeq ($(HAVE_MSVCRT),n)
VOLKSLOGGER_SOURCES += $(SRC)/Device/Volkslogger/vlutils.cpp
endif


XCSOAR_SOURCES := \
	$(SRC)/Globals.cpp \
	\
	$(SRC)/Poco/RWLock.cpp \
	\
	$(SRC)/AirspaceGlue.cpp \
	$(SRC)/AirspaceParser.cpp \
	$(SRC)/AirspaceVisibility.cpp \
	\
	$(SRC)/Atmosphere.cpp \
	$(SRC)/ClimbAverageCalculator.cpp \
	$(SRC)/ConditionMonitor.cpp \
	$(SRC)/Calibration.cpp \
	$(SRC)/FLARM/FLARMNet.cpp \
	$(SRC)/FlarmCalculations.cpp \
	$(SRC)/GlideComputer.cpp \
	$(SRC)/GlideComputerBlackboard.cpp \
	$(SRC)/GlideComputerAirData.cpp \
	$(SRC)/GlideComputerInterface.cpp \
	$(SRC)/GlideComputerStats.cpp \
	$(SRC)/GlideComputerTask.cpp \
	$(SRC)/GlideRatio.cpp \
	$(SRC)/GlideTerrain.cpp \
	$(SRC)/Logger.cpp \
	$(SRC)/LoggerFRecord.cpp \
	$(SRC)/LoggerImpl.cpp \
	$(SRC)/LoggerSign.cpp \
	$(SRC)/ReplayLogger.cpp \
	$(SRC)/ReplayLoggerGlue.cpp \
	$(SRC)/TeamCodeCalculation.cpp \
	$(SRC)/ThermalLocator.cpp \
	$(SRC)/WayPointParser.cpp \
	$(SRC)/WindAnalyser.cpp \
	$(SRC)/WindMeasurementList.cpp \
	$(SRC)/WindStore.cpp \
	$(SRC)/WindZigZag.cpp \
	\
	$(SRC)/Gauge/GaugeCDI.cpp \
	$(SRC)/Gauge/GaugeFLARM.cpp \
	$(SRC)/Gauge/GaugeVario.cpp \
	\
	$(SRC)/AirfieldDetails.cpp \
	$(SRC)/MenuData.cpp \
	$(SRC)/MenuBar.cpp \
	$(SRC)/ButtonLabel.cpp \
	$(SRC)/Dialogs.cpp \
	$(SRC)/ExpandMacros.cpp \
	$(SRC)/Formatter/Base.cpp \
	$(SRC)/Formatter/TeamCode.cpp \
	$(SRC)/Formatter/WayPoint.cpp \
	$(SRC)/Formatter/LowWarning.cpp \
	$(SRC)/Formatter/Time.cpp \
	$(SRC)/InfoBox.cpp \
	$(SRC)/InfoBoxLayout.cpp \
	$(SRC)/InfoBoxEvents.cpp \
	$(SRC)/InfoBoxManager.cpp \
	$(SRC)/InputEvents.cpp \
	$(SRC)/InputEventsActions.cpp \
	$(SRC)/StatusMessage.cpp \
	$(SRC)/PopupMessage.cpp \
	$(SRC)/LogFile.cpp \
	\
	$(SRC)/MapDrawHelper.cpp \
	$(SRC)/MapWindow.cpp \
	$(SRC)/MapWindowAirspace.cpp \
	$(SRC)/MapWindowEvents.cpp \
	$(SRC)/MapWindowGlideRange.cpp \
	$(SRC)/MapWindowLabels.cpp \
	$(SRC)/MapWindowProjection.cpp \
	$(SRC)/MapWindowRender.cpp \
	$(SRC)/MapWindowScale.cpp \
	$(SRC)/MapWindowSymbols.cpp \
	$(SRC)/MapWindowTask.cpp \
	$(SRC)/MapWindowTarget.cpp \
	$(SRC)/MapWindowThermal.cpp \
	$(SRC)/MapWindowTimer.cpp \
	$(SRC)/MapWindowTraffic.cpp \
	$(SRC)/MapWindowTrail.cpp \
	$(SRC)/MapWindowWaypoints.cpp \
	$(SRC)/DrawThread.cpp \
	\
	$(SRC)/DeviceBlackboard.cpp \
	$(SRC)/InstrumentBlackboard.cpp \
	$(SRC)/InterfaceBlackboard.cpp \
	$(SRC)/MapProjectionBlackboard.cpp \
	$(SRC)/MapWindowBlackboard.cpp \
	$(SRC)/SettingsMapBlackboard.cpp \
	$(SRC)/SettingsComputerBlackboard.cpp \
	$(SRC)/CalculationThread.cpp \
	$(SRC)/InstrumentThread.cpp \
	\
	$(SRC)/Topology.cpp \
	$(SRC)/TopologyStore.cpp \
	$(SRC)/RasterMap.cpp \
	$(SRC)/RasterMapCache.cpp \
	$(SRC)/RasterMapJPG2000.cpp \
	$(SRC)/RasterMapRaw.cpp \
	$(SRC)/RasterTerrain.cpp \
	$(SRC)/RasterWeather.cpp \
	$(SRC)/TerrainRenderer.cpp \
	$(SRC)/Marks.cpp \
	\
	$(SRC)/Persist.cpp \
	$(SRC)/FlightStatistics.cpp \
	\
	$(SRC)/Simulator.cpp \
	$(SRC)/Asset.cpp \
	$(SRC)/Appearance.cpp \
	$(SRC)/Battery.c 		\
	$(SRC)/Language.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/Interface.cpp \
	$(SRC)/LocalTime.cpp \
	$(SRC)/Units.cpp \
	$(SRC)/StringUtil.cpp \
	$(SRC)/UtilsFLARM.cpp \
	$(SRC)/UtilsFont.cpp \
	$(SRC)/UtilsProfile.cpp \
	$(SRC)/UtilsSettings.cpp \
	$(SRC)/UtilsSystem.cpp \
	$(SRC)/UtilsText.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/Audio/Sound.cpp \
	$(SRC)/Audio/VegaVoice.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Compatibility/string.c 	\
	$(SRC)/Registry.cpp \
	$(SRC)/Profile.cpp \
	$(SRC)/xmlParser.cpp \
	$(SRC)/Thread/Thread.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Thread/Debug.cpp \
	\
	$(SRC)/Math/FastRotation.cpp \
	$(SRC)/Math/leastsqs.cpp \
	$(SRC)/Math/LowPassFilter.cpp \
	$(SRC)/Math/NavFunctions.cpp \
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	\
	$(SRC)/Screen/Animation.cpp \
	$(SRC)/Screen/Blank.cpp \
	$(SRC)/Screen/Chart.cpp \
	$(SRC)/Screen/Fonts.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/Screen/Graphics.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/LabelBlock.cpp \
	$(SRC)/Screen/ProgressWindow.cpp \
	\
	$(SRC)/Polar/Polar.cpp \
	$(SRC)/Polar/Loader.cpp \
	$(SRC)/Polar/WinPilot.cpp \
	$(SRC)/Polar/BuiltIn.cpp \
	$(SRC)/Polar/Historical.cpp \
	\
	$(SRC)/Blackboard.cpp \
	$(SRC)/Protection.cpp \
	$(SRC)/ProcessTimer.cpp \
	$(SRC)/MainWindow.cpp \
	$(SRC)/Components.cpp \
	$(SRC)/XCSoar.cpp \
	\
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/List.cpp \
	$(SRC)/Device/device.cpp \
	$(SRC)/Device/Descriptor.cpp \
	$(SRC)/Device/All.cpp \
	$(SRC)/Device/Geoid.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Port.cpp \
	$(SRC)/Device/FLARM.cpp \
	$(SRC)/Device/Internal.cpp \
	$(DRIVER_SOURCES) \
	$(VOLKSLOGGER_SOURCES) \
	$(DIALOG_SOURCES)

#	$(SRC)/VarioSound.cpp \
#	$(SRC)/WaveThread.cpp \


XCSOAR_OBJS = $(call SRC_TO_OBJ,$(XCSOAR_SOURCES))
XCSOAR_LDADD = \
	$(DATA_FIELD_LIBS) \
	$(FORM_LIBS) \
	$(SCREEN_LIBS) \
	$(ENGINE_LIBS) \
	$(SHAPELIB_LIBS) \
	$(JASPER_LIBS) \
	$(ZZIP_LIBS) \
	$(COMPAT_LIBS) \
	$(RESOURCE_BINARY)

XCSOARSETUP_SOURCES = \
	$(SRC)/XCSoarSetup.cpp
XCSOARSETUP_OBJS = $(call SRC_TO_OBJ,$(XCSOARSETUP_SOURCES))

XCSOARLAUNCH_SOURCES = \
	$(SRC)/XCSoarLaunch.c
XCSOARLAUNCH_OBJS = $(call SRC_TO_OBJ,$(XCSOARLAUNCH_SOURCES))

all: all-$(TARGET)

# if no TARGET is set, build all targets
all-: $(addprefix call-,$(DEFAULT_TARGETS))
call-%:
	$(MAKE) TARGET=$(patsubst call-%,%,$@) DEBUG=$(DEBUG) V=$(V)

$(addprefix all-,$(TARGETS)): all-%: $(OUTPUTS)

####### products

SYNCE_PCP = synce-pcp
SYNCE_PRM = synce-prm

install: XCSoar.exe
	@echo Copying to device...
	-$(SYNCE_PRM) ':/Program Files/XCSoar/XCSoar.exe'
	$(SYNCE_PCP) XCSoar.exe ':/Program Files/XCSoar/XCSoar.exe'

CABWIZ = wine 'c:\cabwiz\cabwiz.exe'

$(TARGET_BIN_DIR)/XCSoar.inf: build/cab.inf
	$(Q)cp $< $@

$(TARGET_BIN_DIR)/XCSoar.$(PCPU).CAB: $(TARGET_BIN_DIR)/XCSoar.inf $(OUTPUTS) $(TARGET_BIN_DIR)/XCSoarSetup.dll $(TARGET_BIN_DIR)/XCSoarLaunch.dll
	@$(NQ)echo "  CAB     $@"
	$(Q)cd $(TARGET_BIN_DIR) && $(CABWIZ) XCSoar.inf /cpu $(PCPU)

$(TARGET_BIN_DIR)/XCSoar-$(TARGET).cab: $(TARGET_BIN_DIR)/XCSoar.$(PCPU).CAB
	$(Q)mv $< $@

cab: $(TARGET_BIN_DIR)/XCSoar-$(TARGET).cab

#	wine ezsetup.exe -l english -i XCSoar$(TARGET).ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-$(TARGET).exe

ifneq ($(NOSTRIP_SUFFIX),)
$(TARGET_BIN_DIR)/XCSoar$(TARGET_EXEEXT): $(TARGET_BIN_DIR)/XCSoar$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT)
	@$(NQ)echo "  STRIP   $@"
	$(Q)$(STRIP) $< -o $@
	$(Q)$(SIZE) $@
endif

$(TARGET_BIN_DIR)/XCSoar$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT): $(XCSOAR_OBJS) $(XCSOAR_LDADD) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(SCREEN_LDLIBS) -o $@

$(XCSOARSETUP_OBJS) $(XCSOARLAUNCH_OBJS): CFLAGS += -Wno-missing-declarations -Wno-missing-prototypes

$(TARGET_OUTPUT_DIR)/XCSoarSetup.e: $(SRC)/XcSoarSetup.def $(XCSOARSETUP_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	$(Q)$(DLLTOOL) -e $@ -d $^

$(TARGET_BIN_DIR)/XCSoarSetup.dll: TARGET_LDLIBS =
$(TARGET_BIN_DIR)/XCSoarSetup.dll: $(TARGET_OUTPUT_DIR)/XCSoarSetup.e $(XCSOARSETUP_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  DLL     $@"
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
# JMW not tested yet, probably need to use dlltool?

$(TARGET_OUTPUT_DIR)/XCSoarLaunch.e: $(SRC)/XCSoarLaunch.def $(XCSOARLAUNCH_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	$(Q)$(DLLTOOL) -e $@ -d $^

$(TARGET_OUTPUT_DIR)/XCSoarLaunch.rsc: $(SRC)/XCSoarLaunch.rc | $(TARGET_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

$(TARGET_BIN_DIR)/XCSoarLaunch.dll: TARGET_LDLIBS = -laygshell
$(TARGET_BIN_DIR)/XCSoarLaunch.dll: $(TARGET_OUTPUT_DIR)/XCSoarLaunch.e $(XCSOARLAUNCH_OBJS) $(TARGET_OUTPUT_DIR)/XCSoarLaunch.rsc | $(TARGET_BIN_DIR)/dirstamp
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

IGNORE	:= \( -name .svn -o -name CVS -o -name .git \) -prune -o

clean: cleancov FORCE
	@$(NQ)echo "cleaning all"
	$(Q)rm -rf output
	$(RM) $(BUILDTESTS)

cleancov: FORCE
	@$(NQ)echo "cleaning cov"
	$(Q)find ./ $(IGNORE) \( \
		   -name '*.bb' \
		-o -name '*.bbg' \
		-o -name '*.gcda' \
		-o -name '*.gcda.info' \
		-o -name '*.gcno' \
		-o -name '*.gcno.info' \
	\) -type f -print | xargs -r $(RM)

.PHONY: FORCE

ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*.d)
endif

