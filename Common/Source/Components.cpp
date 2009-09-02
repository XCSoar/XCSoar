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
#include "InfoBoxLayout.h"
#include "InfoBox.h"
#include "InfoBoxManager.h"
#include "RasterTerrain.h"
#include "RasterWeather.h"
#include "Gauge/GaugeCDI.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeVarioAltA.hpp"
#include "InputEvents.h"
#include "Atmosphere.h"
#include "Device/Geoid.h"
#include "Math/SunEphemeris.hpp"
#include "Dialogs.h"
#include "Waypointparser.h"
#include "Airspace.h"
#include "ButtonLabel.h"
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
#include "Blackboard.hpp"
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
#include "Screen/MainWindow.hpp"

GaugeVario *gauge_vario;
GaugeFLARM *gauge_flarm;
Marks *marks;
TopologyStore *topology;

MapWindow map_window;
NMEA_INFO     GPS_INFO;
DERIVED_INFO  CALCULATED_INFO;


/////////////////////////////////////////////////////////////////////////////////

static void PreloadInitialisation(bool ask) {
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
    RestoreRegistry();
    ReadRegistrySettings();
    Message::InitFile();

    //    CreateProgressDialog(gettext(TEXT("Initialising")));

  } else {
    dlgStartupShowModal();
    RestoreRegistry();
    ReadRegistrySettings();

    CreateProgressDialog(gettext(TEXT("Initialising")));
  }

  // Interface (before interface)
  if (!ask) {
#ifndef DEBUG_TRANSLATIONS
    ReadLanguageFile();
#endif
    Message::LoadFile();
    InputEvents::readFile();
  }

}


void AfterStartup() {
  static bool first = true;
  if (!first) {
    return;
  }
  first = false;
  StartupStore(TEXT("ProgramStarted=3\n"));
  StartupLogFreeRamAndStorage();

  Message::Startup(true);
#ifdef _SIM_
  StartupStore(TEXT("GCE_STARTUP_SIMULATOR\n"));
  InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
#else
  StartupStore(TEXT("GCE_STARTUP_REAL\n"));
  InputEvents::processGlideComputer(GCE_STARTUP_REAL);
#endif

  // Create default task if none exists
  StartupStore(TEXT("Create default task\n"));
  DefaultTask();

  StartupStore(TEXT("CloseProgressDialog\n"));
  CloseProgressDialog();

  main_window.full_screen();
  TriggerAll();
  InfoBoxManager::SetDirty(true);
  TriggerRedraws();

  Message::Startup(false);
#ifdef _INPUTDEBUG_
  InputEvents::showErrors();
#endif
}


