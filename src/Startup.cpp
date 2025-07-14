// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Startup.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "DataGlobals.hpp"
#include "ui/canvas/Features.hpp" // for SOFTWARE_ROTATE_DISPLAY
#include "Profile/Profile.hpp"
#include "Profile/Current.hpp"
#include "Profile/Settings.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/AsyncLoader.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Skysight/Skysight.hpp"
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
#include "Device/Factory.hpp"
#include "Device/device.hpp"
#include "Device/MultipleDevices.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Audio/Features.hpp"
#include "Audio/GlobalVolumeController.hpp"
#include "Audio/VarioGlue.hpp"
#include "Audio/VolumeController.hpp"
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
#include "io/FileCache.hpp"
#include "io/async/AsioThread.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "net/http/Init.hpp"
#include "net/http/DownloadManager.hpp"
#include "net/client/tim/Glue.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "Hardware/DisplayGlue.hpp"
#include "util/Compiler.h"
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
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Operation/SubOperationEnvironment.hpp"
#include "Widget/ProgressWidget.hpp"
#include "PageActions.hpp"
#include "Weather/Features.hpp"
#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Plane/PlaneGlue.hpp"
#include "UIState.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "thread/Debug.hpp"

#include "lua/StartFile.hpp"
#include "lua/Background.hpp"

#include "util/ScopeExit.hxx"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/canvas/opengl/Dynamic.hpp"
#else
#include "DrawThread.hpp"
#endif

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

static TaskManager *task_manager;
static GlideComputerEvents *glide_computer_events;
static AllMonitors *all_monitors;
static GlideComputerTaskEvents *task_events;
static DeviceFactory *device_factory;

