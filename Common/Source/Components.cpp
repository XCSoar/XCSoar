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
#include "Registry.hpp"
#include "Profile.hpp"
#include "Interface.hpp"
#include "UtilsProfile.hpp"
#include "Asset.hpp"
#include "InfoBox.h"
#include "InfoBoxManager.h"
#include "RasterTerrain.h"
#include "RasterWeather.h"
#include "InputEvents.h"
#include "Atmosphere.h"
#include "Device/Geoid.h"
#include "Dialogs.h"
#include "WayPointParser.h"
#include "AirspaceGlue.hpp"
#include "AirspaceWarning.h"
#include "AirspaceDatabase.hpp"
#include "ButtonLabel.hpp"
#include "SnailTrail.hpp"
#include "Language.hpp"
#include "Task.h"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "Math/FastMath.h"
#include "UtilsSystem.hpp"
#include "UtilsFLARM.hpp"
#include "SettingsUser.hpp"
#include "Logger.h"
#include "McReady.h"
#include "AirfieldDetails.h"
#include "Screen/Fonts.hpp"
#include "DeviceBlackboard.hpp"
#include "MapWindow.h"
#include "Marks.h"
#include "Device/device.h"
#include "TopologyStore.h"
#include "Topology.h"
#include "Audio/VarioSound.h"
#include "Screen/Graphics.hpp"
#include "Calculations.h"
#include "Polar/Loader.hpp"
#include "Persist.hpp"
#include "Device/Parser.h"
#include "MainWindow.hpp"
#include "resource.h"
#include "GlideComputer.hpp"
#include "DrawThread.hpp"
#include "StatusMessage.hpp"
#include "options.h"
#include "CalculationThread.hpp"
#include "InstrumentThread.hpp"
#include "WayPointList.hpp"
#if defined(__BORLANDC__)  // due to compiler bug
  #include "Polar/Polar.hpp"
#endif

WayPointList way_points;
AirspaceDatabase airspace_database;
Marks *marks;
TopologyStore *topology;
RasterTerrain terrain;
RasterWeather RASP;
GlideComputer glide_computer;
DrawThread *draw_thread;
CalculationThread *calculation_thread;
InstrumentThread *instrument_thread;
Logger logger; // global

void XCSoarInterface::PreloadInitialisation(bool ask) {
  if (ask) {
    #ifdef PNA
      CleanRegistry(); // VENTA2-FIX for PNA we can't delete all registries..by now
    #endif
  }

  SetToRegistry(TEXT("XCV"), 1);

#ifdef DEBUG_TRANSLATIONS
  ReadLanguageFile();
#endif

  // Registry (early)

  if (ask) {
    Profile::RestoreRegistry();
    Profile::ReadRegistrySettings();

    // CreateProgressDialog(gettext(TEXT("Initialising")));

  } else {
    dlgStartupShowModal();
    Profile::RestoreRegistry();
    Profile::ReadRegistrySettings();

    CreateProgressDialog(gettext(TEXT("Initialising")));
  }

  // Interface (before interface)
  if (!ask) {
#ifndef DEBUG_TRANSLATIONS
    ReadLanguageFile();
#endif
    status_messages.LoadFile();
    InputEvents::readFile();
  }

}


void XCSoarInterface::AfterStartup() {
  static bool first = true;
  if (!first) {
    return;
  }
  first = false;

  StartupStore(TEXT("ProgramStarted=3\n"));
  StartupLogFreeRamAndStorage();

  status_messages.Startup(true);

  if (is_simulator()) {
    StartupStore(TEXT("GCE_STARTUP_SIMULATOR\n"));
    InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
  } else {
    StartupStore(TEXT("GCE_STARTUP_REAL\n"));
    InputEvents::processGlideComputer(GCE_STARTUP_REAL);
  }

  // Create default task if none exists
  StartupStore(TEXT("Create default task\n"));
  task.DefaultTask(SettingsComputer(), Basic());

  StartupStore(TEXT("CloseProgressDialog\n"));
  CloseProgressDialog();

  main_window.full_screen();
  InfoBoxManager::SetDirty(true);
  TriggerAll();

  status_messages.Startup(false);
#ifdef _INPUTDEBUG_
  InputEvents::showErrors();
#endif
}

// NEWTASK
#ifdef NEWTASK
extern int test_newtask(int test_num);
#endif


/**
 * "Boots" up XCSoar
 * @param hInstance Instance handle
 * @param lpCmdLine Command line string
 * @return True if bootup successful, False otherwise
 */
