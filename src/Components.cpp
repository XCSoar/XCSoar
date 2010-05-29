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
#include "Profile.hpp"
#include "Interface.hpp"
#include "ProfileKeys.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "InfoBox.hpp"
#include "InfoBoxManager.hpp"
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
#include "UtilsFLARM.hpp"
#include "SettingsUser.hpp"
#include "Logger/Logger.hpp"
#include "AirfieldDetails.h"
#include "Screen/Fonts.hpp"
#include "DeviceBlackboard.hpp"
#include "MapWindow.hpp"
#include "Marks.hpp"
#include "Device/device.hpp"
#include "TopologyStore.h"
#include "Topology.h"
#include "Audio/VarioSound.h"
#include "Screen/Graphics.hpp"
#include "Polar/Loader.hpp"
#include "Persist.hpp"
#include "MainWindow.hpp"
#include "resource.h"
#include "GlideComputer.hpp"
#include "DrawThread.hpp"
#include "StatusMessage.hpp"
#include "options.h"
#include "CalculationThread.hpp"
#include "InstrumentThread.hpp"
#include "ReplayLoggerGlue.hpp"

#include "Waypoint/Waypoints.hpp"
#include "WayPoint/WayPointGlue.hpp"

#include "Airspace/AirspaceWarningManager.hpp"
#include "Airspace/Airspaces.hpp"
#include "AirspaceClientUI.hpp"
#include "AirspaceClientCalc.hpp"
#include "AirspaceGlue.hpp"

#include "Task/TaskManager.hpp"
#include "TaskClientUI.hpp"
#include "TaskClientMap.hpp"
#include "TaskClientCalc.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideComputerInterface.hpp"

Marks *marks;
TopologyStore *topology;
RasterTerrain terrain;
RasterWeather RASP;

DrawThread *draw_thread;
CalculationThread *calculation_thread;
InstrumentThread *instrument_thread;

Logger logger;
ReplayLoggerGlue replay;

Waypoints way_points;

GlideComputerTaskEvents task_events;

TaskManager task_manager(task_events,
                         way_points);
TaskClientCalc task_calc(task_manager);

TaskClientUI task_ui(task_manager, 
                     XCSoarInterface::SettingsComputer(),
                     task_events);
/// @todo JMW have ui-specific task_events! Don't use glide computer's events

AIRCRAFT_STATE ac_state;

Airspaces airspace_database;

AirspaceWarningManager airspace_warning(airspace_database,
                                        ac_state,
                                        task_manager);

AirspaceClientUI airspace_ui(airspace_database,
                             airspace_warning);

AirspaceClientCalc airspace_calc(airspace_database,
                                 airspace_warning);

GlideComputer glide_computer(task_calc,
                             airspace_calc,
                             task_events);

void
XCSoarInterface::PreloadInitialisation(bool ask)
{
  Profile::Set(TEXT("XCV"), 1);

  if (!ask) {
    Profile::Load();
    Profile::Use();
  } else {
    if (Profile::use_files())
      dlgStartupShowModal();

    Profile::Load();
    Profile::Use();

    CreateProgressDialog(gettext(TEXT("Initialising")));
  }
}