static bool
LoadProfile()
{
  if (Profile::GetPath() == nullptr &&
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
  try {
    const auto lua_path = LocalPath(_T("lua"));
    Lua::StartFile(AllocatedPath::Build(lua_path, _T("init.lua")));
  } catch (...) {
      LogError(std::current_exception());
  }

  if (is_simulator()) {
    InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
  } else {
    InputEvents::processGlideComputer(GCE_STARTUP_REAL);
  }

  auto &way_points = *data_components->waypoints;

  const auto defaultTask = LoadDefaultTask(CommonInterface::GetComputerSettings().task,
                                           &way_points);
  if (defaultTask) {
    {
      ScopeSuspendAllThreads suspend;
      defaultTask->CheckDuplicateWaypoints(way_points);
      way_points.Optimise();
    }

    backend_components->protected_task_manager->TaskCommit(*defaultTask);
  }

  task_manager->Resume();

  InfoBoxManager::SetDirty();

  ForceCalculation();
}

void
MainWindow::LoadTerrain() noexcept
{
  SetTopWidget(nullptr);

  delete terrain_loader;
  terrain_loader = nullptr;

  if (const auto path = Profile::GetPath(ProfileKeys::MapFile);
      path != nullptr) {
    LogString("LoadTerrain");
    terrain_loader = new AsyncTerrainOverviewLoader();

    terrain_loader_env = std::make_unique<PluggableOperationEnvironment>();
    auto *progress = new ProgressWidget(*terrain_loader_env,
                                        _("Loading Terrain File..."));
    SetTopWidget(progress);

    terrain_loader->Start(file_cache, path, *terrain_loader_env,
                          terrain_loader_notify);
  } else if (data_components->terrain) {
    /* the map file has been disabled - remove the terrain from all
       subsystems and dispose the object */

    const ScopeSuspendAllThreads suspend;
    DataGlobals::UnsetTerrain();

    /* this call is only necessary so the bottom widget that may have
       been cleared by UnsetTerrain() gets created again */
    DataGlobals::SetTerrain(nullptr);
  }
}

void
MainWindow::OnTerrainLoaded() noexcept
try {
  assert(terrain_loader != nullptr);

  std::unique_ptr<AsyncTerrainOverviewLoader> loader{std::exchange(terrain_loader, nullptr)};
  auto new_terrain = loader->Wait();
  loader.reset();

  SetTopWidget(nullptr);
  terrain_loader_env.reset();

  const ScopeSuspendAllThreads suspend;

  DataGlobals::UnsetTerrain();
  DataGlobals::SetTerrain(std::move(new_terrain));
  DataGlobals::UpdateHome(false);

  SetAirspaceGroundLevels(*data_components->airspaces,
                          *data_components->terrain);
} catch (...) {
  LogError(std::current_exception(), "LoadTerrain failed");
}

/**
 * "Boots" up XCSoar
 * @param lpCmdLine Command line string
 * @return True if bootup successful, False otherwise
 */
bool
Startup(UI::Display &display)
{
  VerboseOperationEnvironment operation;
  operation.SetProgressRange(1024);

#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::Initialise();
#endif

  // Creates the main window

  UI::TopWindowStyle style;
  if (CommandLine::full_screen)
    style.FullScreen();

  style.Resizable();

#ifdef SOFTWARE_ROTATE_DISPLAY
  style.InitialOrientation(Display::DetectInitialOrientation());
#endif

  MainWindow *const main_window = CommonInterface::main_window =
    new MainWindow(display);
  main_window->Create(SystemWindowSize(), style);
  if (!main_window->IsDefined())
    return false;

  LogFmt("Display dpi={},{}",
         Display::GetDPI(display).x, Display::GetDPI(display).y);

#ifdef ENABLE_OPENGL
  LogFmt("OpenGL: "
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
         "mda={} "
#endif
         "npot={} stencil={:#x}",
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
         GLExt::HaveMultiDrawElements(),
#endif
         OpenGL::texture_non_power_of_two,
         OpenGL::render_buffer_stencil);
#endif

#ifdef ANDROID
  /* mark the UI EventLoop as "running", which allows
     TopWindow::Pause() to submit the PAUSE command to the event
     queue; this works because the remaining code in this function may
     invoke modal dialogs, and its event loop will be able to process
     PAUSE even if TopWindow::RunEventLoop() is not yet invoked */
  main_window->BeginRunning();
  AtScopeExit(main_window) { main_window->EndRunning(); };
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

#ifdef ANDROID
  {
    const auto env = Java::GetEnv();
    native_view->AcquireWakeLock(env);
    native_view->SetFullScreen(env, ui_settings.display.full_screen);
  }
#endif

  Display::LoadOrientation(operation);
  main_window->CheckResize();

  main_window->InitialiseConfigured();

  file_cache = new FileCache(GetCachePath());

  data_components = new DataComponents();
  backend_components = new BackendComponents();

  ReadLanguageFile();

  try {
    LogString("Loading input events file");
    InputEvents::readFile();
  } catch (...) {
    LogError(std::current_exception());
  }

  backend_components->igc_logger = std::make_unique<Logger>();
  backend_components->nmea_logger = std::make_unique<NMEALogger>();

  // Initialize DeviceBlackboard
  device_factory = new DeviceFactory{
    *asio_thread, *global_cares_channel,
#ifdef ANDROID
    *context, permission_manager,
    bluetooth_helper, ioio_helper, usb_serial_helper,
#endif
  };

  backend_components->devices = std::make_unique<MultipleDevices>(*backend_components->device_blackboard,
                                                                  backend_components->nmea_logger.get(),
                                                                  *device_factory);

  // Initialize main blackboard data
  task_events = new GlideComputerTaskEvents();
  task_manager = new TaskManager(computer_settings.task,
                                 *data_components->waypoints);
  task_manager->SetTaskEvents(*task_events);
  task_manager->Reset();

  backend_components->protected_task_manager =
    std::make_unique<ProtectedTaskManager>(*task_manager, computer_settings.task);

  // Read the terrain file
  main_window->LoadTerrain();

  backend_components->glide_computer =
    std::make_unique<GlideComputer>(computer_settings,
                                    *data_components->waypoints,
                                    *data_components->airspaces,
                                    *backend_components->protected_task_manager,
                                    *task_events);
  backend_components->glide_computer->SetTerrain(data_components->terrain.get());
  backend_components->glide_computer->SetLogger(backend_components->igc_logger.get());
  backend_components->glide_computer->Initialise();

  backend_components->replay =
    std::make_unique<Replay>(*backend_components->device_blackboard,
                             backend_components->igc_logger.get(),
                             *backend_components->protected_task_manager);

#ifdef HAVE_CMDLINE_REPLAY
  if (CommandLine::replay_path != nullptr) {
    try {
      backend_components->replay->Start(Path(CommandLine::replay_path));
    } catch (...) {
      LogError(std::current_exception());
    }
  }
#endif


  GlidePolar &gp = CommonInterface::SetComputerSettings().polar.glide_polar_task;
  gp = GlidePolar(0);
  gp.SetMC(computer_settings.task.safety_mc);
  gp.SetBugs(computer_settings.polar.degradation_factor);
  gp.SetCrewMass(computer_settings.logger.crew_mass_template);
  PlaneGlue::FromProfile(CommonInterface::SetComputerSettings().plane,
                         Profile::map);
  PlaneGlue::Synchronize(computer_settings.plane,
                         CommonInterface::SetComputerSettings(), gp);
  task_manager->SetGlidePolar(gp);

  // Read the topography file(s)
  data_components->topography = std::make_unique<TopographyStore>();
  {
    LogString("Loading Topography File...");
    operation.SetText(_("Loading Topography File..."));
    LoadConfiguredTopography(*data_components->topography);
    operation.SetProgressPosition(256);
  }

  // Read the waypoint files
  LogString("ReadWaypoints");
  {
    SubOperationEnvironment sub_env(operation, 256, 512);
    sub_env.SetText(_("Loading Waypoints..."));
    WaypointGlue::LoadWaypoints(*data_components->waypoints,
                                data_components->terrain.get(),
                                sub_env);
  }

  // Read and parse the airfield info file
  try {
    SubOperationEnvironment sub_env(operation, 512, 768);
    sub_env.SetText(_("Loading Airfield Details File..."));
    WaypointDetails::ReadFileFromProfile(*data_components->waypoints, sub_env);
  } catch (...) {
    LogError(std::current_exception());
  }

  // Set the home waypoint
  WaypointGlue::SetHome(*data_components->waypoints,
                        data_components->terrain.get(),
                        CommonInterface::SetComputerSettings().poi,
                        CommonInterface::SetComputerSettings().team_code,
                        backend_components->device_blackboard.get(),
                        false);

  // ReSynchronise the blackboards here since SetHome touches them
  backend_components->device_blackboard->Merge();
  CommonInterface::ReadBlackboardBasic(backend_components->device_blackboard->Basic());

  // Scan for weather forecast
  LogString("RASP load");
  auto rasp = LoadConfiguredRasp();

  //Initialise Skysight weather forecast
  LogFormat("Skysight load");
  auto skysight = std::make_shared<Skysight>(*Net::curl);

  // Reads the airspace files
  {
    SubOperationEnvironment sub_env(operation, 768, 1024);
    ReadAirspace(*data_components->airspaces,
                 computer_settings.pressure,
                 sub_env);
  }

  if (data_components->terrain)
    SetAirspaceGroundLevels(*data_components->airspaces,
                            *data_components->terrain);

  {
    const AircraftState aircraft_state =
      ToAircraftState(backend_components->device_blackboard->Basic(),
                      backend_components->device_blackboard->Calculated());
    ProtectedAirspaceWarningManager::ExclusiveLease lease(backend_components->glide_computer->GetAirspaceWarnings());
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
  if (backend_components->devices != nullptr) {
    operation.SetText(_("Starting devices"));
    devStartup(*backend_components->devices,
               CommonInterface::GetSystemSettings());
  }

/*
  -- Reset polar in case devices need the data
  GlidePolar::UpdatePolar(true, computer_settings);

  This should be done inside devStartup if it is really required
*/

  operation.SetText(_("Initialising display"));

  GlueMapWindow *map_window = main_window->GetMap();
  if (map_window != nullptr) {
    map_window->SetWaypoints(data_components->waypoints.get());
    map_window->SetTask(backend_components->protected_task_manager.get());
    map_window->SetRoutePlanner(&backend_components->glide_computer->GetProtectedRoutePlanner());
    map_window->SetGlideComputer(backend_components->glide_computer.get());
    map_window->SetAirspaces(data_components->airspaces.get());

    map_window->SetTopography(data_components->topography.get());
    map_window->SetTerrain(data_components->terrain.get());
    map_window->SetRasp(rasp);
    map_window->SetSkysight(skysight);

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

  glide_computer_events = new GlideComputerEvents();
  glide_computer_events->Reset();
  live_blackboard.AddListener(*glide_computer_events);

  all_monitors = new AllMonitors();

  if (!is_simulator() && computer_settings.logger.enable_flight_logger) {
    backend_components->flight_logger = std::make_unique<GlueFlightLogger>(live_blackboard);
    backend_components->flight_logger->SetPath(LocalPath(_T("flights.log")));
  }

  if (computer_settings.logger.enable_nmea_logger)
    backend_components->nmea_logger->Enable();

  LogString("ProgramStarted");

  // Give focus to the map
  main_window->SetDefaultFocus();

  // Start calculation thread
  backend_components->merge_thread->Start();
  backend_components->calculation_thread->Start();

  PageActions::Update();

  net_components = new NetComponents(*asio_thread, *Net::curl,
                                     computer_settings.tracking);
#ifdef HAVE_SKYLINES_TRACKING
  if (map_window != nullptr)
    map_window->SetSkyLinesData(&net_components->tracking->GetSkyLinesData());
#endif

#ifdef HAVE_HTTP
  if (map_window != nullptr)
    map_window->SetThermalInfoMap(net_components->tim.get());
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

  MainWindow *const main_window = CommonInterface::main_window;
  auto &live_blackboard = CommonInterface::GetLiveBlackboard();

  // Show progress dialog
  operation.SetText(_("Shutdown, please wait..."));

  // Log shutdown information
  LogString("Entering shutdown...");

  main_window->BeginShutdown();

  Lua::StopAllBackground();

  // Turn off all displays
  global_running = false;

  // Stop logger and save igc file
  if (backend_components != nullptr && backend_components->igc_logger != nullptr) {
    operation.SetText(_("Shutdown, saving logs..."));

    try {
      backend_components->igc_logger->GUIStopLogger(CommonInterface::Basic(), true);
    } catch (...) {
      LogError(std::current_exception());
    }
  }

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

  // Close any device connections
  if (backend_components != nullptr && backend_components->devices != nullptr) {
    LogString("Stop devices");
    backend_components->devices->Close();
  }

  // Stop threads
  LogString("Stop threads");
#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::BeginDeinitialise();
#endif
#ifndef ENABLE_OPENGL
  if (draw_thread != nullptr)
    draw_thread->BeginStop();
#endif

  if (backend_components != nullptr) {
    if (backend_components->calculation_thread)
      backend_components->calculation_thread->BeginStop();

    if (backend_components->merge_thread)
      backend_components->merge_thread->BeginStop();

    // Wait for the calculations thread to finish
    LogString("Waiting for calculation thread");

    if (backend_components->merge_thread) {
      backend_components->merge_thread->Join();
      backend_components->merge_thread.reset();
    }

    if (backend_components->calculation_thread) {
      backend_components->calculation_thread->Join();
      backend_components->calculation_thread.reset();
    }
  }

  //  Wait for the drawing thread to finish
#ifndef ENABLE_OPENGL
  LogString("Waiting for draw thread");

  if (draw_thread != nullptr) {
    draw_thread->Join();
    delete draw_thread;
    draw_thread = nullptr;
  }
#endif

  LogString("delete MapWindow");
  main_window->Deinitialise();

  // Stop sound
  AudioVarioGlue::Deinitialise();

  // Save the task for the next time
  if (backend_components != nullptr && backend_components->protected_task_manager) {
    operation.SetText(_("Shutdown, saving task..."));
    LogString("Save default task");

    try {
      backend_components->protected_task_manager->TaskSaveDefault();
    } catch (...) {
      LogError(std::current_exception());
    }
  }

  operation.SetText(_("Shutdown, please wait..."));

  // Clear terrain database

  delete terrain_loader;
  terrain_loader = nullptr;

  if (backend_components != nullptr)
    backend_components->devices.reset();

  delete device_factory;
  device_factory = nullptr;

  if (backend_components != nullptr) {
    backend_components->nmea_logger.reset();

    if (backend_components->protected_task_manager) {
      backend_components->protected_task_manager->SetRoutePlanner(nullptr);
      backend_components->protected_task_manager.reset();
    }
  }

  delete task_manager;
  task_manager = nullptr;

#ifdef HAVE_NOAA
  delete noaa_store;
  noaa_store = nullptr;
#endif

  delete net_components;
  net_components = nullptr;

#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::Deinitialise();
#endif

  // Close the progress dialog
  LogString("Close Progress Dialog");
  operation.Hide();

  delete task_events;
  task_events = nullptr;

  // Destroy FlarmNet records
  DeinitTrafficGlobals();

  delete backend_components;
  backend_components = nullptr;

  delete data_components;
  data_components = nullptr;

  delete file_cache;
  file_cache = nullptr;

  LogString("Close Windows - main");
  main_window->Destroy();
  delete main_window;
  CommonInterface::main_window = nullptr;

  CloseLanguageFile();

  Display::RestoreOrientation();

  LogString("Finished shutdown");
}
