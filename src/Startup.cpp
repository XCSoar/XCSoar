/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Startup.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Input/InputEvents.hpp"
#include "Input/InputQueue.hpp"
#include "Geo/Geoid.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/dlgSimulatorPrompt.hpp"
#include "Language/LanguageGlue.hpp"
#include "Language/Language.hpp"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "Math/FastMath.h"
#include "UtilsSystem.hpp"
#include "FLARM/Glue.hpp"
#include "MapSettings.hpp"
#include "Logger/Logger.hpp"
#include "Logger/NMEALogger.hpp"
#include "Logger/GlueFlightLogger.hpp"
#include "Waypoint/WaypointDetailsReader.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Markers/Markers.hpp"
#include "Markers/ProtectedMarkers.hpp"
#include "Device/device.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Audio/VarioGlue.hpp"
#include "Screen/Busy.hpp"
#include "Polar/PolarGlue.hpp"
#include "Polar/Polar.hpp"
#include "CommandLine.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"
#include "Look/GlobalFonts.hpp"
#include "resource.h"
#include "Computer/GlideComputer.hpp"
#include "Computer/GlideComputerInterface.hpp"
#include "Computer/Events.hpp"
#include "Monitor/AllMonitors.hpp"
#include "StatusMessage.hpp"
#include "MergeThread.hpp"
#include "CalculationThread.hpp"
#include "Replay/Replay.hpp"
#include "LocalPath.hpp"
#include "IO/FileCache.hpp"
#include "Net/DownloadManager.hpp"
#include "Hardware/AltairControl.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "Hardware/DisplayGlue.hpp"
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
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Operation/VerboseOperationEnvironment.hpp"
#include "PageActions.hpp"
#include "Weather/Features.hpp"
#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Plane/PlaneGlue.hpp"
#include "UIState.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "Thread/Debug.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Globals.hpp"
#else
#include "DrawThread.hpp"
#endif

static Markers *marks;
static TaskManager *task_manager;
static GlideComputerEvents *glide_computer_events;
static AllMonitors *all_monitors;
static GlideComputerTaskEvents *task_events;

static bool
LoadProfile()
{
  if (StringIsEmpty(Profile::GetPath()) &&
      !dlgStartupShowModal())
    return false;

  Profile::Load();
  Profile::Use();

  Units::SetConfig(CommonInterface::GetUISettings().units);
  SetUserCoordinateFormat(CommonInterface::GetUISettings().coordinate_format);

#ifdef HAVE_MODEL_TYPE
  global_model_type = CommonInterface::GetSystemSettings().model_type;
#endif

  return true;
}

static void
AfterStartup()
{
  StartupLogFreeRamAndStorage();

  CommonInterface::status_messages.Startup(true);

  if (is_simulator()) {
    InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
  } else {
    InputEvents::processGlideComputer(GCE_STARTUP_REAL);
  }

  const TaskFactoryType task_type_default =
    CommonInterface::GetComputerSettings().task.task_type_default;
  OrderedTask *defaultTask =
    protected_task_manager->TaskCreateDefault(&way_points, task_type_default);
  if (defaultTask) {
    {
      ScopeSuspendAllThreads suspend;
      defaultTask->CheckDuplicateWaypoints(way_points);
      way_points.Optimise();
    }

    protected_task_manager->TaskCommit(*defaultTask);
    delete defaultTask;
  }

  task_manager->Resume();

  CommonInterface::main_window->Fullscreen();
  InfoBoxManager::SetDirty();

  ForceCalculation();

  CommonInterface::status_messages.Startup(false);
}

/**
 * "Boots" up XCSoar
 * @param hInstance Instance handle
 * @param lpCmdLine Command line string
 * @return True if bootup successful, False otherwise
 */
