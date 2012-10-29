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
#               in build/targets.mk for a list of valid target platforms.
#
#   ENABLE_SDL  If set to "y", the UI is drawn with libSDL.
#
#   OPENGL      "y" means render with OpenGL.
#
#   EYE_CANDY   "n" disables eye candy rendering.
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
#   LTO         "y" enables gcc's link-time optimization flag (experimental,
#               requires gcc 4.5)
#
#   CLANG       "y" to use clang instead of gcc
#
#   ANALYZER    "y" to support the clang analyzer
#
#   LLVM        "y" to compile LLVM bitcode with clang
#
#   LIBCXX      The absolute path of the libc++ svn/git working directory.
#

.DEFAULT_GOAL := all

topdir = .

-include $(topdir)/build/local-config.mk

include $(topdir)/build/make.mk
include $(topdir)/build/bool.mk
include $(topdir)/build/string.mk
include $(topdir)/build/dirs.mk
include $(topdir)/build/verbose.mk
include $(topdir)/build/util.mk
include $(topdir)/build/detect.mk
include $(topdir)/build/targets.mk
include $(topdir)/build/pkgconfig.mk
include $(topdir)/build/resource.mk
include $(topdir)/build/options.mk
include $(topdir)/build/debug.mk
include $(topdir)/build/coverage.mk
include $(topdir)/build/libintl.mk
include $(topdir)/build/boost.mk
include $(topdir)/build/opengl.mk
include $(topdir)/build/freetype.mk
include $(topdir)/build/sdl.mk
include $(topdir)/build/flags.mk
include $(topdir)/build/charset.mk
include $(topdir)/build/warnings.mk
include $(topdir)/build/compile.mk
include $(topdir)/build/link.mk
include $(topdir)/build/android.mk
include $(topdir)/build/llvm.mk
include $(topdir)/build/tools.mk
include $(topdir)/build/version.mk
include $(topdir)/build/osx.mk
include $(topdir)/build/generate.mk
include $(topdir)/build/doxygen.mk
include $(topdir)/build/manual.mk

# Create libraries for zzip, jasper and compatibility stuff
include $(topdir)/build/libstdcxx.mk
include $(topdir)/build/libutil.mk
include $(topdir)/build/libmath.mk
include $(topdir)/build/libgeo.mk
include $(topdir)/build/libos.mk
include $(topdir)/build/libtime.mk
include $(topdir)/build/libprofile.mk
include $(topdir)/build/libnet.mk
include $(topdir)/build/zlib.mk
include $(topdir)/build/zzip.mk
include $(topdir)/build/jasper.mk
include $(topdir)/build/libport.mk
include $(topdir)/build/driver.mk
include $(topdir)/build/io.mk
include $(topdir)/build/libasync.mk
include $(topdir)/build/shapelib.mk
include $(topdir)/build/libwaypoint.mk
include $(topdir)/build/libairspace.mk
include $(topdir)/build/libtask.mk
include $(topdir)/build/libroute.mk
include $(topdir)/build/libcontest.mk
include $(topdir)/build/libglide.mk
include $(topdir)/build/datafield.mk
include $(topdir)/build/screen.mk
include $(topdir)/build/libthread.mk
include $(topdir)/build/form.mk
include $(topdir)/build/libaudio.mk
include $(topdir)/build/harness.mk

include $(topdir)/build/setup.mk
include $(topdir)/build/launch.mk
include $(topdir)/build/vali.mk
include $(topdir)/build/test.mk
include $(topdir)/build/hot.mk

# Load local-config a second time
# to set (override) choices for GXX and friends.
-include $(topdir)/build/local-config.mk

######## output files

ifeq ($(HAVE_POSIX),y)
PROGRAM_NAME = xcsoar
else
PROGRAM_NAME = XCSoar
endif

include $(topdir)/build/dist.mk
include $(topdir)/build/install.mk

######## compiler flags

INCLUDES += -I$(SRC) -I$(ENGINE_SRC_DIR)

####### linker configuration

LDFLAGS = $(TARGET_LDFLAGS) $(FLAGS_PROFILE) $(OPTIMIZE)
LDLIBS = $(TARGET_LDLIBS) $(COVERAGE_LDLIBS)

####### sources