void
XCSoarInterface::AfterStartup()
{
  static bool first = true;
  if (!first)
    return;

  first = false;

  LogStartUp(TEXT("ProgramStarted = 3"));
  StartupLogFreeRamAndStorage();

  status_messages.Startup(true);

  if (is_simulator()) {
    LogStartUp(TEXT("GCE_STARTUP_SIMULATOR"));
    InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
  } else {
    LogStartUp(TEXT("GCE_STARTUP_REAL"));
    InputEvents::processGlideComputer(GCE_STARTUP_REAL);
  }

  // Create default task if none exists
  LogStartUp(TEXT("Create default task"));
  task_manager.default_task(Basic().Location);

  SetSettingsComputer().enable_olc = true;
  task_ui.task_load_default();

  task_manager.resume();

  LogStartUp(TEXT("CloseProgressDialog"));
  CloseProgressDialog();

  main_window.full_screen();
  InfoBoxManager::SetDirty(true);

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
XCSoarInterface::Startup(HINSTANCE hInstance, LPCTSTR lpCmdLine)
{
  // Set the application title to "XCSoar"
  TCHAR szTitle[] = _T("XCSoar");

  // Store instance handle in our global variable
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

  PreloadInitialisation(false);

  // Send the SettingsMap to the DeviceBlackboard
  SendSettingsMap();

  // Creates the main window
  LogStartUp(TEXT("Create main window"));
  RECT WindowSize = SystemWindowSize();
  main_window.set(szTitle,
                  WindowSize.left, WindowSize.top,
                  WindowSize.right, WindowSize.bottom);

  if (!main_window.defined())
    return false;

#ifdef SIMULATOR_AVAILABLE
  // prompt for simulator if not set by command line argument "-simulator" or "-fly"
  if (!sim_set_in_cmd_line_flag) {
    dlgSimulatorPromptShowModal();
  }
#endif

  PreloadInitialisation(true);

#ifndef DEBUG_TRANSLATIONS
  ReadLanguageFile();
#endif

  status_messages.LoadFile();
  InputEvents::readFile();

  // Initialize DeviceBlackboard
  device_blackboard.Initialise();

  // Initialize Marks
  marks = new Marks("xcsoar-marks", SettingsComputer());
  topology = new TopologyStore(marks->GetTopology());

  // Show the main and map windows
  LogStartUp(TEXT("Create map window"));
  main_window.show();
  main_window.map.show();

#ifdef HAVE_ACTIVATE_INFO
  SHSetAppKeyWndAssoc(VK_APP1, main_window);
  SHSetAppKeyWndAssoc(VK_APP2, main_window);
  SHSetAppKeyWndAssoc(VK_APP3, main_window);
  SHSetAppKeyWndAssoc(VK_APP4, main_window);
  // Typical Record Button
  //	Why you can't always get this to work
  //	http://forums.devbuzz.com/m_1185/mpage_1/key_/tm.htm
  //	To do with the fact it is a global hotkey, but you can with code above
  //	Also APPA is record key on some systems
  SHSetAppKeyWndAssoc(VK_APP5, main_window);
  SHSetAppKeyWndAssoc(VK_APP6, main_window);
#endif

  // Initialize main blackboard data
  task_manager.reset();

  glide_computer.Initialise();

  // Load the EGM96 geoid data
  OpenGeoid();

  Profile::GetWind();

  GlidePolar gp = task_manager.get_glide_polar();
  if (LoadPolarById(SettingsComputer(), gp))
    task_manager.set_glide_polar(gp);

  // Read the topology file(s)
  topology->Open();

  // Read the terrain file
  CreateProgressDialog(gettext(TEXT("Loading Terrain File...")));
  LogStartUp(TEXT("OpenTerrain"));
  terrain.OpenTerrain();

  CreateProgressDialog(gettext(TEXT("Loading way points...")));

  // Read the waypoint files
  WayPointGlue::ReadWaypoints(way_points, &terrain);

  // Read and parse the airfield info file
  ReadAirfieldFile();

  // Set the home waypoint
  WayPointGlue::SetHome(way_points, terrain, SetSettingsComputer(),
                        false, true);

  // ReSynchronise the blackboards here since SetHome touches them
  ReadBlackboardBasic(device_blackboard.Basic());

  CreateProgressDialog(gettext(TEXT("Loading Terrain File...")));
  terrain.ServiceFullReload(Basic().Location);

  // Scan for weather forecast
  CreateProgressDialog(gettext(TEXT("Scanning weather forecast")));
  LogStartUp(TEXT("RASP load"));
  RASP.ScanAll(Basic().Location);

  // Reads the airspace files
  ReadAirspace(airspace_ui, &terrain, Basic().pressure);

  const AIRCRAFT_STATE aircraft_state =
    ToAircraftState(device_blackboard.Basic());
  airspace_warning.reset(aircraft_state);

  // Read the FLARM details file
  OpenFLARMDetails();

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
  CreateProgressDialog(gettext(TEXT("Starting devices")));
  devStartup(lpCmdLine);

/*
  -- Reset polar in case devices need the data
  LogStartUp(TEXT("GlidePolar::UpdatePolar"));
  GlidePolar::UpdatePolar(true, SettingsComputer());

  This should be done inside devStartup if it is really required
*/

  CreateProgressDialog(gettext(TEXT("Initialising display")));

  main_window.map.set_way_points(&way_points);
  main_window.map.set_task(&task_ui);
  main_window.map.set_airspaces(&airspace_ui);

  main_window.map.set_topology(topology);
  main_window.map.set_terrain(&terrain);
  main_window.map.set_weather(&RASP);
  main_window.map.set_marks(marks);

  // Finally ready to go.. all structures must be present before this.

  // Create the drawing thread
  LogStartUp(TEXT("CreateDrawingThread"));
  draw_thread = new DrawThread(main_window.map, main_window.flarm);
  draw_thread->start();

  // Show the infoboxes
  LogStartUp(TEXT("ShowInfoBoxes"));
  InfoBoxManager::Show();

  // Create the calculation thread
  LogStartUp(TEXT("CreateCalculationThread"));
  CreateCalculationThread();

  // Initialise the airspace warning dialog
  LogStartUp(TEXT("dlgAirspaceWarningInit"));
  dlgAirspaceWarningInit(main_window);

  // Find unique ID of this PDA
  ReadAssetNumber();

  LogStartUp(TEXT("ProgramStarted"));

  // Give focus to the map
  main_window.map.set_focus();

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
  // Show progress dialog
  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));
  StartHourglassCursor();

  // Log shutdown information
  LogStartUp(TEXT("Entering shutdown..."));
  StartupLogFreeRamAndStorage();

  // Turn off all displays
  globalRunningEvent.reset();

  // Stop logger and save igc file
  CreateProgressDialog(gettext(TEXT("Shutdown, saving logs...")));
  logger.guiStopLogger(Basic(), true);

  // Save settings to profile
  CreateProgressDialog(gettext(TEXT("Shutdown, saving profile...")));
  Profile::Save();

  // Stop sound
  LogStartUp(TEXT("SaveSoundSettings"));
  Profile::SetSoundSettings();

