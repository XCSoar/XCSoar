/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "InputEvents.h"
#include "Atmosphere.h"
#include "Device/Geoid.h"
#include "Dialogs.h"
#include "ButtonLabel.hpp"
#include "Language.hpp"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "Math/FastMath.h"
#include "UtilsSystem.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "SettingsMap.hpp"
#include "Logger/Logger.hpp"
#include "Logger/NMEALogger.hpp"
#include "AirfieldDetails.h"
#include "Screen/Fonts.hpp"
#include "DeviceBlackboard.hpp"
#include "MapWindow.hpp"
#include "Marks.hpp"
#include "Device/device.hpp"
#include "Topology/TopologyStore.hpp"
#include "Topology/TopologyGlue.hpp"
#include "Audio/VarioSound.h"
#include "Screen/Graphics.hpp"
#include "Screen/Busy.hpp"
#include "Polar/Loader.hpp"
#include "Persist.hpp"
#include "MainWindow.hpp"
#include "resource.h"
#include "GlideComputer.hpp"
#include "DrawThread.hpp"
#include "StatusMessage.hpp"
#include "CalculationThread.hpp"
#include "InstrumentThread.hpp"
#include "Replay/Replay.hpp"
#include "ResourceLoader.hpp"
#include "LocalPath.hpp"
#include "IO/FileCache.hpp"
#include "Hardware/AltairControl.hpp"
#include "Hardware/Display.hpp"
#include "Compiler.h"

#include "Waypoint/Waypoints.hpp"
#include "WayPoint/WayPointGlue.hpp"

#include "Airspace/AirspaceWarningManager.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"

#include "Task/TaskManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideComputerInterface.hpp"
#include "ProgressGlue.hpp"
#include "Pages.hpp"

FileCache *file_cache;
Marks *marks;
TopologyStore *topology;
RasterTerrain *terrain;
RasterWeather RASP;

DrawThread *draw_thread;
CalculationThread *calculation_thread;
InstrumentThread *instrument_thread;

Logger logger;
Replay replay;

Waypoints way_points;

GlideComputerTaskEvents task_events;

static TaskManager task_manager(task_events, way_points);

ProtectedTaskManager protected_task_manager(task_manager,
                                            XCSoarInterface::SettingsComputer(),
                                            task_events);

AIRCRAFT_STATE ac_state;

Airspaces airspace_database;

AirspaceWarningManager airspace_warning(airspace_database,
                                        ac_state,
                                        task_manager);

ProtectedAirspaceWarningManager airspace_warnings(airspace_warning);

GlideComputer glide_computer(protected_task_manager, airspace_warnings,
                             task_events);

#ifdef GNAV
AltairControl altair_control;
#endif

bool
XCSoarInterface::LoadProfile()
{
  if (Profile::use_files() && !dlgStartupShowModal())
    return false;

  Profile::Load();
  Profile::Use();

  ProgressGlue::Create(_("Initialising"));

  return true;
}

static void
LoadDisplayOrientation()
{
  if (!Display::RotateSupported())
    return;

  Display::orientation orientation = Profile::GetDisplayOrientation();
  if (orientation == Display::ORIENTATION_DEFAULT)
    return;

  if (!Display::Rotate(orientation)) {
    LogStartUp(_T("Display rotation failed"));
    return;
  }

  LogStartUp(_T("Display rotated"));

  XCSoarInterface::main_window.Initialise();

  RECT rc = SystemWindowSize();
  XCSoarInterface::main_window.fast_move(rc.left, rc.top,
                                         rc.right - rc.left, rc.bottom - rc.top);

  /* force the progress dialog to update its layout */
  ProgressGlue::Close();
  ProgressGlue::Create(NULL);
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
  static bool first = true;
  if (!first)
    return;

  first = false;

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

  // Create default task if none exists
  LogStartUp(_T("Create default task"));
  task_manager.default_task(Basic().Location);

  SetSettingsComputer().enable_olc = true;
  protected_task_manager.task_load_default();

  task_manager.resume();

  LogStartUp(_T("CloseProgressDialog"));
  ProgressGlue::Close();

  main_window.full_screen();
  InfoBoxManager::SetDirty();

  TriggerGPSUpdate();

  status_messages.Startup(false);
#ifdef _INPUTDEBUG_
  InputEvents::showErrors();
#endif
}

/**
 * "Boots" up XCSoar
 * @param hInstance Instance handle
 * @param lpCmdLine Command line string
 * @return True if bootup successful, False otherwise
 */