DIALOG_SOURCES = \
	$(SRC)/Form/XMLWidget.cpp \
	$(SRC)/Dialogs/XML.cpp \
	$(SRC)/Dialogs/Inflate.cpp \
	$(SRC)/Dialogs/Message.cpp \
	$(SRC)/Dialogs/ListPicker.cpp \
	$(SRC)/Dialogs/JobDialog.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Dialogs/FileManager.cpp \
	$(SRC)/Dialogs/DeviceListDialog.cpp \
	$(SRC)/Dialogs/PortMonitor.cpp \
	$(SRC)/Dialogs/ManageCAI302Dialog.cpp \
	$(SRC)/Dialogs/CAI302/UnitsEditor.cpp \
	$(SRC)/Dialogs/CAI302/WaypointUploader.cpp \
	$(SRC)/Dialogs/ManageFlarmDialog.cpp \
	$(SRC)/Dialogs/LX/ManageV7Dialog.cpp \
	$(SRC)/Dialogs/LX/V7ConfigWidget.cpp \
	$(SRC)/Dialogs/LX/ManageNanoDialog.cpp \
	$(SRC)/Dialogs/LX/NanoConfigWidget.cpp \
	$(SRC)/Dialogs/Vega/VegaParametersWidget.cpp \
	$(SRC)/Dialogs/Vega/VegaConfigurationDialog.cpp \
	$(SRC)/Dialogs/Vega/VegaDemoDialog.cpp \
	$(SRC)/Dialogs/Vega/VoiceSettingsDialog.cpp \
	$(SRC)/Dialogs/FLARM/ConfigWidget.cpp \
	$(SRC)/Dialogs/MapItemListDialog.cpp \
	$(SRC)/Dialogs/MapItemListSettingsDialog.cpp \
	$(SRC)/Dialogs/MapItemListSettingsPanel.cpp \
	$(SRC)/Dialogs/WindSettingsPanel.cpp \
	$(SRC)/Dialogs/dlgAirspace.cpp \
	$(SRC)/Dialogs/ColorListDialog.cpp \
	$(SRC)/Dialogs/dlgAirspacePatterns.cpp \
	$(SRC)/Dialogs/dlgAirspaceDetails.cpp \
	$(SRC)/Dialogs/AirspaceList.cpp \
	$(SRC)/Dialogs/AirspaceCRendererSettingsDialog.cpp \
	$(SRC)/Dialogs/AirspaceCRendererSettingsPanel.cpp \
	$(SRC)/Dialogs/dlgAirspaceWarnings.cpp \
	$(SRC)/Dialogs/dlgAlternatesList.cpp \
	$(SRC)/Dialogs/dlgFlarmDetailsList.cpp \
	$(SRC)/Dialogs/dlgAnalysis.cpp \
	$(SRC)/Dialogs/dlgBasicSettings.cpp \
	$(SRC)/Dialogs/dlgBrightness.cpp \
	$(SRC)/Dialogs/dlgChecklist.cpp \
	$(SRC)/Dialogs/dlgPlanes.cpp \
	$(SRC)/Dialogs/dlgPlaneDetails.cpp \
	$(SRC)/Dialogs/dlgPlanePolar.cpp \
	$(SRC)/Dialogs/ComboPicker.cpp \
	$(SRC)/Dialogs/FilePicker.cpp \
	$(SRC)/Dialogs/dlgConfiguration.cpp \
	$(SRC)/Dialogs/dlgConfigFonts.cpp \
	$(SRC)/Dialogs/dlgConfigInfoboxes.cpp \
	$(SRC)/Dialogs/dlgConfigWaypoints.cpp \
	$(SRC)/Dialogs/dlgFlarmTrafficDetails.cpp \
	$(SRC)/Dialogs/HelpDialog.cpp \
	$(SRC)/Dialogs/dlgInfoBoxAccess.cpp \
	$(SRC)/Dialogs/ReplayDialog.cpp \
	$(SRC)/Dialogs/dlgSimulatorPrompt.cpp \
	$(SRC)/Dialogs/dlgStartup.cpp \
	\
	$(SRC)/Dialogs/dlgStatus.cpp \
	$(SRC)/Dialogs/StatusPanels/StatusPanel.cpp \
	$(SRC)/Dialogs/StatusPanels/FlightStatusPanel.cpp \
	$(SRC)/Dialogs/StatusPanels/SystemStatusPanel.cpp \
	$(SRC)/Dialogs/StatusPanels/TaskStatusPanel.cpp \
	$(SRC)/Dialogs/StatusPanels/RulesStatusPanel.cpp \
	$(SRC)/Dialogs/StatusPanels/TimesStatusPanel.cpp \
	\
	$(SRC)/Dialogs/Waypoint/WaypointInfoWidget.cpp \
	$(SRC)/Dialogs/Waypoint/WaypointCommandsWidget.cpp \
	\
	$(SRC)/Dialogs/dlgSwitches.cpp \
	\
	$(SRC)/Dialogs/ConfigPanels/AirspaceConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/DevicesConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/ExperimentalConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/GaugesConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/VarioConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/GlideComputerConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/WindConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/InfoBoxesConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/InterfaceConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/LayoutConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/LoggerConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/LoggerInfoConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/MapDisplayConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/PagesConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/PolarConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/RouteConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/SafetyFactorsConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/SiteConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/SymbolsConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/TaskRulesConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/TaskDefaultsConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/TerrainDisplayConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/UnitsConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/TimeConfigPanel.cpp \
	$(SRC)/Dialogs/ConfigPanels/WaypointDisplayConfigPanel.cpp \
	\
	$(SRC)/Dialogs/TaskManager/TaskManagerDialog.cpp \
	$(SRC)/Dialogs/TaskManager/TaskClosePanel.cpp \
	$(SRC)/Dialogs/TaskManager/TaskEditPanel.cpp \
	$(SRC)/Dialogs/TaskManager/TaskPropertiesPanel.cpp \
	$(SRC)/Dialogs/TaskManager/TaskMiscPanel.cpp \
	$(SRC)/Dialogs/TaskManager/TaskActionsPanel.cpp \
	$(SRC)/Dialogs/TaskManager/TaskListPanel.cpp \
	$(SRC)/Dialogs/TaskManager/TaskCalculatorPanel.cpp \
	$(SRC)/Dialogs/dlgTaskOptionalStarts.cpp \
	$(SRC)/Dialogs/dlgTaskPoint.cpp \
	$(SRC)/Dialogs/dlgTaskPointType.cpp \
	$(SRC)/Dialogs/dlgTaskHelpers.cpp \
	$(SRC)/Dialogs/dlgTarget.cpp \
	\
	$(SRC)/Dialogs/dlgTeamCode.cpp \
	$(SRC)/Dialogs/dlgTextEntry.cpp \
	$(SRC)/Dialogs/dlgTextEntry_Keyboard.cpp \
	$(SRC)/Dialogs/dlgWeather.cpp \
	$(SRC)/Dialogs/dlgWaypointDetails.cpp \
	$(SRC)/Dialogs/dlgWaypointEdit.cpp \
	$(SRC)/Dialogs/WaypointList.cpp \
	$(SRC)/Dialogs/WindSettingsDialog.cpp \
	$(SRC)/Dialogs/FontEdit.cpp \
	$(SRC)/Dialogs/dlgCredits.cpp \
	$(SRC)/Dialogs/dlgQuickMenu.cpp \

