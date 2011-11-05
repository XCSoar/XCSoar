/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Components.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/DisplayConfig.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterWeather.hpp"
#include "InputEvents.hpp"
#include "Geo/Geoid.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/dlgSimulatorPrompt.hpp"
#include "ButtonLabel.hpp"
#include "Language/LanguageGlue.hpp"
#include "Language/Language.hpp"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "Math/FastMath.h"
#include "UtilsSystem.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "FLARM/FlarmNet.hpp"
#include "SettingsMap.hpp"
#include "Logger/Logger.hpp"
#include "Logger/NMEALogger.hpp"
#include "Waypoint/WaypointDetailsReader.hpp"
#include "Screen/Fonts.hpp"
#include "DeviceBlackboard.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Markers.hpp"
#include "ProtectedMarkers.hpp"
#include "Device/device.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Audio/VarioSound.h"
#include "Screen/Graphics.hpp"
#include "Screen/Busy.hpp"
#include "Polar/PolarGlue.hpp"
#include "Polar/Polar.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"
#include "resource.h"
#include "Computer/GlideComputer.hpp"
#include "Computer/GlideComputerInterface.hpp"
#include "StatusMessage.hpp"
#include "MergeThread.hpp"
#include "CalculationThread.hpp"
#include "Replay/Replay.hpp"
#include "LocalPath.hpp"
#include "IO/FileCache.hpp"
#include "Hardware/AltairControl.hpp"
#include "Hardware/Display.hpp"
#include "Compiler.h"
#include "NMEA/Aircraft.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointGlue.hpp"

#include "Airspace/AirspaceWarningManager.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"

#include "Task/TaskManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Operation.hpp"
#include "Pages.hpp"
#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Plane/PlaneGlue.hpp"
#include "UIState.hpp"
#include "Net/Features.hpp"
#include "Tracking/TrackingGlue.hpp"

#ifndef ENABLE_OPENGL
#include "DrawThread.hpp"
#endif

FileCache *file_cache;
static Markers *marks;
ProtectedMarkers *protected_marks;
TopographyStore *topography;
RasterTerrain *terrain;
RasterWeather RASP;

#ifndef ENABLE_OPENGL
DrawThread *draw_thread;
#endif

DeviceBlackboard *device_blackboard;

MergeThread *merge_thread;
CalculationThread *calculation_thread;

Logger logger;
Replay *replay;

#ifdef HAVE_TRACKING
TrackingGlue *tracking;
#endif

Waypoints way_points;

GlideComputerTaskEvents task_events;

static TaskManager *task_manager;
ProtectedTaskManager *protected_task_manager;

Airspaces airspace_database;

static AirspaceWarningManager *airspace_warning;
ProtectedAirspaceWarningManager *airspace_warnings;

GlideComputer *glide_computer;

#ifdef GNAV
AltairControl altair_control;
#endif

bool
XCSoarInterface::LoadProfile()
{
  if (!dlgStartupShowModal())
    return false;

  Profile::Load();
  Profile::Use();

  return true;
}

static void
LoadDisplayOrientation(VerboseOperationEnvironment &env)
{
  if (!Display::RotateSupported())
    return;

  Display::RotateInitialize();

  Display::orientation orientation = Profile::GetDisplayOrientation();
  if (orientation == Display::ORIENTATION_DEFAULT)
    return;

  if (!Display::Rotate(orientation)) {
    LogStartUp(_T("Display rotation failed"));
    return;
  }

  LogStartUp(_T("Display rotated"));

  XCSoarInterface::main_window.Initialise();

  /* force the progress dialog to update its layout */
  env.UpdateLayout();
}

static void
RestoreDisplayOrientation()
{
  if (!Display::RotateSupported())
    return;

  Display::orientation orientation = Profile::GetDisplayOrientation();
  if (orientation == Display::ORIENTATION_DEFAULT)
    return;

  Display::RotateRestore();
}

