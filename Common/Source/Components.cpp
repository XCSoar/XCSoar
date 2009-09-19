/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "Waypointparser.h"
#include "Airspace.h"
#include "ButtonLabel.hpp"
#include "SnailTrail.hpp"
#include "Message.h"
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
#include "TerrainRenderer.h"
#include "Audio/VarioSound.h"
#include "Screen/Graphics.hpp"
#include "Calculations.h"
#include "Polar/Historical.hpp"
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

WayPointList way_points;
Marks *marks;
TopologyStore *topology;
RasterTerrain terrain;
RasterWeather RASP;
GlideComputer glide_computer;
DrawThread *draw_thread;
CalculationThread *calculation_thread;
InstrumentThread *instrument_thread;
Logger logger; // global

/////////////////////////////////////////////////////////////////////////////////

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

  // Registery (early)

  if (ask) {
    Profile::RestoreRegistry();
    Profile::ReadRegistrySettings();

    //    CreateProgressDialog(gettext(TEXT("Initialising")));

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
#ifdef _SIM_
  StartupStore(TEXT("GCE_STARTUP_SIMULATOR\n"));
  InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
#else
  StartupStore(TEXT("GCE_STARTUP_REAL\n"));
  InputEvents::processGlideComputer(GCE_STARTUP_REAL);
#endif

  // Create default task if none exists
  StartupStore(TEXT("Create default task\n"));
  task.DefaultTask(SettingsComputer());

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


void XCSoarInterface::StartupInfo() {
#ifdef CREDITS_FIVV
  CreateProgressDialog(gettext(TEXT("Special ITA version")));
  Sleep(1000);
#endif
#ifdef PNA // VENTA-ADDON
  TCHAR sTmp[MAX_PATH];
  _stprintf(sTmp, TEXT("PNA MODEL=%s (%d)"), GlobalModelName, GlobalModelType);
  CreateProgressDialog(sTmp); Sleep(3000);
#endif // non PNA
#ifdef _SIM_
  CreateProgressDialog(TEXT("SIMULATION")); Sleep(2000);
#endif
#ifdef PNA
  if ( SetBacklight() == true )
    CreateProgressDialog(TEXT("AUTOMATIC BACKLIGHT CONTROL"));
  else
    CreateProgressDialog(TEXT("NO BACKLIGHT CONTROL"));
  Sleep(3000);

  // this should work ok for all pdas as well
  if ( SetSoundVolume() == true )
    CreateProgressDialog(TEXT("AUTOMATIC SOUND LEVEL CONTROL"));
  else
    CreateProgressDialog(TEXT("NO SOUND LEVEL CONTROL"));
  Sleep(3000);
#endif
}


bool XCSoarInterface::Startup(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
  TCHAR szTitle[MAX_LOADSTRING];                        // The title bar text

  hInst = hInstance;            // Store instance handle in our global variable

  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

  //If it is already running, then focus on the window
  if (MainWindow::find(szTitle))
    return false;

  SendSettingsMap();

  PaintWindow::register_class(hInst);
  MainWindow::register_class(hInst);
  MapWindow::register_class(hInst);

  // other startup...

  InitSineTable();
  PreloadInitialisation(true);

  SendSettingsMap();

  StartupStore(TEXT("Create main window\n"));

  RECT WindowSize = SystemWindowSize();
  main_window.set(szTitle,
		  WindowSize.left, WindowSize.top,
		  WindowSize.right, WindowSize.bottom);

  if (!main_window.defined()) {
    return false;
  }
  main_window.install_timer();

  device_blackboard.Initialise();
  ///////////////////////////////////////////////////////
  /// 
  marks = new Marks("xcsoar-marks");
  topology = new TopologyStore(marks->GetTopology());

  ///////////////////////////////////////////////////////
  //// create map window

  StartupStore(TEXT("Create map window\n"));

  ///////////////////////////////////////////////////////
  // initial show

  main_window.show();
  main_window.update();

  main_window.map.show();
  main_window.map.update();

  ///////////////////////////////////////////////////////
  // other initialisation...

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

  // Initialise main blackboard data

  task.ClearTask();
  glide_computer.Initialise();
  logger.LinkGRecordDLL(); // try to link DLL if it exists
  OpenGeoid();

  PreloadInitialisation(false);
  ////////////////////////////////////////////////////////

  Profile::LoadWindFromRegistry();
  CalculateNewPolarCoef();
  StartupStore(TEXT("GlidePolar::UpdatePolar\n"));
  GlidePolar::UpdatePolar(false, SettingsComputer());

  StartupInfo();

  topology->Open();
  terrain.OpenTerrain();

  ReadWayPoints(way_points, terrain);

  ReadAirfieldFile();
  SetHome(way_points, terrain, SetSettingsComputer(), false, true);

  // need to re-synchronise blackboards here since SetHome touches them
  ReadBlackboardBasic(device_blackboard.Basic());

  terrain.ServiceFullReload(Basic().Location);

  CreateProgressDialog(gettext(TEXT("Scanning weather forecast")));
  StartupStore(TEXT("RASP load\n"));
  RASP.ScanAll(Basic().Location);

  ReadAirspace();
  SortAirspace();

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

  CreateProgressDialog(gettext(TEXT("Starting devices")));
  devStartup(lpCmdLine);

  // re-set polar in case devices need the data
  StartupStore(TEXT("GlidePolar::UpdatePolar\n"));
  GlidePolar::UpdatePolar(true, SettingsComputer());

  CreateProgressDialog(gettext(TEXT("Initialising display")));

  // Finally ready to go.. all structures must be present before this.
  StartupStore(TEXT("CreateDrawingThread\n"));
  draw_thread = new DrawThread(main_window.map, main_window.flarm);
  draw_thread->start();

  StartupStore(TEXT("ShowInfoBoxes\n"));
  InfoBoxManager::Show();

  StartupStore(TEXT("CreateCalculationThread\n"));
  CreateCalculationThread();

  StartupStore(TEXT("dlgAirspaceWarningInit\n"));
  dlgAirspaceWarningInit();

  // find unique ID of this PDA
  ReadAssetNumber();

  // Da-da, start everything now
  StartupStore(TEXT("ProgramStarted\n"));

  // map gets initial focus
  main_window.map.set_focus();

  // start threads running
  calculation_thread->start();
  instrument_thread->start();

  globalRunningEvent.trigger();

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
  task.ResumeAbortTask(SettingsComputer(), -1); // turn off abort if it was on.
  StartupStore(TEXT("Save default task\n"));
  task.SaveDefaultTask();
  StartupStore(TEXT("Clear task data\n"));
  task.ClearTask();
  StartupStore(TEXT("Close airspace\n"));
  CloseAirspace();

  StartupStore(TEXT("Close waypoints\n"));
  way_points.clear();

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  StartupStore(TEXT("CloseTerrainTopology\n"));

  RASP.Close();
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

  DeleteAirspace();

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