ifeq ($(HAVE_PCM_PLAYER),y)
DIALOG_SOURCES += $(SRC)/Dialogs/ConfigPanels/AudioVarioConfigPanel.cpp
endif

ifeq ($(HAVE_HTTP),y)
DIALOG_SOURCES += \
	$(SRC)/Dialogs/dlgNOAAList.cpp \
	$(SRC)/Dialogs/dlgNOAADetails.cpp \
	$(SRC)/Dialogs/ConfigPanels/TrackingConfigPanel.cpp
endif

XCSOAR_SOURCES := \
	$(IO_SRC_DIR)/ConfiguredFile.cpp \
	$(IO_SRC_DIR)/DataFile.cpp \
	$(SRC)/Airspace/ProtectedAirspaceWarningManager.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileIGC.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/MapTaskManager.cpp \
	$(SRC)/Task/ProtectedTaskManager.cpp \
	$(SRC)/Task/RoutePlannerGlue.cpp \
	$(SRC)/Task/ProtectedRoutePlanner.cpp \
	$(SRC)/Task/TaskStore.cpp \
	\
	$(SRC)/RadioFrequency.cpp \
	\
	$(SRC)/Engine/Navigation/TraceHistory.cpp \
	$(SRC)/Engine/Navigation/Aircraft.cpp \
	$(SRC)/Engine/Trace/Point.cpp \
	$(SRC)/Engine/Trace/Trace.cpp \
	$(SRC)/Engine/Trace/Vector.cpp \
	$(SRC)/Engine/Util/Gradient.cpp \
	$(SRC)/Renderer/LabelBlock.cpp \
	$(SRC)/Renderer/TextInBox.cpp \
	$(SRC)/Renderer/TraceHistoryRenderer.cpp \
	$(SRC)/Renderer/ThermalBandRenderer.cpp \
	$(SRC)/Renderer/TaskProgressRenderer.cpp \
	\
	$(SRC)/Airspace/AirspaceGlue.cpp \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Airspace/AirspaceVisibility.cpp \
	$(SRC)/Airspace/AirspaceComputerSettings.cpp \
	$(SRC)/Renderer/AirspaceRendererSettings.cpp \
	\
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Operation/PopupOperationEnvironment.cpp \
	$(SRC)/Operation/MessageOperationEnvironment.cpp \
	$(SRC)/Operation/ThreadedOperationEnvironment.cpp \
	$(SRC)/Operation/VerboseOperationEnvironment.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/Atmosphere/CuSonde.cpp \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitor.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorAATTime.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorFinalGlide.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorGlideTerrain.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorLandableReachable.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorStartRules.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorSunset.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorWind.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitors.cpp \
	$(SRC)/Plane/PlaneGlue.cpp \
	$(SRC)/Plane/PlaneFileGlue.cpp \
	$(SRC)/FLARM/FlarmId.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/FLARM/Record.cpp \
	$(SRC)/FLARM/Database.cpp \
	$(SRC)/FLARM/FlarmNet.cpp \
	$(SRC)/FLARM/FlarmNetReader.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/FlarmCalculations.cpp \
	$(SRC)/FLARM/Friends.cpp \
	$(SRC)/FLARM/FriendsGlue.cpp \
	$(SRC)/FLARM/FlarmComputer.cpp \
	$(SRC)/FLARM/Glue.cpp \
	$(SRC)/Computer/CuComputer.cpp \
	$(SRC)/Computer/FlyingComputer.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Computer/ThermalBandComputer.cpp \
	$(SRC)/Computer/WindComputer.cpp \
	$(SRC)/Computer/ContestComputer.cpp \
	$(SRC)/Computer/TraceComputer.cpp \
	$(SRC)/Computer/WarningComputer.cpp \
	$(SRC)/Computer/ThermalLocator.cpp \
	$(SRC)/Computer/ThermalBase.cpp \
	$(SRC)/Computer/LiftDatabaseComputer.cpp \
	$(SRC)/Computer/LogComputer.cpp \
	$(SRC)/Computer/GlideRatioCalculator.cpp \
	$(SRC)/Computer/GlideComputer.cpp \
	$(SRC)/Computer/GlideComputerBlackboard.cpp \
	$(SRC)/Computer/GlideComputerAirData.cpp \
	$(SRC)/Computer/GlideComputerStats.cpp \
	$(SRC)/Computer/GlideComputerRoute.cpp \
	$(SRC)/Computer/GlideComputerTask.cpp \
	$(SRC)/Computer/GlideComputerInterface.cpp \
	$(SRC)/Computer/Events.cpp \
	$(SRC)/BallastDumpManager.cpp \
	$(SRC)/Logger/Settings.cpp \
	$(SRC)/Logger/Logger.cpp \
	$(SRC)/Logger/LoggerFRecord.cpp \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/Logger/LoggerEPE.cpp \
	$(SRC)/Logger/LoggerImpl.cpp \
	$(SRC)/Logger/IGCFileCleanup.cpp \
	$(SRC)/IGC/IGCFix.cpp \
	$(SRC)/IGC/IGCWriter.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/Logger/NMEALogger.cpp \
	$(SRC)/Logger/ExternalLogger.cpp \
	$(SRC)/Logger/FlightLogger.cpp \
	$(SRC)/Logger/GlueFlightLogger.cpp \
	$(SRC)/NMEA/Info.cpp \
	$(SRC)/NMEA/MoreData.cpp \
	$(SRC)/NMEA/Acceleration.cpp \
	$(SRC)/NMEA/Attitude.cpp \
	$(SRC)/NMEA/ExternalSettings.cpp \
	$(SRC)/NMEA/FlyingState.cpp \
	$(SRC)/NMEA/Derived.cpp \
	$(SRC)/NMEA/VarioInfo.cpp \
	$(SRC)/NMEA/ClimbInfo.cpp \
	$(SRC)/NMEA/CirclingInfo.cpp \
	$(SRC)/NMEA/ThermalBand.cpp \
	$(SRC)/NMEA/ThermalLocator.cpp \
	$(SRC)/NMEA/ClimbHistory.cpp \
	$(SRC)/NMEA/InputLine.cpp \
	$(SRC)/NMEA/Checksum.cpp \
	$(SRC)/NMEA/Aircraft.cpp \
	$(SRC)/Replay/Replay.cpp \
	$(SRC)/IGC/IGCParser.cpp \
	$(SRC)/Replay/IgcReplay.cpp \
	$(SRC)/Replay/IgcReplayGlue.cpp \
	$(SRC)/Replay/NmeaReplay.cpp \
	$(SRC)/Replay/NmeaReplayGlue.cpp \
	$(SRC)/Replay/DemoReplay.cpp \
	$(SRC)/Replay/DemoReplayGlue.cpp \
	$(SRC)/Replay/TaskAutoPilot.cpp \
	$(SRC)/Replay/AircraftSim.cpp \
	$(SRC)/TeamCode.cpp \
	$(SRC)/Waypoint/WaypointList.cpp \
	$(SRC)/Waypoint/WaypointListBuilder.cpp \
	$(SRC)/Waypoint/WaypointFilter.cpp \
	$(SRC)/Waypoint/WaypointGlue.cpp \
	$(SRC)/Waypoint/LastUsed.cpp \
	$(SRC)/Waypoint/HomeGlue.cpp \
	$(SRC)/Waypoint/WaypointFileType.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointReaderCompeGPS.cpp \
	$(SRC)/Waypoint/WaypointWriter.cpp \
	$(SRC)/Wind/CirclingWind.cpp \
	$(SRC)/Wind/WindMeasurementList.cpp \
	$(SRC)/Wind/WindStore.cpp \
	$(SRC)/Wind/WindEKF.cpp \
	$(SRC)/Wind/WindEKFGlue.cpp \
	\
	$(SRC)/CrossSection/AirspaceXSRenderer.cpp \
	$(SRC)/CrossSection/TerrainXSRenderer.cpp \
	$(SRC)/CrossSection/CrossSectionRenderer.cpp \
	$(SRC)/CrossSection/CrossSectionWindow.cpp \
	\
	$(SRC)/Gauge/ThermalAssistantRenderer.cpp \
	$(SRC)/Gauge/ThermalAssistantWindow.cpp \
	$(SRC)/Gauge/BigThermalAssistantWindow.cpp \
	$(SRC)/Gauge/FlarmTrafficWindow.cpp \
	$(SRC)/Look/FlarmTrafficLook.cpp \
	$(SRC)/Gauge/GaugeFLARM.cpp \
	$(SRC)/Gauge/GaugeThermalAssistant.cpp \
	$(SRC)/Gauge/VarioSettings.cpp \
	$(SRC)/Gauge/TrafficSettings.cpp \
	$(SRC)/Gauge/GaugeVario.cpp \
	$(SRC)/Gauge/GlueGaugeVario.cpp \
	$(SRC)/Gauge/TaskView.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	\
	$(SRC)/Waypoint/WaypointDetailsReader.cpp \
	$(SRC)/Menu/MenuData.cpp \
	$(SRC)/Menu/MenuBar.cpp \
	$(SRC)/Menu/ButtonLabel.cpp \
	$(SRC)/Dialogs/Dialogs.cpp \
	$(SRC)/Menu/ExpandMacros.cpp \
	$(SRC)/InfoBoxes/Content/Factory.cpp \
	$(SRC)/InfoBoxes/Content/Alternate.cpp \
	$(SRC)/InfoBoxes/Content/Base.cpp \
	$(SRC)/InfoBoxes/Content/Altitude.cpp \
	$(SRC)/InfoBoxes/Content/Direction.cpp \
	$(SRC)/InfoBoxes/Content/Glide.cpp \
	$(SRC)/InfoBoxes/Content/MacCready.cpp \
	$(SRC)/InfoBoxes/Content/Other.cpp \
	$(SRC)/InfoBoxes/Content/Speed.cpp \
	$(SRC)/InfoBoxes/Content/Task.cpp \
	$(SRC)/InfoBoxes/Content/Team.cpp \
	$(SRC)/InfoBoxes/Content/Terrain.cpp \
	$(SRC)/InfoBoxes/Content/Thermal.cpp \
	$(SRC)/InfoBoxes/Content/Time.cpp \
	$(SRC)/InfoBoxes/Content/Trace.cpp \
	$(SRC)/InfoBoxes/Content/Weather.cpp \
	$(SRC)/InfoBoxes/Content/Airspace.cpp \
	$(SRC)/InfoBoxes/Data.cpp \
	$(SRC)/InfoBoxes/Format.cpp \
	$(SRC)/InfoBoxes/Units.cpp \
	$(SRC)/InfoBoxes/InfoBoxSettings.cpp \
	$(SRC)/InfoBoxes/InfoBoxWindow.cpp \
	$(SRC)/InfoBoxes/InfoBoxLayout.cpp \
	$(SRC)/InfoBoxes/InfoBoxManager.cpp \
	$(SRC)/InfoBoxes/Panel/AltitudeInfo.cpp \
	$(SRC)/InfoBoxes/Panel/AltitudeSimulator.cpp \
	$(SRC)/InfoBoxes/Panel/AltitudeSetup.cpp \
	$(SRC)/InfoBoxes/Panel/MacCreadyEdit.cpp \
	$(SRC)/InfoBoxes/Panel/MacCreadySetup.cpp \
	$(SRC)/InfoBoxes/Panel/WindEdit.cpp \
	$(SRC)/InfoBoxes/Panel/WindSetup.cpp \
	$(SRC)/Pan.cpp \
	$(SRC)/Input/InputConfig.cpp \
	$(SRC)/Input/InputDefaults.cpp \
	$(SRC)/Input/InputEvents.cpp \
	$(SRC)/Input/InputEventsActions.cpp \
	$(SRC)/Input/InputEventsDevice.cpp \
	$(SRC)/Input/InputEventsVega.cpp \
	$(SRC)/Input/InputEventsInfoBox.cpp \
	$(SRC)/Input/InputEventsMap.cpp \
	$(SRC)/Input/InputEventsAirspace.cpp \
	$(SRC)/Input/InputEventsTask.cpp \
	$(SRC)/Input/InputEventsSettings.cpp \
	$(SRC)/Input/InputEventsThermalAssistant.cpp \
	$(SRC)/Input/InputEventsTraffic.cpp \
	$(SRC)/Input/InputQueue.cpp \
	$(SRC)/Input/InputLookup.cpp \
	$(SRC)/Input/InputKeys.cpp \
	$(SRC)/Input/InputParser.cpp \
	$(SRC)/PageSettings.cpp \
	$(SRC)/Pages.cpp \
	$(SRC)/StatusMessage.cpp \
	$(SRC)/PopupMessage.cpp \
	$(SRC)/Message.cpp \
	$(SRC)/LogFile.cpp \
	\
	$(SRC)/Geo/Geoid.cpp \
	$(SRC)/MapWindow/MapCanvas.cpp \
	$(SRC)/MapWindow/MapDrawHelper.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(SRC)/Projection/CompareProjection.cpp \
	$(SRC)/Renderer/ChartRenderer.cpp \
	$(SRC)/Renderer/BackgroundRenderer.cpp \
	$(SRC)/Renderer/OZRenderer.cpp \
	$(SRC)/Renderer/TaskPointRenderer.cpp \
	$(SRC)/Renderer/TaskRenderer.cpp \
	$(SRC)/Renderer/AircraftRenderer.cpp \
	$(SRC)/Renderer/AirspaceRenderer.cpp \
	$(SRC)/Renderer/AirspaceListRenderer.cpp \
	$(SRC)/Renderer/AirspacePreviewRenderer.cpp \
	$(SRC)/Renderer/BestCruiseArrowRenderer.cpp \
	$(SRC)/Renderer/CompassRenderer.cpp \
	$(SRC)/Renderer/FinalGlideBarRenderer.cpp \
	$(SRC)/Renderer/MapItemListRenderer.cpp \
	$(SRC)/Renderer/OZPreviewRenderer.cpp \
	$(SRC)/Renderer/TrackLineRenderer.cpp \
	$(SRC)/Renderer/TrafficRenderer.cpp \
	$(SRC)/Renderer/TrailRenderer.cpp \
	$(SRC)/Renderer/UnitSymbolRenderer.cpp \
	$(SRC)/Renderer/WaypointListRenderer.cpp \
	$(SRC)/Renderer/WaypointIconRenderer.cpp \
	$(SRC)/Renderer/WaypointRenderer.cpp \
	$(SRC)/Renderer/WaypointRendererSettings.cpp \
	$(SRC)/Renderer/WindArrowRenderer.cpp \
	$(SRC)/Projection/ChartProjection.cpp \
	$(SRC)/MapWindow/MapItemList.cpp \
	$(SRC)/MapWindow/MapItemListBuilder.cpp \
	$(SRC)/MapWindow/MapWindow.cpp \
	$(SRC)/MapWindow/MapWindowEvents.cpp \
	$(SRC)/MapWindow/MapWindowGlideRange.cpp \
	$(SRC)/MapWindow/MapWindowLabels.cpp \
	$(SRC)/Projection/MapWindowProjection.cpp \
	$(SRC)/MapWindow/MapWindowRender.cpp \
	$(SRC)/MapWindow/MapWindowSymbols.cpp \
	$(SRC)/MapWindow/MapWindowTask.cpp \
	$(SRC)/MapWindow/MapWindowThermal.cpp \
	$(SRC)/MapWindow/MapWindowTimer.cpp \
	$(SRC)/MapWindow/MapWindowTraffic.cpp \
	$(SRC)/MapWindow/MapWindowTrail.cpp \
	$(SRC)/MapWindow/MapWindowWaypoints.cpp \
	$(SRC)/MapWindow/GlueMapWindow.cpp \
	$(SRC)/MapWindow/GlueMapWindowItems.cpp \
	$(SRC)/MapWindow/GlueMapWindowEvents.cpp \
	$(SRC)/MapWindow/GlueMapWindowOverlays.cpp \
	$(SRC)/MapWindow/GlueMapWindowDisplayMode.cpp \
	$(SRC)/MapWindow/TargetMapWindow.cpp \
	$(SRC)/MapWindow/TargetMapWindowEvents.cpp \
	$(SRC)/MapWindow/TargetMapWindowDrag.cpp \
	$(SRC)/GestureManager.cpp \
	$(SRC)/TrackingGestureManager.cpp \
	$(SRC)/DrawThread.cpp \
	\
	$(SRC)/Computer/BasicComputer.cpp \
	$(SRC)/Computer/AutoQNH.cpp \
	\
	$(SRC)/Blackboard/BlackboardListener.cpp \
	$(SRC)/Blackboard/ProxyBlackboardListener.cpp \
	$(SRC)/Blackboard/RateLimitedBlackboardListener.cpp \
	$(SRC)/Blackboard/LiveBlackboard.cpp \
	$(SRC)/Blackboard/InterfaceBlackboard.cpp \
	\
	$(SRC)/Blackboard/DeviceBlackboard.cpp \
	$(SRC)/MapWindow/MapWindowBlackboard.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/UIGlobals.cpp \
	$(SRC)/UIState.cpp \
	$(SRC)/UISettings.cpp \
	$(SRC)/DisplaySettings.cpp \
	$(SRC)/MapSettings.cpp \
	$(SRC)/SystemSettings.cpp \
	$(SRC)/Audio/Settings.cpp \
	$(SRC)/Audio/VarioSettings.cpp \
	$(SRC)/ComputerSettings.cpp \
	$(SRC)/TeamCodeSettings.cpp \
	$(SRC)/MergeThread.cpp \
	$(SRC)/CalculationThread.cpp \
	$(SRC)/DisplayMode.cpp \
	\
	$(SRC)/Topography/TopographyFile.cpp \
	$(SRC)/Topography/TopographyStore.cpp \
	$(SRC)/Topography/TopographyFileRenderer.cpp \
	$(SRC)/Topography/TopographyRenderer.cpp \
	$(SRC)/Topography/TopographyGlue.cpp \
	$(SRC)/Topography/XShape.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterTileCache.cpp \
	$(SRC)/Terrain/RasterTerrain.cpp \
	$(SRC)/Terrain/RasterWeather.cpp \
	$(SRC)/Terrain/HeightMatrix.cpp \
	$(SRC)/Terrain/RasterRenderer.cpp \
	$(SRC)/Terrain/TerrainRenderer.cpp \
	$(SRC)/Terrain/WeatherTerrainRenderer.cpp \
	$(SRC)/Terrain/TerrainSettings.cpp \
	$(SRC)/Markers/Markers.cpp \
	$(SRC)/Markers/ProtectedMarkers.cpp \
	\
	$(SRC)/FlightStatistics.cpp \
	$(SRC)/Renderer/FlightStatisticsRenderer.cpp \
	$(SRC)/Renderer/BarographRenderer.cpp \
	$(SRC)/Renderer/ClimbChartRenderer.cpp \
	$(SRC)/Renderer/GlidePolarRenderer.cpp \
	$(SRC)/Renderer/WindChartRenderer.cpp \
	$(SRC)/Renderer/CuRenderer.cpp \
	\
	$(SRC)/Simulator.cpp \
	$(SRC)/Asset.cpp \
	$(SRC)/Hardware/Display.cpp \
	$(SRC)/Hardware/DisplayGlue.cpp \
	$(SRC)/Hardware/Vibrator.cpp \
	$(SRC)/Language/MOFile.cpp \
	$(SRC)/Language/Language.cpp \
	$(SRC)/Language/LanguageGlue.cpp \
	$(SRC)/LocalPath.cpp \
	$(SRC)/UIActions.cpp \
	$(SRC)/Interface.cpp \
	$(SRC)/ProgressWindow.cpp \
	$(SRC)/ProgressGlue.cpp \
	$(SRC)/LocalTime.cpp \
	$(SRC)/Units/Units.cpp \
	$(SRC)/Units/UnitsGlue.cpp \
	$(SRC)/Units/UnitsStore.cpp \
	$(SRC)/Formatter/Units.cpp \
	$(SRC)/Formatter/UserUnits.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Formatter/GlideRatioFormatter.cpp \
	$(SRC)/Formatter/GeoPointFormatter.cpp \
	$(SRC)/Formatter/ByteSizeFormatter.cpp \
	$(SRC)/Formatter/UserGeoPointFormatter.cpp \
	$(SRC)/Formatter/TimeFormatter.cpp \
	$(SRC)/Formatter/IGCFilenameFormatter.cpp \
	$(SRC)/Formatter/AirspaceFormatter.cpp \
	$(SRC)/Formatter/AirspaceUserUnitsFormatter.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Units/Settings.cpp \
	$(SRC)/Formatter/AngleFormatter.cpp \
	$(SRC)/FLARM/FlarmDetails.cpp \
	$(SRC)/UtilsSettings.cpp \
	$(SRC)/UtilsSystem.cpp \
	$(SRC)/OS/LogError.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/Audio/Sound.cpp \
	$(SRC)/Audio/VegaVoice.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Profile/Profile.cpp \
	$(SRC)/Profile/Earth.cpp \
	$(SRC)/Profile/Screen.cpp \
	$(SRC)/Profile/TrackingProfile.cpp \
	$(SRC)/Profile/SystemProfile.cpp \
	$(SRC)/Profile/ComputerProfile.cpp \
	$(SRC)/Profile/RouteProfile.cpp \
	$(SRC)/Profile/ContestProfile.cpp \
	$(SRC)/Profile/TaskProfile.cpp \
	$(SRC)/Profile/ContestProfile.cpp \
	$(SRC)/Profile/MapProfile.cpp \
	$(SRC)/Profile/PageProfile.cpp \
	$(SRC)/Profile/UIProfile.cpp \
	$(SRC)/Profile/ProfileGlue.cpp \
	$(SRC)/Profile/ProfileKeys.cpp \
	$(SRC)/Profile/FontConfig.cpp \
	$(SRC)/Profile/UnitsConfig.cpp \
	$(SRC)/Profile/DeviceConfig.cpp \
	$(SRC)/Profile/InfoBoxConfig.cpp \
	$(SRC)/Profile/AirspaceConfig.cpp \
	$(SRC)/Profile/TerrainConfig.cpp \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(SRC)/XML/DataNode.cpp \
	$(SRC)/XML/DataNodeXML.cpp \
	\
	$(SRC)/Repository/FileRepository.cpp \
	$(SRC)/Repository/Parser.cpp \
	\
	$(SRC)/Job/Thread.cpp \
	$(SRC)/Job/Async.cpp \
	\
	$(SRC)/RateLimiter.cpp \
	\
	$(SRC)/Tracking/TrackingSettings.cpp \
	\
	$(SRC)/Math/Screen.cpp \
	$(SRC)/Math/SunEphemeris.cpp \
	\
	$(SRC)/Screen/Blank.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/UnitSymbol.cpp \
	$(SRC)/Screen/Ramp.cpp \
	$(SRC)/Screen/TerminalWindow.cpp \
	$(SRC)/ResourceLoader.cpp \
	\
	$(SRC)/Look/Fonts.cpp \
	$(SRC)/Look/CustomFonts.cpp \
	$(SRC)/Look/Look.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/TerminalLook.cpp \
	$(SRC)/Look/VarioLook.cpp \
	$(SRC)/Look/ChartLook.cpp \
	$(SRC)/Look/MapLook.cpp \
	$(SRC)/Look/WindArrowLook.cpp \
	$(SRC)/Look/ThermalBandLook.cpp \
	$(SRC)/Look/TraceHistoryLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(SRC)/Look/TrailLook.cpp \
	$(SRC)/Look/CrossSectionLook.cpp \
	$(SRC)/Look/GestureLook.cpp \
	$(SRC)/Look/HorizonLook.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Look/TrafficLook.cpp \
	$(SRC)/Look/InfoBoxLook.cpp \
	$(SRC)/Look/WaypointLook.cpp \
	$(SRC)/Look/AircraftLook.cpp \
	$(SRC)/Look/MarkerLook.cpp \
	$(SRC)/Look/NOAALook.cpp \
	$(SRC)/Look/FinalGlideBarLook.cpp \
	$(SRC)/Look/IconLook.cpp \
	$(SRC)/Look/UnitsLook.cpp \
	$(SRC)/Look/ThermalAssistantLook.cpp \
	\
	$(SRC)/Polar/PolarGlue.cpp \
	$(SRC)/Polar/PolarFileGlue.cpp \
	$(SRC)/Polar/Shape.cpp \
	$(SRC)/Polar/Polar.cpp \
	$(SRC)/Polar/Parser.cpp \
	$(SRC)/Polar/PolarStore.cpp \
	\
	$(SRC)/Protection.cpp \
	$(SRC)/BatteryTimer.cpp \
	$(SRC)/ProcessTimer.cpp \
	$(SRC)/MainWindow.cpp \
	$(SRC)/Components.cpp \
	\
	$(SRC)/Widgets/TrafficWidget.cpp \
	$(SRC)/Widgets/CrossSectionWidget.cpp \
	$(SRC)/Widgets/BigThermalAssistantWidget.cpp \
	$(SRC)/Widgets/DeviceEditWidget.cpp \
	$(SRC)/Widgets/PolarShapeEditWidget.cpp \
	$(SRC)/Widgets/ObservationZoneEditWidget.cpp \
	$(SRC)/Widgets/CylinderZoneEditWidget.cpp \
	$(SRC)/Widgets/LineSectorZoneEditWidget.cpp \
	$(SRC)/Widgets/SectorZoneEditWidget.cpp \
	\
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Declaration.cpp \
	$(SRC)/Device/Register.cpp \
	$(SRC)/Device/List.cpp \
	$(SRC)/Device/device.cpp \
	$(SRC)/Device/Port/ConfiguredPort.cpp \
	$(SRC)/Device/Descriptor.cpp \
	$(SRC)/Device/Dispatcher.cpp \
	$(SRC)/Device/All.cpp \
	$(SRC)/Device/Parser.cpp \
	$(SRC)/Device/Simulator.cpp \
	$(SRC)/Device/Port/LineSplitter.cpp \
	$(SRC)/Device/Internal.cpp \
	$(DIALOG_SOURCES)