bool
XCSoarInterface::Startup(HINSTANCE hInstance)
{
  // Set the application title to "XCSoar"
  TCHAR szTitle[] = _T("XCSoar");

  // Store instance handle in our global variable
#ifdef WIN32
  ResourceLoader::Init(hInstance);
#endif

  hInst = hInstance;

  //If "XCSoar" is already running, stop this instance
  if (MainWindow::find(szTitle))
    return false;

  // Send the SettingsMap to the DeviceBlackboard
  SendSettingsMap();

  // Register window classes
  PaintWindow::register_class(hInst);
  MainWindow::register_class(hInst);
  MapWindow::register_class(hInst);

  // Send the SettingsMap to the DeviceBlackboard
  SendSettingsMap();

  // Creates the main window
  LogStartUp(_T("Create main window"));
  RECT WindowSize = SystemWindowSize();
  main_window.set(szTitle,
                  WindowSize.left, WindowSize.top,
                  WindowSize.right, WindowSize.bottom);
  if (!main_window.defined())
    return false;

  main_window.Initialise();

#ifdef SIMULATOR_AVAILABLE
  // prompt for simulator if not set by command line argument "-simulator" or "-fly"
  if (!sim_set_in_cmd_line_flag)
    global_simulator_flag = dlgSimulatorPromptShowModal();
#endif

  if (!LoadProfile())
    return false;

  LoadDisplayOrientation();

  main_window.InitialiseConfigured();

  TCHAR path[MAX_PATH];
  LocalPath(path, _T("cache"));
  file_cache = new FileCache(path);

  Graphics::InitialiseConfigured(SettingsMap());

  ReadLanguageFile();

  status_messages.LoadFile();
  InputEvents::readFile();

  // Initialize DeviceBlackboard
  device_blackboard.Initialise();

  // Initialize Marks
  marks = new Marks();

  // Show the main and map windows
  LogStartUp(_T("Create map window"));
  main_window.show();
  main_window.map.show();

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
  task_manager.reset();

  glide_computer.Initialise();

  // Load the EGM96 geoid data
  OpenGeoid();

  GlidePolar gp = task_manager.get_glide_polar();
  if (LoadPolarById(SettingsComputer(), gp))
    task_manager.set_glide_polar(gp);

  task_manager.set_olc_rules(SettingsComputer().olc_rules);

  // Read the topology file(s)
  topology = new TopologyStore();
  LoadConfiguredTopology(*topology);

  // Read the terrain file
  ProgressGlue::Create(_("Loading Terrain File..."));
  LogStartUp(_T("OpenTerrain"));
  terrain = RasterTerrain::OpenTerrain(file_cache);

  // Read the waypoint files
  WayPointGlue::ReadWaypoints(way_points, terrain);

  // Read and parse the airfield info file
  ReadAirfieldFile(way_points);

  // Set the home waypoint
  WayPointGlue::SetHome(way_points, terrain, SetSettingsComputer(),
                        false, true);

  // ReSynchronise the blackboards here since SetHome touches them
  ReadBlackboardBasic(device_blackboard.Basic());

  if (terrain != NULL) {
    ProgressGlue::Create(_("Loading Terrain File..."));
    RasterTerrain::UnprotectedLease lease(*terrain);
    lease->SetViewCenter(Basic().Location);
  }

  // Scan for weather forecast
  ProgressGlue::Create(_("Scanning weather forecast"));
  LogStartUp(_T("RASP load"));
  RASP.ScanAll(Basic().Location);

  // Reads the airspace files
  ReadAirspace(airspace_database, terrain, Basic().pressure);

  const AIRCRAFT_STATE aircraft_state =
    ToAircraftState(device_blackboard.Basic());
  airspace_warning.reset(aircraft_state);
  airspace_warning.set_config(XCSoarInterface::SettingsComputer().airspace_warnings);

  // Read the FLARM details file
  FlarmDetails::Load();

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
  ProgressGlue::Create(_("Starting devices"));
  devStartup();

/*
  -- Reset polar in case devices need the data
  LogStartUp(_T("GlidePolar::UpdatePolar"));
  GlidePolar::UpdatePolar(true, SettingsComputer());

  This should be done inside devStartup if it is really required
*/

  ProgressGlue::Create(_("Initialising display"));

  main_window.map.set_way_points(&way_points);
  main_window.map.set_task(&protected_task_manager);
  main_window.map.set_airspaces(&airspace_database, &airspace_warnings);

  main_window.map.set_topology(topology);
  main_window.map.set_terrain(terrain);
  main_window.map.set_weather(&RASP);
  main_window.map.set_marks(marks);

  // Finally ready to go.. all structures must be present before this.

  // Create the drawing thread
  LogStartUp(_T("CreateDrawingThread"));
  draw_thread = new DrawThread(main_window.map, main_window.flarm,
                               main_window.ta);
  draw_thread->start();

  // Show the infoboxes
  LogStartUp(_T("ShowInfoBoxes"));
  InfoBoxManager::Show();

  // Create the calculation thread
  LogStartUp(_T("CreateCalculationThread"));
  CreateCalculationThread();

  // Initialise the airspace warning dialog
  LogStartUp(_T("dlgAirspaceWarningInit"));
  dlgAirspaceWarningInit(main_window);

  // Find unique ID of this PDA
  ReadAssetNumber();

  LogStartUp(_T("ProgramStarted"));

  // Give focus to the map
  main_window.map.set_focus();

  Pages::LoadFromProfile();

  // Start calculation thread
  calculation_thread->start();

  // Start instrument thread
  if (instrument_thread != NULL)
    instrument_thread->start();

  globalRunningEvent.trigger();
  calculation_thread->resume();
  draw_thread->resume();

  return true;
}