#ifndef DISABLEAUDIOVARIO
  //  VarioSound_EnableSound(false);
  //  VarioSound_Close();
#endif

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  // Stop threads
  LogStartUp(TEXT("CloseDrawingThread"));
  closeTriggerEvent.trigger();
  draw_thread->stop();
  calculation_thread->stop();

  if (instrument_thread != NULL)
    instrument_thread->stop();

  // Wait for the calculations thread to finish
  calculation_thread->join();
  LogStartUp(TEXT("- calculation thread returned"));

  //  Wait for the instruments thread to finish
  if (instrument_thread != NULL)
    instrument_thread->join();
  LogStartUp(TEXT("- instrument thread returned"));

  //  Wait for the drawing thread to finish
  draw_thread->join();
  LogStartUp(TEXT("- draw thread returned"));
  delete draw_thread;

  // Close the AirspaceWarning dialog if still open
  LogStartUp(TEXT("dlgAirspaceWarningDeInit"));
  dlgAirspaceWarningDeInit();

  // Save the task for the next time
  CreateProgressDialog(gettext(TEXT("Shutdown, saving task...")));

  LogStartUp(TEXT("Save default task"));
  task_ui.task_save_default();

  // Clear airspace database
  LogStartUp(TEXT("Close airspace"));
  CloseAirspace(airspace_ui);

  // Clear waypoint database
  LogStartUp(TEXT("Close waypoints"));
  way_points.clear();

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  // Clear weather database
  LogStartUp(TEXT("CloseRASP"));
  RASP.Close();

  // Clear terrain database
  LogStartUp(TEXT("CloseTerrain"));
  terrain.CloseTerrain();

  delete topology;
  delete marks;

  // Close any device connections
  devShutdown();

  // Save everything in the persistent memory file
  SaveCalculationsPersist(Basic(), Calculated());
#if (EXPERIMENTAL > 0)
  //  CalibrationSave();
#endif

  if (is_altair()) {
    LogStartUp(TEXT("Altair shutdown"));
    Sleep(2500);
    StopHourglassCursor();
    InputEvents::eventDLLExecute(TEXT("altairplatform.dll SetShutdown 1"));
    while (true)
      Sleep(100); // free time up for processor to perform shutdown
  }

  // Clear the FLARM database
  CloseFLARMDetails();

  // Kill windows
  LogStartUp(TEXT("Destroy Info Boxes"));
  InfoBoxManager::Destroy();

  LogStartUp(TEXT("Destroy Button Labels"));
  ButtonLabel::Destroy();

  // Kill graphics objects
  LogStartUp(TEXT("Delete Objects"));
  DeleteFonts();

  // Close the progress dialog
  LogStartUp(TEXT("Close Progress Dialog"));
  CloseProgressDialog();

  // Clear the EGM96 database
  CloseGeoid();

  LogStartUp(TEXT("Close Windows - main "));
  main_window.reset();

  LogStartUp(TEXT("Close Graphics"));
  MapGfx.Destroy();

#ifdef DEBUG_TRANSLATIONS
  LogStartUp(TEXT("Writing missing translations"));
  WriteMissingTranslations();
#endif

  StartupLogFreeRamAndStorage();

  LogStartUp(TEXT("Finished shutdown"));
  StopHourglassCursor();
}