ifneq ($(NO_HORIZON),y)
XCSOAR_SOURCES += \
	$(SRC)/Renderer/HorizonRenderer.cpp
endif

ifeq ($(HAVE_CE),y)
XCSOAR_SOURCES += \
	$(SRC)/Device/Windows/Enumerator.cpp
endif

ifeq ($(TARGET),ANDROID)
XCSOAR_SOURCES += \
	$(SRC)/Java/Global.cpp \
	$(SRC)/Java/String.cpp \
	$(SRC)/Java/File.cpp \
	$(SRC)/Java/InputStream.cpp \
	$(SRC)/Java/URL.cpp \
	$(SRC)/Device/Port/AndroidPort.cpp \
	$(SRC)/Device/Port/AndroidBluetoothPort.cpp \
	$(SRC)/Android/Environment.cpp \
	$(SRC)/Android/InternalSensors.cpp \
	$(SRC)/Android/SoundUtil.cpp \
	$(SRC)/Android/TextUtil.cpp \
	$(SRC)/Android/Timer.cpp \
	$(SRC)/Android/EventBridge.cpp \
	$(SRC)/Android/NativeInputListener.cpp \
	$(SRC)/Android/PortBridge.cpp \
	$(SRC)/Android/BluetoothHelper.cpp \
	$(SRC)/Android/Battery.cpp \
	$(SRC)/Android/DownloadManager.cpp \
	$(SRC)/Android/Vibrator.cpp \
	$(SRC)/Android/Context.cpp \
	$(SRC)/Android/LogCat.cpp \
	$(SRC)/Android/Main.cpp