void
XCSoarInterface::Shutdown(void)
{
  gcc_unused ScopeBusyIndicator busy;

  // Show progress dialog
  ProgressGlue::Create(_("Shutdown, please wait..."));

  // Log shutdown information
  LogStartUp(_T("Entering shutdown..."));
  StartupLogFreeRamAndStorage();

  // Turn off all displays
  globalRunningEvent.reset();

  // Stop logger and save igc file
  ProgressGlue::Create(_("Shutdown, saving logs..."));
  logger.guiStopLogger(Basic(), true);

  // Save settings to profile
  ProgressGlue::Create(_("Shutdown, saving Profile/Profile..."));
  Profile::Save();

  // Stop sound
  LogStartUp(_T("SaveSoundSettings"));
  Profile::SetSoundSettings();

#ifndef DISABLEAUDIOVARIO
  //  VarioSound_EnableSound(false);
  //  VarioSound_Close();
#endif

  ProgressGlue::Create(_("Shutdown, please wait..."));

  // Stop threads
  LogStartUp(_T("CloseDrawingThread"));
  closeTriggerEvent.trigger();
  draw_thread->stop();
  calculation_thread->stop();

  if (instrument_thread != NULL)
    instrument_thread->stop();

  // Wait for the calculations thread to finish
  calculation_thread->join();
  LogStartUp(_T("- calculation thread returned"));

  //  Wait for the instruments thread to finish
  if (instrument_thread != NULL)
    instrument_thread->join();
  LogStartUp(_T("- instrument thread returned"));

  //  Wait for the drawing thread to finish
  draw_thread->join();
  LogStartUp(_T("- draw thread returned"));
  delete draw_thread;

  // Close the AirspaceWarning dialog if still open
  LogStartUp(_T("dlgAirspaceWarningDeInit"));
  dlgAirspaceWarningDeInit();

  // Save the task for the next time
  ProgressGlue::Create(_("Shutdown, saving task..."));

  LogStartUp(_T("Save default task"));
  protected_task_manager.task_save_default();

  // Clear airspace database
  LogStartUp(_T("Close airspace"));
  airspace_warnings.clear();
  airspace_database.clear();

  // Clear waypoint database
  LogStartUp(_T("Close waypoints"));
  way_points.clear();

  ProgressGlue::Create(_("Shutdown, please wait..."));

  // Clear weather database
  LogStartUp(_T("CloseRASP"));
  RASP.Close();

  // Clear terrain database
  LogStartUp(_T("CloseTerrain"));

  delete terrain;

  LogStartUp(_T("CloseTopology"));
  delete topology;

  delete marks;

  // Close any device connections
  devShutdown();

  RawLoggerShutdown();

  // Save everything in the persistent memory file
  SaveCalculationsPersist(Basic(), Calculated());

  // Clear the FLARM database
  FlarmDetails::Reset();

  // Kill windows
  LogStartUp(_T("Destroy Info Boxes"));
  InfoBoxManager::Destroy();

  LogStartUp(_T("Destroy Button Labels"));
  ButtonLabel::Destroy();

  // Close the progress dialog
  LogStartUp(_T("Close Progress Dialog"));
  ProgressGlue::Close();

  // Clear the EGM96 database
  CloseGeoid();

  delete file_cache;

  LogStartUp(_T("Close Windows - main "));
  main_window.reset();

  RestoreDisplayOrientation();

  StartupLogFreeRamAndStorage();

  LogStartUp(_T("Finished shutdown"));
}