bool
Startup()
{
  VerboseOperationEnvironment operation;

#ifdef USE_GDI
  //If "XCSoar" is already running, stop this instance
  if (MainWindow::Find())
    return false;
#endif

#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::Initialise();
#endif

  LogFormat("Display dpi=%u,%u", Display::GetXDPI(), Display::GetYDPI());

  // Creates the main window

  TopWindowStyle style;
  if (CommandLine::full_screen)
    style.FullScreen();

  if (!IsWindowsCE())
    style.Resizable();

  MainWindow *const main_window = CommonInterface::main_window =
    new MainWindow(CommonInterface::status_messages);
  main_window->Create(SystemWindowSize(), style);
  if (!main_window->IsDefined())
    return false;

#ifdef ENABLE_OPENGL
  LogFormat("OpenGL: "
#ifdef HAVE_DYNAMIC_EGL
            "egl=%d "
#endif
            "npot=%d vbo=%d fbo=%d stencil=%#x",
#ifdef HAVE_DYNAMIC_EGL
             OpenGL::egl,
#endif
             OpenGL::texture_non_power_of_two,
             OpenGL::vertex_buffer_object,
            OpenGL::frame_buffer_object,
            OpenGL::render_buffer_stencil);
#endif

  main_window->Initialise();

#ifdef SIMULATOR_AVAILABLE
  // prompt for simulator if not set by command line argument "-simulator" or "-fly"
  if (!sim_set_in_cmd_line_flag) {
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

  CommonInterface::SetSystemSettings().SetDefaults();
  CommonInterface::SetComputerSettings().SetDefaults();
  CommonInterface::SetUISettings().SetDefaults();
  CommonInterface::SetUIState().Clear();

  const auto &computer_settings = CommonInterface::GetComputerSettings();
  const auto &ui_settings = CommonInterface::GetUISettings();
  auto &live_blackboard = CommonInterface::GetLiveBlackboard();

  if (!LoadProfile())
    return false;

  operation.SetText(_("Initialising"));

  /* create XCSoarData on the first start */
  CreateDataPath();

  Display::LoadOrientation(operation);
  main_window->CheckResize();

  main_window->InitialiseConfigured();

  TCHAR path[MAX_PATH];
  LocalPath(path, _T("cache"));
  file_cache = new FileCache(path);

  ReadLanguageFile();

  CommonInterface::status_messages.LoadFile();
  InputEvents::readFile();

  // Initialize DeviceBlackboard
  device_blackboard = new DeviceBlackboard();

  DeviceListInitialise();

  // Initialize Markers
  marks = new Markers();
  protected_marks = new ProtectedMarkers(*marks);

#ifdef HAVE_AYGSHELL_DLL
  const AYGShellDLL &ayg = main_window->ayg_shell_dll;
  ayg.SHSetAppKeyWndAssoc(VK_APP1, *main_window);
  ayg.SHSetAppKeyWndAssoc(VK_APP2, *main_window);
  ayg.SHSetAppKeyWndAssoc(VK_APP3, *main_window);
  ayg.SHSetAppKeyWndAssoc(VK_APP4, *main_window);
  // Typical Record Button
  //	Why you can't always get this to work
  //	http://forums.devbuzz.com/m_1185/mpage_1/key_/tm.htm
  //	To do with the fact it is a global hotkey, but you can with code above
  //	Also APPA is record key on some systems
  ayg.SHSetAppKeyWndAssoc(VK_APP5, *main_window);
  ayg.SHSetAppKeyWndAssoc(VK_APP6, *main_window);
#endif

  // Initialize main blackboard data
  task_events = new GlideComputerTaskEvents();
  task_manager = new TaskManager(computer_settings.task, way_points);
  task_manager->SetTaskEvents(*task_events);
  task_manager->Reset();

  protected_task_manager =
    new ProtectedTaskManager(*task_manager, computer_settings.task);

  // Read the terrain file
  operation.SetText(_("Loading Terrain File..."));
  LogFormat("OpenTerrain");
  terrain = RasterTerrain::OpenTerrain(file_cache, operation);

  logger = new Logger();

  glide_computer = new GlideComputer(way_points, airspace_database,
                                     *protected_task_manager,
                                     *task_events);
  glide_computer->ReadComputerSettings(computer_settings);
  glide_computer->SetTerrain(terrain);
  glide_computer->SetLogger(logger);
  glide_computer->Initialise();

  replay = new Replay(logger, *protected_task_manager);

  // Load the EGM96 geoid data
  EGM96::Load();

  GlidePolar &gp = CommonInterface::SetComputerSettings().polar.glide_polar_task;
  gp = GlidePolar(fixed(0));
  gp.SetMC(computer_settings.task.safety_mc);
  gp.SetBugs(computer_settings.polar.degradation_factor);
  PlaneGlue::FromProfile(CommonInterface::SetComputerSettings().plane);
  PlaneGlue::Synchronize(computer_settings.plane,
                         CommonInterface::SetComputerSettings(), gp);
  task_manager->SetGlidePolar(gp);

  // Read the topography file(s)
  topography = new TopographyStore();
  LoadConfiguredTopography(*topography, operation);

  // Read the waypoint files
  WaypointGlue::LoadWaypoints(way_points, terrain, operation);

  // Read and parse the airfield info file
  WaypointDetails::ReadFileFromProfile(way_points, operation);

  // Set the home waypoint
  WaypointGlue::SetHome(way_points, terrain,
                        CommonInterface::SetComputerSettings().poi,
                        CommonInterface::SetComputerSettings().team_code,
                        device_blackboard, false);

  // ReSynchronise the blackboards here since SetHome touches them
  device_blackboard->Merge();
  CommonInterface::ReadBlackboardBasic(device_blackboard->Basic());

  // Scan for weather forecast
  LogFormat("RASP load");
  RASP.ScanAll(CommonInterface::Basic().location, operation);

  // Reads the airspace files
  ReadAirspace(airspace_database, terrain, computer_settings.pressure,
               operation);

  {
    const AircraftState aircraft_state =
      ToAircraftState(device_blackboard->Basic(),
                      device_blackboard->Calculated());
    ProtectedAirspaceWarningManager::ExclusiveLease lease(glide_computer->GetAirspaceWarnings());
    lease->Reset(aircraft_state);
  }

#ifdef HAVE_NOAA
  noaa_store = new NOAAStore();
  noaa_store->LoadFromProfile();
#endif

  AudioVarioGlue::Initialise();
  AudioVarioGlue::Configure(ui_settings.sound.vario);

  // Start the device thread(s)
  operation.SetText(_("Starting devices"));
  devStartup();

/*
  -- Reset polar in case devices need the data
  GlidePolar::UpdatePolar(true, computer_settings);

  This should be done inside devStartup if it is really required
*/

  operation.SetText(_("Initialising display"));

  GlueMapWindow *map_window = main_window->GetMap();
  if (map_window != NULL) {
    map_window->SetWaypoints(&way_points);
    map_window->SetTask(protected_task_manager);
    map_window->SetRoutePlanner(&glide_computer->GetProtectedRoutePlanner());
    map_window->SetGlideComputer(glide_computer);
    map_window->SetAirspaces(&airspace_database);

    map_window->SetTopography(topography);
    map_window->SetTerrain(terrain);
    map_window->SetWeather(&RASP);
    map_window->SetMarks(protected_marks);
    map_window->SetLogger(logger);

#ifdef HAVE_NOAA
    map_window->SetNOAAStore(noaa_store);
#endif

    /* show map at home waypoint until GPS fix becomes available */
    if (computer_settings.poi.home_location_available)
      map_window->SetLocation(computer_settings.poi.home_location);
  }

  // Finally ready to go.. all structures must be present before this.

  // Create the drawing thread
#ifndef ENABLE_OPENGL
  draw_thread = new DrawThread(*map_window);
  draw_thread->Start(true);
#endif

  // Show the infoboxes
  InfoBoxManager::Show();

  // Create the calculation thread
  CreateCalculationThread();

  // Find unique ID of this PDA
  ReadAssetNumber();

  glide_computer_events = new GlideComputerEvents();
  glide_computer_events->Reset();
  live_blackboard.AddListener(*glide_computer_events);

  all_monitors = new AllMonitors();

  if (computer_settings.logger.enable_flight_logger) {
    flight_logger = new GlueFlightLogger(live_blackboard);
    LocalPath(path, _T("flights.log"));
    flight_logger->SetPath(path);
  }

  if (computer_settings.logger.enable_nmea_logger)
    NMEALogger::enabled = true;

  LogFormat("ProgramStarted");

  // Give focus to the map
  main_window->SetDefaultFocus();

  // Start calculation thread
  merge_thread->Start();
  calculation_thread->Start();

  PageActions::Update();

#ifdef HAVE_TRACKING
  tracking = new TrackingGlue();
  tracking->SetSettings(computer_settings.tracking);

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  if (map_window != nullptr)
    map_window->SetSkyLinesData(&tracking->GetSkyLinesData());
#endif
#endif

  assert(!global_running);
  global_running = true;

  AfterStartup();

  operation.Hide();

  main_window->FinishStartup();

  return true;
}

void
Shutdown()
{
  VerboseOperationEnvironment operation;
  gcc_unused ScopeBusyIndicator busy;

  MainWindow *const main_window = CommonInterface::main_window;
  auto &live_blackboard = CommonInterface::GetLiveBlackboard();

  // Show progress dialog
  operation.SetText(_("Shutdown, please wait..."));

  // Log shutdown information
  LogFormat("Entering shutdown...");

  main_window->BeginShutdown();

  StartupLogFreeRamAndStorage();

  // Turn off all displays
  global_running = false;

#ifdef HAVE_TRACKING
  if (tracking != NULL)
    tracking->StopAsync();
#endif

  // Stop logger and save igc file
  operation.SetText(_("Shutdown, saving logs..."));
  logger->GUIStopLogger(CommonInterface::Basic(), true);

  delete flight_logger;
  flight_logger = NULL;

  delete all_monitors;
  live_blackboard.RemoveListener(*glide_computer_events);
  delete glide_computer_events;

  SaveFlarmColors();

  // Save settings to profile
  operation.SetText(_("Shutdown, saving profile..."));
  Profile::Save();

  // Stop sound

  AudioVarioGlue::Deinitialise();

  operation.SetText(_("Shutdown, please wait..."));

  // Stop threads
  LogFormat("Stop threads");
#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::BeginDeinitialise();
#endif
#ifndef ENABLE_OPENGL
  draw_thread->BeginStop();
#endif
  calculation_thread->BeginStop();
  merge_thread->BeginStop();

  // Wait for the calculations thread to finish
  LogFormat("Waiting for calculation thread");

  merge_thread->Join();
  delete merge_thread;
  merge_thread = NULL;

  calculation_thread->Join();
  delete calculation_thread;
  calculation_thread = NULL;

  //  Wait for the drawing thread to finish
#ifndef ENABLE_OPENGL
  LogFormat("Waiting for draw thread");

  draw_thread->Join();
  delete draw_thread;
#endif

  LogFormat("delete MapWindow");
  main_window->Deinitialise();

  // Save the task for the next time
  operation.SetText(_("Shutdown, saving task..."));

  LogFormat("Save default task");
  protected_task_manager->TaskSaveDefault();

  // Clear waypoint database
  way_points.Clear();

  operation.SetText(_("Shutdown, please wait..."));

  // Clear weather database
  RASP.Close();

  // Clear terrain database

  delete terrain;
  delete topography;

  delete protected_marks;
  delete marks;

  // Close any device connections
  devShutdown();

  NMEALogger::Shutdown();

  delete replay;

  DeviceListDeinitialise();

  delete device_blackboard;
  device_blackboard = NULL;

  protected_task_manager->SetRoutePlanner(NULL);

  delete protected_task_manager;
  delete task_manager;

#ifdef HAVE_NOAA
  delete noaa_store;
#endif

#ifdef HAVE_TRACKING
  if (tracking != NULL) {
    tracking->WaitStopped();
    delete tracking;
  }
#endif

#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::Deinitialise();
#endif

  // Close the progress dialog
  LogFormat("Close Progress Dialog");
  operation.Hide();

  // Clear the EGM96 database
  EGM96::Close();

  delete glide_computer;
  delete task_events;
  delete logger;

  // Clear airspace database
  airspace_database.Clear();

  // Destroy FlarmNet records
  DeinitTrafficGlobals();

  delete file_cache;

  LogFormat("Close Windows - main");
  main_window->Destroy();

  CloseLanguageFile();

  Display::RestoreOrientation();

  StartupLogFreeRamAndStorage();

  LogFormat("Finished shutdown");
}