ifneq ($(IOIOLIB_DIR),)
XCSOAR_SOURCES += \
	$(SRC)/Device/Port/AndroidIOIOUartPort.cpp \
	$(SRC)/Android/NativeBMP085Listener.cpp \
	$(SRC)/Android/BMP085Device.cpp \
	$(SRC)/Android/IOIOHelper.cpp
endif

ifeq ($(DEBUG),y)
XCSOAR_SOURCES += \
	$(SRC)/Android/Assert.cpp
endif

else
XCSOAR_SOURCES += \
	$(SRC)/CommandLine.cpp \
	$(SRC)/Hardware/Battery.cpp \
	$(SRC)/XCSoar.cpp
endif

ifeq ($(TARGET),ALTAIR)
XCSOAR_SOURCES += $(SRC)/Hardware/AltairControl.cpp
endif

ifeq ($(HAVE_HTTP),y)
XCSOAR_SOURCES += \
	$(SRC)/Renderer/NOAAListRenderer.cpp \
	$(SRC)/Weather/NOAAGlue.cpp \
	$(SRC)/Weather/METARParser.cpp \
	$(SRC)/Weather/NOAAFormatter.cpp \
	$(SRC)/Weather/NOAADownloader.cpp \
	$(SRC)/Weather/NOAAStore.cpp \
	$(SRC)/Weather/NOAAUpdater.cpp