bool XCSoarInterface::Startup(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
  // The title bar text
  TCHAR szTitle[MAX_LOADSTRING];

  // Store instance handle in our global variable
  hInst = hInstance;

  // IDS_APP_TITLE = XCSoar (see XCSoar.rc)
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

  //If it is already running, then focus on the window
  if (MainWindow::find(szTitle))
    return false;

  // Send the SettingsMap to the DeviceBlackboard
  SendSettingsMap();

  // Register window classes
  PaintWindow::register_class(hInst);
  MainWindow::register_class(hInst);
  MapWindow::register_class(hInst);

  // Fill the fast(co)sine table
  InitSineTable();

  PreloadInitialisation(true);

  // Send the SettingsMap to the DeviceBlackboard
  SendSettingsMap();

  // Creates the main window
  StartupStore(TEXT("Create main window\n"));
  RECT WindowSize = SystemWindowSize();
  main_window.set(szTitle,
		  WindowSize.left, WindowSize.top,
		  WindowSize.right, WindowSize.bottom);

  if (!main_window.defined()) {
    return false;
  }
  main_window.install_timer();

  // Initialize DeviceBlackboard
  device_blackboard.Initialise();

  // Initialize Marks
  marks = new Marks("xcsoar-marks", SettingsComputer());
  topology = new TopologyStore(marks->GetTopology());

  // Show the main and map windows
  StartupStore(TEXT("Create map window\n"));
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
  task.ClearTask();
  glide_computer.Initialise();
  logger.LinkGRecordDLL(); // try to link DLL if it exists

  // Load the EGM96 geoid data
  OpenGeoid();

  PreloadInitialisation(false);

  Profile::LoadWindFromRegistry();

  // TODO TB: seems to be out of date?!
  LoadPolarById(POLARID, polar);

  // Calculate polar-related data and saves it to the cache
  StartupStore(TEXT("GlidePolar::UpdatePolar\n"));
  GlidePolar::UpdatePolar(false, SettingsComputer());

  // Read the topology file(s)
  topology->Open();

  // Read the terrain file
  CreateProgressDialog(gettext(TEXT("Loading Terrain File...")));
  SetProgressStepSize(2);
  StartupStore(TEXT("OpenTerrain\n"));
  terrain.OpenTerrain();

  // Read the waypoint files
  ReadWayPoints(way_points, &terrain);

  // Read and parse the airfield info file
  ReadAirfieldFile();

  // Set the home waypoint
  SetHome(way_points, &terrain, SetSettingsComputer(), false, true);

  // ReSynchronise the blackboards here since SetHome touches them
  ReadBlackboardBasic(device_blackboard.Basic());

  CreateProgressDialog(gettext(TEXT("Loading Terrain File...")));
  terrain.ServiceFullReload(Basic().Location);

  // Scan for weather forecast
  CreateProgressDialog(gettext(TEXT("Scanning weather forecast")));
  StartupStore(TEXT("RASP load\n"));
  RASP.ScanAll(Basic().Location);

  // Reads the airspace files
  ReadAirspace(airspace_database, &terrain);
  // Sorts the airspaces by priority
  SortAirspace(airspace_database);

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

  // Reset polar in case devices need the data
  StartupStore(TEXT("GlidePolar::UpdatePolar\n"));
  GlidePolar::UpdatePolar(true, SettingsComputer());

  CreateProgressDialog(gettext(TEXT("Initialising display")));

  main_window.map.set_way_points(&way_points);
  main_window.map.set_task(&task);
  main_window.map.set_airspaces(&airspace_database);
  main_window.map.set_topology(topology);
  main_window.map.set_terrain(&terrain);
  main_window.map.set_weather(&RASP);
  main_window.map.set_marks(marks);
  main_window.map.set_snail_trail(&glide_computer.GetSnailTrail());
  main_window.map.set_olc(&glide_computer.GetOLC());

  // Finally ready to go.. all structures must be present before this.

  // Create the drawing thread
  StartupStore(TEXT("CreateDrawingThread\n"));
  draw_thread = new DrawThread(main_window.map, main_window.flarm);
  draw_thread->start();

  // Show the infoboxes
  StartupStore(TEXT("ShowInfoBoxes\n"));
  InfoBoxManager::Show();

  // Create the calculation thread
  StartupStore(TEXT("CreateCalculationThread\n"));
  CreateCalculationThread();

#ifdef NEWTASK  
  { // NEWTASK
    PeriodClock t;
    t.reset(); t.update();
    CreateProgressDialog(gettext(TEXT("Running test 0")));
    test_newtask(0);
    StartupStore(TEXT("test 0 %d\n"),t.elapsed());

    /*
    t.update();
    CreateProgressDialog(gettext(TEXT("Running test 1")));
    test_newtask(1);
    StartupStore(TEXT("test 1 %d\n"),t.elapsed());

    t.update();
    CreateProgressDialog(gettext(TEXT("Running test 2")));
    test_newtask(2);
    StartupStore(TEXT("test 2 %d\n"),t.elapsed());

    t.update();
    CreateProgressDialog(gettext(TEXT("Running test 3")));
    test_newtask(3);
    StartupStore(TEXT("test 3 %d\n"),t.elapsed());

    t.update();
    CreateProgressDialog(gettext(TEXT("Running test 4")));
    test_newtask(4);
    StartupStore(TEXT("test 4 %d\n"),t.elapsed());
    */
    CreateProgressDialog(gettext(TEXT("test complete")));
  }
#endif

  // Initialise the airspace warning dialog
  StartupStore(TEXT("dlgAirspaceWarningInit\n"));
  dlgAirspaceWarningInit();

  // Find unique ID of this PDA
  ReadAssetNumber();

  StartupStore(TEXT("ProgramStarted\n"));

  // Give focus to the map
  main_window.map.set_focus();

  // Start calculation thread
  calculation_thread->start();

  // Start instrument thread
  instrument_thread->start();

  globalRunningEvent.trigger();
  calculation_thread->resume();
  draw_thread->resume();

  return true;
}


