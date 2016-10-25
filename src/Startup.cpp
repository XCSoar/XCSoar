/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Profile/Current.hpp"
#include "Profile/Settings.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Input/InputEvents.hpp"
#include "Input/InputQueue.hpp"
#include "Dialogs/StartupDialog.hpp"
#include "Dialogs/dlgSimulatorPrompt.hpp"
#include "Language/LanguageGlue.hpp"
#include "Language/Language.hpp"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "UtilsSystem.hpp"
#include "FLARM/Glue.hpp"
#include "Logger/Logger.hpp"
#include "Logger/NMEALogger.hpp"
#include "Logger/GlueFlightLogger.hpp"
#include "Waypoint/WaypointDetailsReader.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Device/device.hpp"
#include "Device/MultipleDevices.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Audio/Features.hpp"
#include "Audio/GlobalVolumeController.hpp"
#include "Audio/VarioGlue.hpp"
#include "Audio/VolumeController.hpp"
#include "Screen/Busy.hpp"
#include "CommandLine.hpp"
#include "MainWindow.hpp"
#include "Computer/GlideComputer.hpp"
#include "Computer/GlideComputerInterface.hpp"
#include "Computer/Events.hpp"
#include "Monitor/AllMonitors.hpp"
#include "MergeThread.hpp"
#include "CalculationThread.hpp"
#include "Replay/Replay.hpp"
#include "LocalPath.hpp"
#include "IO/FileCache.hpp"
#include "IO/Async/AsioThread.hpp"
#include "IO/Async/GlobalAsioThread.hpp"
#include "Net/HTTP/DownloadManager.hpp"
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
#include "Task/DefaultTask.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
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

#include "Lua/StartFile.hpp"
#include "Lua/Background.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Dynamic.hpp"
#else
#include "DrawThread.hpp"
#endif

static TaskManager *task_manager;
static GlideComputerEvents *glide_computer_events;
static AllMonitors *all_monitors;
static GlideComputerTaskEvents *task_events;

static bool
LoadProfile()
{
  if (Profile::GetPath().IsNull() &&
      !dlgStartupShowModal())
    return false;

  Profile::Load();
  Profile::Use(Profile::map);

  Units::SetConfig(CommonInterface::GetUISettings().format.units);
  SetUserCoordinateFormat(CommonInterface::GetUISettings().format.coordinate_format);

  return true;
}