void StartupInfo() {
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


bool Startup(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
  TCHAR szTitle[MAX_LOADSTRING];                        // The title bar text
  TCHAR szWindowClass[MAX_LOADSTRING];                  // The window class name
  RECT rc;

  hInst = hInstance;            // Store instance handle in our global variable

  LoadString(hInstance, IDC_XCSOAR, szWindowClass, MAX_LOADSTRING);
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

  //If it is already running, then focus on the window
  if (main_window.find(szWindowClass, szTitle))
    return false;

  PaintWindow::register_class(hInst);
  main_window.register_class(hInst, szWindowClass);

  // other startup...

  InitSineTable();
  PreloadInitialisation(true);

  StartupStore(TEXT("Create main window\n"));

  RECT WindowSize = SystemWindowSize();
  main_window.set(szWindowClass, szTitle,
		  WindowSize.left, WindowSize.top,
		  WindowSize.right, WindowSize.bottom);

  if (!main_window.defined()) {
    return false;
  }
  main_window.install_timer();

  rc = main_window.get_client_rect();
#if (WINDOWSPC>0)
  rc.left = 0;
  rc.right = SCREENWIDTH;
  rc.top = 0;
  rc.bottom = SCREENHEIGHT;
#endif

  StartupStore(TEXT("InfoBox geometry\n"));

  InfoBoxLayout::ScreenGeometry(rc);
  // color/pattern chart (must have infobox geometry before this)
  MapGfx.Initialise(hInstance);

  StartupStore(TEXT("Create info boxes\n"));
  RECT rcsmall = InfoBoxManager::Create(rc);

  StartupStore(TEXT("Create button labels\n"));
  ButtonLabel::CreateButtonLabels(rc);
  ButtonLabel::SetLabelText(0,TEXT("MODE"));

  StartupStore(TEXT("Initialise fonts\n"));
  InitialiseFonts(main_window, rc);

  StartupStore(TEXT("Create FLARM gauge\n"));
  gauge_flarm = new GaugeFLARM(main_window);

  StartupStore(TEXT("Initialise message system\n"));
  Message::Initialize(rc); // creates window, sets fonts

  ///////////////////////////////////////////////////////
  //// create map window

  StartupStore(TEXT("Create map window\n"));

  map_window.SetMapRect(rcsmall);
  map_window.register_class(hInst, TEXT("MapWindowClass"));
  map_window.set(main_window, TEXT("MapWindowClass"),
                 0, 0, rc.right - rc.left, rc.bottom-rc.top);
  map_window.set_font(MapWindowFont);

  ///////////////////////////////////////////////////////
  // initial show

  main_window.show();
  main_window.update();

  map_window.show();
  map_window.update();

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

  ClearTask();
  InitCalculations(&GPS_INFO,&CALCULATED_INFO);
  LinkGRecordDLL(); // try to link DLL if it exists
  OpenGeoid();

  PreloadInitialisation(false);
  ////////////////////////////////////////////////////////

  GaugeCDI::Create();

  gauge_vario = new GaugeVario(main_window, map_window.GetMapRect());

  LoadWindFromRegistry();
  CalculateNewPolarCoef();
  StartupStore(TEXT("GlidePolar::UpdatePolar\n"));
  GlidePolar::UpdatePolar(false);

  StartupInfo();

  marks = new Marks();
  topology = new TopologyStore(marks->GetTopology());
  topology->Open();
  marks->Initialise();
  RasterTerrain::OpenTerrain();

  ReadWayPoints();
  InitWayPointCalc();

  ReadAirfieldFile();
  SetHome(false);

  RasterTerrain::ServiceFullReload(GPS_INFO.Latitude,
                                   GPS_INFO.Longitude);

  CreateProgressDialog(gettext(TEXT("Scanning weather forecast")));
  StartupStore(TEXT("RASP load\n"));
  RASP.Scan(GPS_INFO.Latitude, GPS_INFO.Longitude);

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
  GlidePolar::UpdatePolar(true);

  CreateProgressDialog(gettext(TEXT("Initialising display")));

  // just about done....

  DoSunEphemeris(GPS_INFO.Longitude, GPS_INFO.Latitude);

  // Finally ready to go
  StartupStore(TEXT("CreateDrawingThread\n"));
  map_window.CreateDrawingThread();
  StartupStore(TEXT("ShowInfoBoxes\n"));
  InfoBoxManager::Show();

  StartupStore(TEXT("CreateCalculationThread\n"));
  CreateCalculationThread();

  StartupStore(TEXT("AirspaceWarnListInit\n"));
  AirspaceWarnListInit();
  StartupStore(TEXT("dlgAirspaceWarningInit\n"));
  dlgAirspaceWarningInit();

  // find unique ID of this PDA
  ReadAssetNumber();

  // Da-da, start everything now
  StartupStore(TEXT("ProgramStarted\n"));

  // map gets initial focus
  map_window.set_focus();

  globalRunningEvent.trigger();

  return true;
}


void Shutdown(void) {
  int i;

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));
  StartHourglassCursor();

  StartupStore(TEXT("Entering shutdown...\n"));
  StartupLogFreeRamAndStorage();

  // turn off all displays
  globalRunningEvent.reset();

  StartupStore(TEXT("dlgAirspaceWarningDeInit\n"));
  dlgAirspaceWarningDeInit();
  StartupStore(TEXT("AirspaceWarnListDeInit\n"));
  AirspaceWarnListDeInit();

  CreateProgressDialog(gettext(TEXT("Shutdown, saving logs...")));
  // stop logger
  guiStopLogger(true);

  CreateProgressDialog(gettext(TEXT("Shutdown, saving profile...")));
  // Save settings
  StoreRegistry();

  // Stop sound

  StartupStore(TEXT("SaveSoundSettings\n"));
  SaveSoundSettings();

#ifndef DISABLEAUDIOVARIO
  //  VarioSound_EnableSound(false);
  //  VarioSound_Close();
#endif

  // Stop drawing
  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  // Stop calculating too (wake up)
  TriggerAll();

  StartupStore(TEXT("CloseDrawingThread\n"));
  map_window.CloseDrawingThread();

  // Clear data

  CreateProgressDialog(gettext(TEXT("Shutdown, saving task...")));
  StartupStore(TEXT("Save default task\n"));
  mutexTaskData.Lock();
  ResumeAbortTask(-1); // turn off abort if it was on.
  mutexTaskData.Unlock();
  SaveDefaultTask();

  StartupStore(TEXT("Clear task data\n"));

  ClearTask();
  CloseAirspace();
  CloseWayPoints();

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  StartupStore(TEXT("CloseTerrainTopology\n"));

  RASP.Close();
  RasterTerrain::CloseTerrain();
  topology->Close();
  marks->Close();
  CloseTerrainRenderer();

  delete topology;
  delete marks;

  devShutdown();

  SaveCalculationsPersist(&CALCULATED_INFO);
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

  StartupStore(TEXT("Close Gauges\n"));

  GaugeCDI::Destroy();
  delete gauge_vario;
  delete gauge_flarm;

  StartupStore(TEXT("Close Messages\n"));
  Message::Destroy();

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

  StartupStore(TEXT("Close Calculations\n"));
  CloseCalculations();

  CloseGeoid();

  StartupStore(TEXT("Close Windows - map\n"));
  map_window.reset();
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