XCSOAR_SOURCES += \
	$(SRC)/Tracking/SkyLines/Client.cpp \
	$(SRC)/Tracking/SkyLines/Glue.cpp \
	$(SRC)/Tracking/LiveTrack24.cpp \
	$(SRC)/Tracking/TrackingGlue.cpp
endif

ifeq ($(HAVE_PCM_PLAYER),y)
XCSOAR_SOURCES += $(SRC)/Audio/VarioGlue.cpp
endif

XCSOAR_LDADD = \
	$(RESOURCE_BINARY)

XCSOAR_DEPENDS = GETTEXT PROFILE \
	FORM DATA_FIELD \
	AUDIO SCREEN \
	DRIVER PORT \
	IO ASYNC TASK CONTEST ROUTE GLIDE WAYPOINT AIRSPACE \
	SHAPELIB JASPER ZZIP \
	LIBNET TIME OS THREAD \
	UTIL GEO MATH

XCSOAR_STRIP = y

ifeq ($(TARGET),ANDROID)
$(eval $(call link-shared-library,$(PROGRAM_NAME),XCSOAR))
else
$(eval $(call link-program,$(PROGRAM_NAME),XCSOAR))
endif

include $(topdir)/build/gettext.mk
include $(topdir)/build/cab.mk

OUTPUTS := $(XCSOAR_BIN) $(VALI_XCS_BIN)
OUTPUTS += $(XCSOARSETUP_DLL) $(XCSOARLAUNCH_DLL)

ifeq ($(TARGET),ANDROID)
OUTPUTS += $(ANDROID_BIN)/XCSoar-debug.apk
endif

all: $(OUTPUTS)
everything: $(OUTPUTS) debug build-check build-harness

clean: FORCE
	@$(NQ)echo "cleaning all"
	$(Q)rm -rf build/local-config.mk
	$(Q)rm -rf $(OUT)
	$(RM) $(BUILDTESTS)

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
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/test/src/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/test/src/*.d)
endif