void
XCSoarInterface::AfterStartup()
{
  LogStartUp(_T("ProgramStarted = 3"));
  StartupLogFreeRamAndStorage();

  status_messages.Startup(true);

  if (is_simulator()) {
    LogStartUp(_T("GCE_STARTUP_SIMULATOR"));
    InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
  } else {
    LogStartUp(_T("GCE_STARTUP_REAL"));
    InputEvents::processGlideComputer(GCE_STARTUP_REAL);
  }

  OrderedTask *defaultTask = protected_task_manager->TaskCreateDefault(
      &way_points, SettingsComputer().task.task_type_default);
  if (defaultTask) {
    {
      ScopeSuspendAllThreads suspend;
      defaultTask->CheckDuplicateWaypoints(way_points);
      way_points.optimise();
    }
    protected_task_manager->TaskCommit(*defaultTask);
    delete defaultTask;
  }

  task_manager->Resume();

  main_window.full_screen();
  InfoBoxManager::SetDirty();

  TriggerGPSUpdate();

  status_messages.Startup(false);
}

/**
 * "Boots" up XCSoar
 * @param hInstance Instance handle
 * @param lpCmdLine Command line string
 * @return True if bootup successful, False otherwise
 */
bool
XCSoarInterface::Startup()
{
  VerboseOperationEnvironment operation;

  // Set the application title to "XCSoar"
  TCHAR szTitle[] = _T("XCSoar");

  //If "XCSoar" is already running, stop this instance
  if (MainWindow::find(szTitle))
    return false;

  // Creates the main window
  LogStartUp(_T("Create main window"));
  PixelRect WindowSize = SystemWindowSize();
  main_window.set(szTitle,
                  WindowSize.left, WindowSize.top,
                  WindowSize.right, WindowSize.bottom);
  if (!main_window.defined())
    return false;

  main_window.Initialise();

#ifdef SIMULATOR_AVAILABLE
  // prompt for simulator if not set by command line argument "-simulator" or "-fly"
  if (!sim_set_in_cmd_line_flag) {
    DialogLook white_look;
    white_look.Initialise(Fonts::MapBold, Fonts::Map,
                          Fonts::MapBold, Fonts::MapBold);
    white_look.SetBackgroundColor(COLOR_WHITE);
    SetXMLDialogLook(white_look);

    SimulatorPromptResult result = dlgSimulatorPromptShowModal();
    switch (result) {
    case SPR_QUIT:
      return false;

    case SPR_FLY:
      global_simulator_flag = false;
      break;

    case SPR_SIMULATOR:
      global_simulator_flag = true;
      break;
    }
  }
#endif

  SetXMLDialogLook(main_window.look->dialog);

  SetSettingsComputer().SetDefaults();
  SetUISettings().SetDefaults();
  SetUIState().Clear();

  if (!LoadProfile())
    return false;

  operation.SetText(_("Initialising"));

  /* create XCSoarData on the first start */
  CreateDataPath();

  InitAsset();

  LoadDisplayOrientation(operation);

  main_window.InitialiseConfigured();

  TCHAR path[MAX_PATH];
  LocalPath(path, _T("cache"));
  file_cache = new FileCache(path);

  Graphics::InitialiseConfigured(SettingsMap());

  ReadLanguageFile();

  status_messages.LoadFile();
  InputEvents::readFile();

  // Initialize DeviceBlackboard
  device_blackboard = new DeviceBlackboard();

  // Initialize Markers
  marks = new Markers();
  protected_marks = new ProtectedMarkers(*marks);

  // Show the main and map windows
  LogStartUp(_T("Create map window"));
  main_window.show();

#ifdef HAVE_AYGSHELL_DLL
  const AYGShellDLL &ayg = main_window.ayg_shell_dll;
  ayg.SHSetAppKeyWndAssoc(VK_APP1, main_window);
  ayg.SHSetAppKeyWndAssoc(VK_APP2, main_window);
  ayg.SHSetAppKeyWndAssoc(VK_APP3, main_window);
  ayg.SHSetAppKeyWndAssoc(VK_APP4, main_window);
  // Typical Record Button
  //	Why you can't always get this to work
  //	http://forums.devbuzz.com/m_1185/mpage_1/key_/tm.htm
  //	To do with the fact it is a global hotkey, but you can with code above
  //	Also APPA is record key on some systems
  ayg.SHSetAppKeyWndAssoc(VK_APP5, main_window);
  ayg.SHSetAppKeyWndAssoc(VK_APP6, main_window);
#endif

  // Initialize main blackboard data
  task_manager = new TaskManager(task_events, way_points);
  task_manager->Reset();

  protected_task_manager =
    new ProtectedTaskManager(*task_manager,
                             XCSoarInterface::SettingsComputer().task,
                             task_events);

  airspace_warning = new AirspaceWarningManager(airspace_database,
                                                *task_manager);
  airspace_warnings = new ProtectedAirspaceWarningManager(*airspace_warning);

  // Read the terrain file
  operation.SetText(_("Loading Terrain File..."));
  LogStartUp(_T("OpenTerrain"));
  terrain = RasterTerrain::OpenTerrain(file_cache, operation);

  glide_computer = new GlideComputer(way_points, airspace_database,
                                     *protected_task_manager,
                                     *airspace_warnings,
                                     task_events);
  glide_computer->ReadSettingsComputer(SettingsComputer());
  glide_computer->set_terrain(terrain);
  glide_computer->SetLogger(&logger);
  glide_computer->Initialise();

  replay = new Replay(&logger, *protected_task_manager);

  // Load the EGM96 geoid data
  OpenGeoid();

  GlidePolar &gp = SetSettingsComputer().glide_polar_task;
  gp = GlidePolar(fixed_zero);
  gp.SetMC(SettingsComputer().task.safety_mc);
  PlaneGlue::FromProfile(SetSettingsComputer().plane);
  PlaneGlue::Synchronize(SettingsComputer().plane, SetSettingsComputer(), gp);
  task_manager->SetGlidePolar(gp);

  // Read the topography file(s)
  topography = new TopographyStore();
  LoadConfiguredTopography(*topography, operation);

  // Read the waypoint files
  WaypointGlue::LoadWaypoints(way_points, terrain, operation);

  // Read and parse the airfield info file
  WaypointDetails::ReadFileFromProfile(way_points, operation);

  // Set the home waypoint
  WaypointGlue::SetHome(way_points, terrain, SetSettingsComputer(),
                        false);

  // ReSynchronise the blackboards here since SetHome touches them
  device_blackboard->Merge();
  ReadBlackboardBasic(device_blackboard->Basic());

  // Scan for weather forecast
  LogStartUp(_T("RASP load"));
  RASP.ScanAll(Basic().location, operation);

  // Reads the airspace files
  ReadAirspace(airspace_database, terrain, SettingsComputer().pressure,
               operation);

  const AircraftState aircraft_state =
    ToAircraftState(device_blackboard->Basic(),
                    device_blackboard->Calculated());
  airspace_warning->reset(aircraft_state);
  airspace_warning->set_config(CommonInterface::SettingsComputer().airspace.warnings);

  // Read the FLARM details file
  FlarmDetails::Load();

#ifdef HAVE_NET
  noaa_store = new NOAAStore();
  noaa_store->LoadFromProfile();
#endif

#ifndef DISABLEAUDIOVARIO
  /*
  VarioSound_Init();
  VarioSound_EnableSound(EnableSoundVario);
  VarioSound_SetVdead(SoundDeadband);
  VarioSound_SetV(0);
  VarioSound_SetSoundVolume(SoundVolume);
  */
#endif

  // Start the device thread(s)
  operation.SetText(_("Starting devices"));
  devStartup();

/*
  -- Reset polar in case devices need the data
  LogStartUp(_T("GlidePolar::UpdatePolar"));
  GlidePolar::UpdatePolar(true, SettingsComputer());

  This should be done inside devStartup if it is really required
*/

  operation.SetText(_("Initialising display"));

  GlueMapWindow *map_window = main_window.map;
  if (map_window != NULL) {
    map_window->set_way_points(&way_points);
    map_window->set_task(protected_task_manager);
    map_window->SetRoutePlanner(&glide_computer->GetProtectedRoutePlanner());
    map_window->SetGlideComputer(glide_computer);
    map_window->set_airspaces(&airspace_database, airspace_warnings);

    map_window->set_topography(topography);
    map_window->set_terrain(terrain);
    map_window->set_weather(&RASP);
    map_window->set_marks(protected_marks);
    map_window->SetLogger(&logger);
  }

  // Finally ready to go.. all structures must be present before this.

  // Create the drawing thread
#ifndef ENABLE_OPENGL
  LogStartUp(_T("CreateDrawingThread"));
  draw_thread = new DrawThread(*map_window);
  draw_thread->Start(true);
#endif

  // Show the infoboxes
  LogStartUp(_T("ShowInfoBoxes"));
  InfoBoxManager::Show();

  // Create the calculation thread
  LogStartUp(_T("CreateCalculationThread"));
  CreateCalculationThread();

  // Find unique ID of this PDA
  ReadAssetNumber();

  LogStartUp(_T("ProgramStarted"));

  // Give focus to the map
  main_window.SetDefaultFocus();

  Pages::Initialise(GetUISettings().pages);

  // Start calculation thread
  merge_thread->Start();
  calculation_thread->Start();

#ifdef HAVE_TRACKING
  tracking = new TrackingGlue();
  tracking->SetSettings(SettingsComputer().tracking);
#endif

  globalRunningEvent.Signal();

  AfterStartup();

  operation.Hide();

#ifndef ENABLE_OPENGL
  draw_thread->Resume();
#endif

  return true;
}