static void
AfterStartup()
{
  StartupLogFreeRamAndStorage();

  try {
    const auto lua_path = LocalPath(_T("lua"));
    Lua::StartFile(AllocatedPath::Build(lua_path, _T("init.lua")));
  } catch (const std::runtime_error &e) {
      LogError(e);
  }

  if (is_simulator()) {
    InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
  } else {
    InputEvents::processGlideComputer(GCE_STARTUP_REAL);
  }

  OrderedTask *defaultTask = LoadDefaultTask(CommonInterface::GetComputerSettings().task,
                                             &way_points);
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

  InfoBoxManager::SetDirty();

  ForceCalculation();
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

#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::Initialise();
#endif

  LogFormat("Display dpi=%u,%u", Display::GetXDPI(), Display::GetYDPI());

  // Creates the main window

  TopWindowStyle style;
  if (CommandLine::full_screen)
    style.FullScreen();

  style.Resizable();

  MainWindow *const main_window = CommonInterface::main_window =
    new MainWindow();
  main_window->Create(SystemWindowSize(), style);
  if (!main_window->IsDefined())
    return false;

#ifdef ENABLE_OPENGL
  LogFormat("OpenGL: "
#ifdef ANDROID
#ifdef USE_EGL
            "egl=native "
#else
            "egl=no "
#endif
#endif
#ifdef HAVE_OES_DRAW_TEXTURE
            "oesdt=%d "
#endif
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
            "mda=%d "
#endif
            "npot=%d vbo=%d fbo=%d stencil=%#x",
#ifdef HAVE_OES_DRAW_TEXTURE
            OpenGL::oes_draw_texture,
#endif
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
            GLExt::HaveMultiDrawElements(),
#endif
             OpenGL::texture_non_power_of_two,
             OpenGL::vertex_buffer_object,
            OpenGL::frame_buffer_object,
            OpenGL::render_buffer_stencil);
#endif

  CommonInterface::SetUISettings().SetDefaults();
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

  {
    file_cache = new FileCache(LocalPath(_T("cache")));
  }

  ReadLanguageFile();

  InputEvents::readFile();

  // Initialize DeviceBlackboard
  device_blackboard = new DeviceBlackboard();
  devices = new MultipleDevices(*asio_thread);
  device_blackboard->SetDevices(*devices);

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

  glide_computer = new GlideComputer(computer_settings,
                                     way_points, airspace_database,
                                     *protected_task_manager,
                                     *task_events);
  glide_computer->SetTerrain(terrain);
  glide_computer->SetLogger(logger);
  glide_computer->Initialise();

  replay = new Replay(logger, *protected_task_manager);

#ifdef HAVE_CMDLINE_REPLAY
  if (CommandLine::replay_path != nullptr) {
    try {
      replay->Start(Path(CommandLine::replay_path));
    } catch (const std::runtime_error &e) {
      LogError(e);
    }
  }
#endif


  GlidePolar &gp = CommonInterface::SetComputerSettings().polar.glide_polar_task;
  gp = GlidePolar(0);
  gp.SetMC(computer_settings.task.safety_mc);
  gp.SetBugs(computer_settings.polar.degradation_factor);
  PlaneGlue::FromProfile(CommonInterface::SetComputerSettings().plane,
                         Profile::map);
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
  auto rasp = std::make_shared<RaspStore>(LocalPath(_T(RASP_FILENAME)));
  rasp->ScanAll();

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

#ifdef HAVE_VOLUME_CONTROLLER
  volume_controller->SetVolume(ui_settings.sound.master_volume);
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
  if (map_window != nullptr) {
    map_window->SetWaypoints(&way_points);
    map_window->SetTask(protected_task_manager);
    map_window->SetRoutePlanner(&glide_computer->GetProtectedRoutePlanner());
    map_window->SetGlideComputer(glide_computer);
    map_window->SetAirspaces(&airspace_database);

    map_window->SetTopography(topography);
    map_window->SetTerrain(terrain);
    map_window->SetRasp(rasp);

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

  if (!is_simulator() && computer_settings.logger.enable_flight_logger) {
    flight_logger = new GlueFlightLogger(live_blackboard);
    flight_logger->SetPath(LocalPath(_T("flights.log")));
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
  tracking = new TrackingGlue(*asio_thread);
  tracking->SetSettings(computer_settings.tracking);

#ifdef HAVE_SKYLINES_TRACKING
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

  Lua::StopAllBackground();

  // Turn off all displays
  global_running = false;

#ifdef HAVE_TRACKING
  if (tracking != nullptr)
    tracking->StopAsync();
#endif

  // Stop logger and save igc file
  operation.SetText(_("Shutdown, saving logs..."));
  if (logger != nullptr)
    logger->GUIStopLogger(CommonInterface::Basic(), true);

  delete flight_logger;
  flight_logger = nullptr;

  delete all_monitors;
  all_monitors = nullptr;

  if (glide_computer_events != nullptr) {
    live_blackboard.RemoveListener(*glide_computer_events);
    delete glide_computer_events;
    glide_computer_events = nullptr;
  }

  SaveFlarmColors();

  // Save settings to profile
  operation.SetText(_("Shutdown, saving profile..."));
  Profile::Save();

  operation.SetText(_("Shutdown, please wait..."));

  // Stop threads
  LogFormat("Stop threads");
#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::BeginDeinitialise();
#endif
#ifndef ENABLE_OPENGL
  if (draw_thread != nullptr)
    draw_thread->BeginStop();
#endif

  if (calculation_thread != nullptr)
    calculation_thread->BeginStop();

  if (merge_thread != nullptr)
    merge_thread->BeginStop();

  // Wait for the calculations thread to finish
  LogFormat("Waiting for calculation thread");

  if (merge_thread != nullptr) {
    merge_thread->Join();
    delete merge_thread;
    merge_thread = nullptr;
  }

  if (calculation_thread != nullptr) {
    calculation_thread->Join();
    delete calculation_thread;
    calculation_thread = nullptr;
  }

  //  Wait for the drawing thread to finish
#ifndef ENABLE_OPENGL
  LogFormat("Waiting for draw thread");

  if (draw_thread != nullptr) {
    draw_thread->Join();
    delete draw_thread;
    draw_thread = nullptr;
  }
#endif

  LogFormat("delete MapWindow");
  main_window->Deinitialise();

  // Stop sound
  AudioVarioGlue::Deinitialise();

  // Save the task for the next time
  operation.SetText(_("Shutdown, saving task..."));

  LogFormat("Save default task");
  if (protected_task_manager != nullptr) {
    try {
      protected_task_manager->TaskSaveDefault();
    } catch (const std::runtime_error &e) {
      LogError(e);
    }
  }

  // Clear waypoint database
  way_points.Clear();

  operation.SetText(_("Shutdown, please wait..."));

  // Clear terrain database

  delete terrain;
  terrain = nullptr;
  delete topography;
  topography = nullptr;

  // Close any device connections
  devShutdown();

  NMEALogger::Shutdown();

  delete replay;
  replay = nullptr;

  delete devices;
  devices = nullptr;
  delete device_blackboard;
  device_blackboard = nullptr;

  if (protected_task_manager != nullptr) {
    protected_task_manager->SetRoutePlanner(nullptr);
    delete protected_task_manager;
    protected_task_manager = nullptr;
  }

  delete task_manager;
  task_manager = nullptr;

#ifdef HAVE_NOAA
  delete noaa_store;
  noaa_store = nullptr;
#endif

#ifdef HAVE_TRACKING
  if (tracking != nullptr) {
    tracking->WaitStopped();
    delete tracking;
    tracking = nullptr;
  }
#endif

#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::Deinitialise();
#endif

  // Close the progress dialog
  LogFormat("Close Progress Dialog");
  operation.Hide();

  delete glide_computer;
  glide_computer = nullptr;
  delete task_events;
  task_events = nullptr;
  delete logger;
  logger = nullptr;

  // Clear airspace database
  airspace_database.Clear();

  // Destroy FlarmNet records
  DeinitTrafficGlobals();

  delete file_cache;
  file_cache = nullptr;

  LogFormat("Close Windows - main");
  main_window->Destroy();
  delete main_window;

  CloseLanguageFile();

  Display::RestoreOrientation();

  StartupLogFreeRamAndStorage();

  LogFormat("Finished shutdown");
}