void XCSoarInterface::Shutdown(void) {
  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));
  StartHourglassCursor();

  StartupStore(TEXT("Entering shutdown...\n"));
  StartupLogFreeRamAndStorage();

  // turn off all displays
  globalRunningEvent.reset();

  StartupStore(TEXT("dlgAirspaceWarningDeInit\n"));
  dlgAirspaceWarningDeInit();

  CreateProgressDialog(gettext(TEXT("Shutdown, saving logs...")));
  // stop logger
  logger.guiStopLogger(Basic(),true);

  CreateProgressDialog(gettext(TEXT("Shutdown, saving profile...")));
  // Save settings
  Profile::StoreRegistry();

  // Stop sound

  StartupStore(TEXT("SaveSoundSettings\n"));
  Profile::SaveSoundSettings();

#ifndef DISABLEAUDIOVARIO
  //  VarioSound_EnableSound(false);
  //  VarioSound_Close();
#endif

  // Stop drawing
  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  StartupStore(TEXT("CloseDrawingThread\n"));
  closeTriggerEvent.trigger();

  calculation_thread->join();
  StartupStore(TEXT("- calculation thread returned\n"));

  instrument_thread->join();
  StartupStore(TEXT("- instrument thread returned\n"));

  draw_thread->join();
  StartupStore(TEXT("- draw thread returned\n"));

  delete draw_thread;

  // Clear data

  CreateProgressDialog(gettext(TEXT("Shutdown, saving task...")));
  StartupStore(TEXT("Resume abort task\n"));
  task.ResumeAbortTask(SettingsComputer(), Basic(), -1); // turn off abort if it was on.
  StartupStore(TEXT("Save default task\n"));
  task.SaveDefaultTask();
  StartupStore(TEXT("Clear task data\n"));
  task.ClearTask();
  StartupStore(TEXT("Close airspace\n"));
  CloseAirspace(airspace_database);

  StartupStore(TEXT("Close waypoints\n"));
  way_points.clear();

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  StartupStore(TEXT("CloseTerrainTopology\n"));

  RASP.Close();

  StartupStore(TEXT("CloseTerrain\n"));
  terrain.CloseTerrain();

  delete topology;
  delete marks;

  devShutdown();

  SaveCalculationsPersist(Basic(),Calculated());
#if (EXPERIMENTAL > 0)
  //  CalibrationSave();
#endif

  #if defined(GNAV) && !defined(PCGNAV)
    StartupStore(TEXT("Altair shutdown\n"));
    Sleep(2500);
    StopHourglassCursor();
    InputEvents::eventDLLExecute(TEXT("altairplatform.dll SetShutdown 1"));
    while(1) {
      Sleep(100); // free time up for processor to perform shutdown
    }
  #endif

  CloseFLARMDetails();

  // Kill windows

  StartupStore(TEXT("Destroy Info Boxes\n"));
  InfoBoxManager::Destroy();

  StartupStore(TEXT("Destroy Button Labels\n"));
  ButtonLabel::Destroy();

  StartupStore(TEXT("Delete Objects\n"));

  // Kill graphics objects

  DeleteFonts();

  DeleteAirspace(airspace_database);

  StartupStore(TEXT("Close Progress Dialog\n"));

  CloseProgressDialog();

  CloseGeoid();

  StartupStore(TEXT("Close Windows - main \n"));
  main_window.reset();
  StartupStore(TEXT("Close Graphics\n"));
  MapGfx.Destroy();

#ifdef DEBUG_TRANSLATIONS
  StartupStore(TEXT("Writing missing translations\n"));
  WriteMissingTranslations();
#endif

  StartupLogFreeRamAndStorage();
  StartupStore(TEXT("Finished shutdown\n"));
  StopHourglassCursor();

}