void
XCSoarInterface::Shutdown(void)
{
  VerboseOperationEnvironment operation;
  gcc_unused ScopeBusyIndicator busy;

  // Show progress dialog
  operation.SetText(_("Shutdown, please wait..."));

  // Log shutdown information
  LogStartUp(_T("Entering shutdown..."));
  StartupLogFreeRamAndStorage();

  // Turn off all displays
  globalRunningEvent.Reset();

#ifdef HAVE_TRACKING
  if (tracking != NULL)
    tracking->StopAsync();
#endif

  // Stop logger and save igc file
  operation.SetText(_("Shutdown, saving logs..."));
  logger.GUIStopLogger(Basic(), true);

  // Save settings to profile
  operation.SetText(_("Shutdown, saving profile..."));
  Profile::Save();

  // Stop sound
  LogStartUp(_T("SaveSoundSettings"));
  Profile::SetSoundSettings();

#ifndef DISABLEAUDIOVARIO
  //  VarioSound_EnableSound(false);
  //  VarioSound_Close();
#endif

  operation.SetText(_("Shutdown, please wait..."));

  // Stop threads
  LogStartUp(_T("Stop threads"));
#ifndef ENABLE_OPENGL
  draw_thread->BeginStop();
#endif
  calculation_thread->BeginStop();
  merge_thread->BeginStop();

  // Wait for the calculations thread to finish
  LogStartUp(_T("Waiting for calculation thread"));

  merge_thread->Join();
  delete merge_thread;
  merge_thread = NULL;

  calculation_thread->Join();
  delete calculation_thread;
  calculation_thread = NULL;

  //  Wait for the drawing thread to finish
#ifndef ENABLE_OPENGL
  LogStartUp(_T("Waiting for draw thread"));

  draw_thread->Join();
  delete draw_thread;
#endif

  LogStartUp(_T("delete MapWindow"));
  main_window.Deinitialise();

  // Save the task for the next time
  operation.SetText(_("Shutdown, saving task..."));

  LogStartUp(_T("Save default task"));
  protected_task_manager->TaskSaveDefault();

  // Clear waypoint database
  LogStartUp(_T("Close waypoints"));
  way_points.clear();

  operation.SetText(_("Shutdown, please wait..."));

  // Clear weather database
  LogStartUp(_T("CloseRASP"));
  RASP.Close();

  // Clear terrain database
  LogStartUp(_T("CloseTerrain"));

  delete terrain;

  LogStartUp(_T("CloseTopography"));
  delete topography;

  delete protected_marks;
  delete marks;

  // Close any device connections
  devShutdown();

  NMEALogger::Shutdown();

  delete replay;

  delete device_blackboard;
  device_blackboard = NULL;

  protected_task_manager->SetRoutePlanner(NULL);

  delete protected_task_manager;
  delete task_manager;

#ifdef HAVE_NET
  delete noaa_store;
#endif

#ifdef HAVE_TRACKING
  if (tracking != NULL) {
    tracking->WaitStopped();
    delete tracking;
  }
#endif

  // Close the progress dialog
  LogStartUp(_T("Close Progress Dialog"));
  operation.Hide();

  // Clear the EGM96 database
  CloseGeoid();

  delete glide_computer;

  // Clear airspace database
  LogStartUp(_T("Close airspace"));
  airspace_warnings->clear();
  airspace_database.clear();

  delete airspace_warnings;
  delete airspace_warning;

  // Destroy FlarmNet records
  FlarmNet::Destroy();

  delete file_cache;

  LogStartUp(_T("Close Windows - main "));
  main_window.reset();

  CloseLanguageFile();

  RestoreDisplayOrientation();

  StartupLogFreeRamAndStorage();

  LogStartUp(_T("Finished shutdown"));
}
