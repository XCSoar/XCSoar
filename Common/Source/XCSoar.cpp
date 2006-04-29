/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
#include "stdafx.h"
#include "compatibility.h"

#include "XCSoar.h"
#include "Mapwindow.h"
#include "Parser.h"
#include "Calculations.h"
#include "Task.h"
#include "Dialogs.h"
#include "Process.h"
#include "Utils.h"
#include "Port.h"
#include "Waypointparser.h"
#include "Airspace.h"
#include "Logger.h"
#include "McReady.h"
#include "AirfieldDetails.h"
#include "InfoBoxLayout.h"

#include <commctrl.h>
#include <aygshell.h>
#include <sipapi.h>

#include "Terrain.h"
#include "VarioSound.h"
#include "device.h"
#include "devCAI302.h"
#include "devEW.h"
#include "devAltairPro.h"
#include "devVega.h"
#include "Externs.h"
#include "units.h"
#include "InputEvents.h"
#include "Message.h"
#include "Atmosphere.h"

#if NEWINFOBOX > 0
#include "InfoBox.h"
#endif


#if NEWINFOBOX
#if !defined(MapScale2)
  #define MapScale2  apMs2Default
#endif

#if SAMGI
Appearance_t Appearance = {
  apMsAltA,
  apMs2None,
  true,
  206,
  {0,-13},
  apFlightModeIconAltA,
  //apFlightModeIconDefault,
  {10,3},
  apCompassAltA,
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  ctBestCruiseTrackAltA,
  afAircraftAltA,
  true,
  fgFinalGlideAltA,
  wpLandableAltA,
  true,
  true,
  true,
  smAlligneTopLeft,
  true,
  true,
  true,
  true,
  true,
  gvnsDefault, 
  false
};
#else

Appearance_t Appearance = {
  apMsAltA, // mapscale
  MapScale2, 
  false, // don't show logger indicator
  206,
  {0,-13},
  apFlightModeIconDefault,
  {0,0},
  apCompassAltA,
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  ctBestCruiseTrackAltA,
  afAircraftAltA,
  true, // don't show speed to fly
  fgFinalGlideDefault,
  wpLandableDefault,
  true,
  false,
  true,
  smAlligneCenter,
  false,
  false,
  false,
  false,
  false,
  gvnsLongNeedle,
  true
};

#endif

#else 

Appearance_t Appearance = {
  apMsDefault,
  apMs2Default,
  false,
  240,
  {0,0},
  apFlightModeIconDefault,
  {0,0},
  apCompassDefault,
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  ctBestCruiseTrackDefault,
  afAircraftDefault,
  false,
  fgFinalGlideDefault,
  wpLandableDefault,
  false,
  false,
  false,
  smAlligneCenter,
  false,
  false,
  false,
  false,
  false,
  gvnsLongNeedle,
  true
};
#endif



extern TCHAR XCSoar_Version[256] = TEXT("");

HWND hWnd1, hWnd2, hWnd3;

HINSTANCE                       hInst;                                  // The current instance
HWND                                    hWndCB;                                 // The command bar handle
HWND                                    hWndMainWindow; // Main Windows
HWND                                    hWndMapWindow;  // MapWindow
HWND          hWndMenuButton = NULL;


int numInfoWindows = 8;

#if NEWINFOBOX>0
InfoBox *InfoBoxes[MAXINFOWINDOWS];
#else
HWND                                    hWndInfoWindow[MAXINFOWINDOWS];
HWND                                    hWndTitleWindow[MAXINFOWINDOWS];
#endif

int                                     InfoType[MAXINFOWINDOWS] = 
#ifdef GNAV
  {
    873336334,
    856820491,
    822280982,
    2829105,
    103166000,
    421601569,
    657002759,
    621743887,
    439168301
  };
#else
  {921102,
   725525,
   262144,
   74518,
   657930,
   2236963,
   394758,
   1644825};
#endif

bool                                    DisplayLocked = true;
bool                                    InfoWindowActive = true;
bool                                    EnableAuxiliaryInfo = false;
int                                     InfoBoxFocusTimeOut = 0;
int                                     MenuTimeOut = 0;
int                                     DisplayTimeOut = 0;
int                                     MenuTimeoutMax = MENUTIMEOUTMAX;


HBRUSH hBrushSelected;
HBRUSH hBrushUnselected;
HBRUSH hBrushButton;
COLORREF ColorSelected = RGB(0xC0,0xC0,0xC0);
COLORREF ColorUnselected = RGB(0xFF,0xFF,0xFF);
COLORREF ColorWarning = RGB(0xFF,0x00,0x00);
COLORREF ColorOK = RGB(0x00,0x00,0xFF);
COLORREF ColorButton = RGB(0xA0,0xE0,0xA0);

// Serial Port Globals

HANDLE                          hPort1 = INVALID_HANDLE_VALUE;    // Handle to the serial port
HANDLE                          hPort2 = INVALID_HANDLE_VALUE;    // Handle to the serial port
BOOL                                    Port1Available = FALSE;
BOOL                                    Port2Available = FALSE;

// Display Gobals
HFONT                                   InfoWindowFont;
HFONT                                   TitleWindowFont;
HFONT                                   MapWindowFont;

HFONT                                   MapWindowBoldFont;

HFONT                                   CDIWindowFont; // New
HFONT                                   MapLabelFont;
HFONT                                   StatisticsFont;

int                                             CurrentInfoType;
int                                             InfoFocus = 0;
int                                             DisplayOrientation = TRACKUP;
int                                             DisplayTextType = DISPLAYNONE;

int                                             AltitudeMode = ALLON;
int                                             ClipAltitude = 1000;
int                                             AltWarningMargin = 100;
int                                             AutoAdvance = 1;
bool                                            AdvanceArmed = false;

double                          QNH = (double)1013.2;
bool EnableBlockSTF = false;

bool GlobalRunning = false; 
// this controls all displays, to make sure everything is
// properly initialised.


//SI to Local Units
double        SPEEDMODIFY = TOKNOTS;
double                          LIFTMODIFY  = TOKNOTS;
double                          DISTANCEMODIFY = TONAUTICALMILES;
double        ALTITUDEMODIFY = TOFEET;

//Flight Data Globals
double        MACCREADY = 0; // JMW now in SI units (m/s) for consistency
bool          AutoMacCready = false;

int          NettoSpeed = 1000;

NMEA_INFO     GPS_INFO;
DERIVED_INFO  CALCULATED_INFO;
BOOL GPSCONNECT = FALSE;
BOOL extGPSCONNECT = FALSE; // this one used by external functions

bool          TaskAborted = false;

bool InfoBoxesDirty= false;
bool DialogActive = false;

//Local Static data
static int iTimerID;

// Final Glide Data
double SAFETYALTITUDEARRIVAL = 500;
double SAFETYALTITUDEBREAKOFF = 700;
double SAFETYALTITUDETERRAIN = 200;
double SAFTEYSPEED = 50.0;

// polar info
double BUGS = 1;
double BALLAST = 0;
int              POLARID = 0;
double POLAR[POLARSIZE] = {0,0,0};
double POLARV[POLARSIZE] = {21,27,40};
double POLARLD[POLARSIZE] = {33,30,20};
double WEIGHTS[POLARSIZE] = {250,70,100};

// Waypoint Database
WAYPOINT *WayPointList = NULL;
unsigned int NumberOfWayPoints = 0;
int SectorType = 1; // FAI sector
DWORD SectorRadius = 500;
int StartLine = TRUE;
DWORD StartRadius = 3000;

int HomeWaypoint = -1;

// Airspace Database
AIRSPACE_AREA *AirspaceArea = NULL;
AIRSPACE_POINT *AirspacePoint = NULL;
POINT *AirspaceScreenPoint = NULL;
AIRSPACE_CIRCLE *AirspaceCircle = NULL;
unsigned int NumberOfAirspacePoints = 0;
unsigned int NumberOfAirspaceAreas = 0;
unsigned int NumberOfAirspaceCircles = 0;

//Airspace Warnings
int AIRSPACEWARNINGS = TRUE;
int WarningTime = 30;
int AcknowledgementTime = 30;

// Registration Data
TCHAR strAssetNumber[MAX_LOADSTRING] = TEXT(""); //4G17DW31L0HY");
TCHAR strRegKey[MAX_LOADSTRING] = TEXT("");

// Interface Files
GetTextSTRUCT GetTextData[MAXSTATUSMESSAGECACHE];
int GetTextData_Size = 0;
StatusMessageSTRUCT StatusMessageData[MAXSTATUSMESSAGECACHE];
int StatusMessageData_Size = 0;

//Snail Trial
SNAIL_POINT SnailTrail[TRAILSIZE];
int SnailNext = 0;

// user interface settings
bool CircleZoom = false;
int WindUpdateMode = 0;
bool EnableTopology = false;
bool EnableTerrain = false;
bool FinalGlideTerrain = false;
bool EnableSoundVario = true;
bool EnableSoundModes = true;
bool EnableSoundTask = true;
int SoundVolume = 80;
int SoundDeadband = 5;
bool EnableVarioGauge = false;
bool EnableAutoBlank = false;
bool ScreenBlanked = false;



//IGC Logger
bool LoggerActive = false;

// Others
BOOL TopWindow = TRUE;

BOOL COMPORTCHANGED = FALSE;
BOOL AIRSPACEFILECHANGED = FALSE;
BOOL AIRFIELDFILECHANGED = FALSE;
BOOL WAYPOINTFILECHANGED = FALSE;
BOOL TERRAINFILECHANGED = FALSE;
BOOL TOPOLOGYFILECHANGED = FALSE;
BOOL POLARFILECHANGED = FALSE;
BOOL LANGUAGEFILECHANGED = FALSE;
BOOL STATUSFILECHANGED = FALSE;
BOOL INPUTFILECHANGED = FALSE;
bool MenuActive = false;

//Task Information
Task_t Task = {{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0}};
TaskStats_t TaskStats;
int ActiveWayPoint = -1;

// Assigned Area Task
double AATTaskLength = 120;
BOOL AATEnabled = FALSE;
DWORD FinishMinHeight = 0;
DWORD StartMaxHeight = 0;
DWORD StartMaxSpeed = 0;


// Statistics
Statistics flightstats;

#if UNDER_CE >= 300
static SHACTIVATEINFO s_sai;
#endif

static  TCHAR *COMMPort[] = {TEXT("COM1:"),TEXT("COM2:"),TEXT("COM3:"),TEXT("COM4:"),TEXT("COM5:"),TEXT("COM6:"),TEXT("COM7:"),TEXT("COM8:"),TEXT("COM9:"),TEXT("COM10:")};
static  DWORD   dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};
static  DWORD PortIndex1 = 0;
static  DWORD SpeedIndex1 = 2;
static  DWORD PortIndex2 = 0;
static  DWORD SpeedIndex2 = 2;

BOOL InfoBoxesHidden = false;

void PopupBugsBallast(int updown);

#include "GaugeCDI.h"
#include "GaugeFLARM.h"
#include "GaugeVario.h"

// Battery status for SIMULATOR mode
//	30% reminder, 20% exit, 30 second reminders on warnings
#ifdef _SIM_
#define BATTERY_WARNING 30
#define BATTERY_EXIT 20
#define BATTERY_REMINDER 30000
DWORD BatteryWarningTime = 0;
#endif
// Groups:
//   Altitude 0,1,20,33
//   Aircraft info 3,6,23,32,37,47
//   LD 4,5,19,38
//   Vario 2,7,8,9,21,22,24,44
//   Wind 25,26,48,49,50
//   Mcready 10,34,35,43
//   Nav 11,12,13,15,16,17,18,27,28,29,30,31
//   Waypoint 14,36,39,40,41,42,45,46
SCREEN_INFO Data_Options[] = {
          // 0
	  {ugAltitude,        TEXT("Height GPS"), TEXT("H GPS"), new InfoBoxFormatter(TEXT("%2.0f")), AltitudeProcessing, 1, 33},
	  // 1
	  {ugAltitude,        TEXT("Height AGL"), TEXT("H AGL"), new FormatterLowWarning(TEXT("%2.0f"),0.0), NoProcessing, 20, 0},
	  // 2
	  {ugVerticalSpeed,   TEXT("Thermal last 30 sec"), TEXT("TC 30s"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 7, 44},
	  // 3
	  {ugNone,            TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f°T")), NoProcessing, 6, 37},
	  // 4
	  {ugNone,            TEXT("L/D instantaneous"), TEXT("L/D Inst"), new InfoBoxFormatter(TEXT("%2.0f")), PopupBugsBallast, 5, 38},
	  // 5
	  {ugNone,            TEXT("L/D cruise"), TEXT("L/D Cru"), new InfoBoxFormatter(TEXT("%2.0f")), PopupBugsBallast, 19, 4},
	  // 6
	  {ugHorizontalSpeed, TEXT("Speed ground"), TEXT("V Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), SpeedProcessing, 23, 3},
	  // 7
	  {ugVerticalSpeed,   TEXT("Last Thermal Average"), TEXT("TL Avg"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
	  // 8
	  {ugAltitude,        TEXT("Last Thermal Gain"), TEXT("TL Gain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 9, 7},
	  // 9
	  {ugNone,            TEXT("Last Thermal Time"), TEXT("TL Time"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 21, 8},
	  // 10
	  {ugVerticalSpeed,   TEXT("MacCready Setting"), TEXT("MacCready"), new InfoBoxFormatter(TEXT("%2.1f")), MacCreadyProcessing, 34, 43},
	  // 11
	  {ugDistance,        TEXT("Next Distance"), TEXT("WP Dist"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 12, 31},
	  // 12
	  {ugAltitude,        TEXT("Next Altitude Difference"), TEXT("WP AltD"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 13, 11},
	  // 13
	  {ugAltitude,        TEXT("Next Altitude Required"), TEXT("WP AltR"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 15, 12},
	  // 14
	  {ugNone,            TEXT("Next Waypoint"), TEXT("Next"), new FormatterWaypoint(TEXT("\0")), NextUpDown, 36, 46},
	  // 15
	  {ugAltitude,        TEXT("Final Altitude Difference"), TEXT("Fin AltD"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 16, 13},
	  // 16
	  {ugAltitude,        TEXT("Final Altitude Required"), TEXT("Fin AltR"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 17, 15},
	  // 17
	  {ugHorizontalSpeed, TEXT("Speed Task Average"), TEXT("V Task"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
	  // 18
	  {ugDistance,        TEXT("Final Distance"), TEXT("Fin Dis"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 27, 17},
	  // 19
	  {ugNone,            TEXT("Final L/D"), TEXT("Fin L/D"), new InfoBoxFormatter(TEXT("%1.0f")), NoProcessing, 38, 5},
	  // 20
	  {ugAltitude,        TEXT("Terrain Elevation"), TEXT("H Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 33, 1},
	  // 21
	  {ugVerticalSpeed,   TEXT("Thermal Average"), TEXT("TC Avg"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 22, 9},
	  // 22
	  {ugAltitude,        TEXT("Thermal Gain"), TEXT("TC Gain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 24, 21},
	  // 23
	  {ugNone,            TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f°T")), DirectionProcessing, 32, 6},
	  // 24
	  {ugVerticalSpeed,   TEXT("Vario"), TEXT("Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 44, 22},
	  // 25
	  {ugWindSpeed,       TEXT("Wind Speed"), TEXT("Wind V"), new InfoBoxFormatter(TEXT("%2.0f")), WindSpeedProcessing, 26, 50},
	  // 26
	  {ugNone,            TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f°T")), WindDirectionProcessing, 48, 25},
	  // 27
	  {ugNone,            TEXT("AA Time"), TEXT("AA Time"), new FormatterTime(TEXT("%2.0f")), NoProcessing, 28, 18},
	  // 28
	  {ugDistance,        TEXT("AA Distance Max"), TEXT("AA Dmax"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 29, 27},
	  // 29
	  {ugDistance,        TEXT("AA Distance Min"), TEXT("AA Dmin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 30, 28},
	  // 30
	  {ugHorizontalSpeed, TEXT("AA Speed Max"), TEXT("AA Vmax"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 31, 29},
	  // 31
	  {ugHorizontalSpeed, TEXT("AA Speed Min"), TEXT("AA Vmin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 51, 30},
	  // 32
	  {ugHorizontalSpeed, TEXT("Airspeed IAS"), TEXT("V IAS"), new InfoBoxFormatter(TEXT("%2.0f")), AirspeedProcessing, 37, 23},
	  // 33
	  {ugAltitude,        TEXT("Pressure Altitude"), TEXT("H Baro"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 0, 20},
	  // 34
	  {ugHorizontalSpeed, TEXT("Speed MacReady"), TEXT("V Mc"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 35, 10},
	  // 35
#if (NEWINFOBOX>0)
	  {ugNone,            TEXT("Percentage climb"), TEXT("% Climb"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 43, 34},
#else
	  {ugNone,            TEXT("Percentage climb"), TEXT("%% Climb"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 43, 34},
#endif
	  // 36
	  {ugNone,            TEXT("Time of flight"), TEXT("Time flt"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 39, 14},
	  // 37
	  {ugNone,            TEXT("G load"), TEXT("G"), new InfoBoxFormatter(TEXT("%2.2f")), AccelerometerProcessing, 47, 32},
	  // 38
	  {ugNone,            TEXT("Next L/D"), TEXT("WP L/D"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 4, 19},
	  // 39
	  {ugNone,            TEXT("Time local"), TEXT("Time loc"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 40, 36},
	  // 40
	  {ugNone,            TEXT("Time UTC"), TEXT("Time UTC"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 41, 39},
	  // 41
	  {ugNone,            TEXT("Task Time To Go"), TEXT("Fin ETE"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 42, 40},
	  // 42
	  {ugNone,            TEXT("Next Time To Go"), TEXT("WP ETE"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 45, 41},
	  // 43
	  {ugHorizontalSpeed, TEXT("Speed Dolphin"), TEXT("V Opt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 10, 35},
	  // 44
	  {ugVerticalSpeed,   TEXT("Netto Vario"), TEXT("Netto"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 2, 24},
	  // 45
	  {ugNone,            TEXT("Task Arrival Time"), TEXT("Fin ETA"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 46, 42},
	  // 46
	  {ugNone,            TEXT("Next Arrival Time"), TEXT("WP ETA"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 14, 45},
	  // 47
	  {ugNone,            TEXT("Bearing Difference"), TEXT("Brng D"), new FormatterDiffBearing(TEXT("")), NoProcessing, 3, 37},
	  // 48
	  {ugNone,            TEXT("Outside Air Temperature"), TEXT("OAT"), new InfoBoxFormatter(TEXT("%2.1f°")), NoProcessing, 49, 26},
	  // 49
	  {ugNone,            TEXT("Relative Humidity"), TEXT("RelHum"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 50, 48},
	  // 50
	  {ugNone,            TEXT("Forecast Temperature"), TEXT("MaxTemp"), new InfoBoxFormatter(TEXT("%2.1f°")), ForecastTemperatureProcessing, 49, 25},
	  // 51
	  {ugDistance,        TEXT("AA Distance Tgt"), TEXT("AA Dtgt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 52, 31},
	  // 52
	  {ugHorizontalSpeed, TEXT("AA Speed Tgt"), TEXT("AA Vtgt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 11, 51},

	};
int NUMSELECTSTRINGS = 53;


CRITICAL_SECTION  CritSec_FlightData;
CRITICAL_SECTION  CritSec_TerrainDataGraphics;
CRITICAL_SECTION  CritSec_TerrainDataCalculations;
CRITICAL_SECTION  CritSec_NavBox;
CRITICAL_SECTION  CritSec_Comm;


// Forward declarations of functions included in this code module:
ATOM                                                    MyRegisterClass (HINSTANCE, LPTSTR);
BOOL                                                    InitInstance    (HINSTANCE, int);
LRESULT CALLBACK        WndProc                 (HWND, UINT, WPARAM, LPARAM);
LRESULT                                         MainMenu(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void                                                    AssignValues(void);
void                                                    DisplayText(void);

void CommonProcessTimer    (void);
void ProcessTimer    (void);
void SIMProcessTimer(void);

void                                                    PopUpSelect(int i);
HWND                                                    CreateRpCommandBar(HWND hwnd);

#ifdef DEBUG
void                                            DebugStore(char *Str);
#endif


void HideMenu() {
  // ignore this if the display isn't locked -- must keep menu visible
  if (DisplayLocked) {
    ShowWindow(hWndMenuButton, SW_HIDE);
    MenuTimeOut = MenuTimeoutMax;
    DisplayTimeOut = 0;
  }
}

void ShowMenu() {
  MenuTimeOut = 0;
  ShowWindow(hWndMenuButton, SW_SHOW);
  DisplayTimeOut = 0;
}


#if (EXPERIMENTAL > 0)
#include "BlueSMS.h"
BlueDialupSMS bsms;
#endif

void SettingsEnter() {
  MenuActive = true;
  MapWindow::SuspendDrawingThread();

  AIRSPACEFILECHANGED = FALSE;
  AIRFIELDFILECHANGED = FALSE;
  WAYPOINTFILECHANGED = FALSE;
  TERRAINFILECHANGED = FALSE;
  TOPOLOGYFILECHANGED = FALSE;
  POLARFILECHANGED = FALSE;
  LANGUAGEFILECHANGED = FALSE;
  STATUSFILECHANGED = FALSE;
  INPUTFILECHANGED = FALSE;
  COMPORTCHANGED = FALSE;
}



void SettingsLeave() {
  SwitchToMapWindow();
  LockFlightData();
  LockNavBox();
  MenuActive = false;
    
  if((WAYPOINTFILECHANGED) || (TERRAINFILECHANGED) || (AIRFIELDFILECHANGED))
    {
      Task[0].Index = -1;  ActiveWayPoint = -1;

      // re-load terrain
      CloseTerrain();
      OpenTerrain();

      // re-load waypoints
      ReadWayPoints();
      ReadAirfieldFile();
      
      // re-set home
      if (WAYPOINTFILECHANGED) {
	HomeWaypoint = -1;
	if(NumberOfWayPoints) SetHome();
      }
    }
  
  if (TOPOLOGYFILECHANGED)
    {
      CloseTopology();
      OpenTopology();
      ReadTopology();
    }
  
  if(AIRSPACEFILECHANGED)
    {
      CloseAirspace();
      ReadAirspace();
      SortAirspace();
    }  
  
  if (POLARFILECHANGED) {
    CalculateNewPolarCoef();
    GlidePolar::SetBallast();
  }
  
  if (AIRFIELDFILECHANGED
      || AIRSPACEFILECHANGED
      || WAYPOINTFILECHANGED
      || TERRAINFILECHANGED
      ) {
    CloseProgressDialog();
    SetFocus(hWndMapWindow);
  }
  
  UnlockFlightData();
  UnlockNavBox();

#ifndef _SIM_
  if(COMPORTCHANGED)
    {
      // JMW disabled com opening in sim mode
      RestartCommPorts();
    }

#endif

  MapWindow::ResumeDrawingThread();

}


#if NEWINFOBOX > 0

void dlgStatusShowModal(void);
void dlgStatusTaskShowModal(void);
void dlgStatusSystemShowModal(void);
void dlgConfigurationShowModal(void);

void SystemConfiguration(void) {
#ifndef _SIM_
  if (LockSettingsInFlight && CALCULATED_INFO.Flying) {
    DoStatusMessage(TEXT("Settings locked in flight"));
    return;
  }
#endif

  SettingsEnter();
  dlgConfigurationShowModal(); 
  SettingsLeave();
}

void ShowStatus(void){
  dlgStatusShowModal();
}

void ShowStatusSystem(void){
  dlgStatusSystemShowModal();
}

void ShowStatusTask(void){
  dlgStatusTaskShowModal();
}

#else

void SystemConfiguration(void) {
}

void ShowStatusSystem(void){
}

void ShowStatusTask(void){
}

void ShowStatus() {
  TCHAR statusmessage[2000];
  TCHAR Temp[1000];
  int iwaypoint= -1;
  double sunsettime;
  int sunsethours;
  int sunsetmins;
  double bearing;
  double distance;
  TCHAR sLongitude[16];
  TCHAR sLatitude[16];
  int   TabStops[] = {60,80,0};

  statusmessage[0]=0;
  _tcscat(statusmessage, TEXT("\r\n"));

  Units::LongitudeToString(GPS_INFO.Longitude, 
			   sLongitude, sizeof(sLongitude)-1);
  Units::LatitudeToString(GPS_INFO.Latitude, 
			  sLatitude, sizeof(sLatitude)-1);

  sunsettime = DoSunEphemeris(GPS_INFO.Longitude,
                              GPS_INFO.Latitude);
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime-sunsethours)*60);

  _stprintf(Temp,TEXT("%s\t%s\r\n%s\t%s\r\n%s\t%.0f %s\r\n%s\t%02d:%02d\r\n\r\n"),
		   gettext(TEXT("Longitude")),
           sLongitude,
		   gettext(TEXT("Latitude")),
           sLatitude,
		   gettext(TEXT("Altitude")),
           GPS_INFO.Altitude*ALTITUDEMODIFY,
            Units::GetAltitudeName(),
		   gettext(TEXT("Sunset")),
           sunsethours,
           sunsetmins
           );
  _tcscat(statusmessage, Temp);

  iwaypoint = FindNearestWayPoint(GPS_INFO.Longitude,
                                  GPS_INFO.Latitude,
                                  100000.0); // big range limit
  if (iwaypoint>=0) {

    bearing = Bearing(GPS_INFO.Latitude,
                      GPS_INFO.Longitude,
                      WayPointList[iwaypoint].Latitude,
                      WayPointList[iwaypoint].Longitude);

    distance = Distance(GPS_INFO.Latitude,
                        GPS_INFO.Longitude,
                        WayPointList[iwaypoint].Latitude,
                        WayPointList[iwaypoint].Longitude)*DISTANCEMODIFY;

    _stprintf(Temp,TEXT("%s\t%s\r\n%s\t%d\r\n%s\t%.1f %s\r\n\r\n"),
		     gettext(TEXT("Near")),
             WayPointList[iwaypoint].Name,
			 gettext(TEXT("Bearing")),
             (int)bearing,
			 gettext(TEXT("Distance")),
              distance,
              Units::GetDistanceName());
    _tcscat(statusmessage, Temp);

  }

  if (extGPSCONNECT) {
    if (GPS_INFO.NAVWarning) {
      wcscat(statusmessage, gettext(TEXT("GPS fix invalid")));       // JMW fixed message
    } else {
      if (GPS_INFO.SatellitesUsed==0) {
	wcscat(statusmessage, gettext(TEXT("No fix")));
      } else {
	wcscat(statusmessage, gettext(TEXT("GPS 3D fix")));
      }
    }
    wcscat(statusmessage, TEXT("\r\n"));

    _stprintf(Temp,TEXT("%s\t%d\r\n"),
			 gettext(TEXT("Satellites in view")),
             GPS_INFO.SatellitesUsed
             );
    wcscat(statusmessage, Temp);
  } else {
    wcscat(statusmessage, gettext(TEXT("GPS disconnected")));
	wcscat(statusmessage, TEXT("\r\n"));
  }
  if (GPS_INFO.VarioAvailable) {
    wcscat(statusmessage, gettext(TEXT("Vario connected")));
  } else {
    wcscat(statusmessage, gettext(TEXT("Vario disconnected")));
  }
  wcscat(statusmessage, TEXT("\r\n"));
  if (LoggerActive) {
    wcscat(statusmessage, gettext(TEXT("Logger ON")));
  } else {
    wcscat(statusmessage, gettext(TEXT("Logger OFF")));
  }
  wcscat(statusmessage, TEXT("\r\n"));

  if (GPS_INFO.FLARM_Available) {
    if (GPS_INFO.FLARM_TX && GPS_INFO.FLARM_GPS) {
      _stprintf(Temp,TEXT("FLARM: OK, level\t%d"),
		GPS_INFO.FLARM_AlarmLevel
		);
      wcscat(statusmessage, Temp);
    } else if (GPS_INFO.FLARM_TX) {
      wcscat(statusmessage, (TEXT("FLARM: No GPS fix")));      
    } else {
      wcscat(statusmessage, (TEXT("FLARM: TX error")));      
    }
  }
  wcscat(statusmessage, TEXT("\r\n"));

  DoStatusMessage(TEXT("Status"), statusmessage);

}
#endif




void FullScreen() {

  if (!MenuActive) {
    SetForegroundWindow(hWndMainWindow);
    SHFullScreen(hWndMainWindow,
                 SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#if (WINDOWSPC>0)
    SetWindowPos(hWndMainWindow,HWND_TOP,
                 0, 0, 0, 0,
                 SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
#else
    SetWindowPos(hWndMainWindow,HWND_TOP,
                 0,0,
                 GetSystemMetrics(SM_CXSCREEN),
                 GetSystemMetrics(SM_CYSCREEN),
                 SWP_SHOWWINDOW);
#endif
  }
  MapWindow::RequestFastRefresh();
  InfoBoxesDirty = true;
}


void LockComm() {
  EnterCriticalSection(&CritSec_Comm);
}

void UnlockComm() {
  LeaveCriticalSection(&CritSec_Comm);
}


void RestartCommPorts() {
  static bool first = true;

  LockComm();
#if (WINDOWSPC>0)
#else
  devClose(devA());
  devClose(devB());
#endif

  // Close both first!
  if(Port1Available)
    {
#if (WINDOWSPC>0)
#else
      Port1Available = FALSE; 
      Port1Close ();
#endif
    }
  if(Port2Available)
    {
#if (WINDOWSPC>0)
#else
      Port2Available = FALSE; 
      Port2Close ();
#endif
    }

#if (WINDOWSPC<1)
  NMEAParser::Reset();
#endif

#if (WINDOWSPC>0)
#else
  first = true;
#endif
  if (first) {
    if (!Port1Available) {
#ifdef GNAV
      PortIndex1 = 2; SpeedIndex1 = 5; 
#else
      PortIndex1 = 0; SpeedIndex1 = 2; 
#endif
      ReadPort1Settings(&PortIndex1,&SpeedIndex1);
      Port1Available = 
	Port1Initialize (COMMPort[PortIndex1],dwSpeed[SpeedIndex1]);
    }
    
    if (!Port2Available) {
#ifdef GNAV
      PortIndex2 = 0; SpeedIndex2 = 5; 
#else
      PortIndex2 = 0; SpeedIndex2 = 2; 
#endif
      ReadPort2Settings(&PortIndex2,&SpeedIndex2);
      if (PortIndex1 != PortIndex2) {
	Port2Available = 
	  Port2Initialize (COMMPort[PortIndex2],dwSpeed[SpeedIndex2]);
      } else {
	Port2Available = FALSE;
      }
    }
    first = false;
  }

#if (WINDOWSPC<1)
  devInit(TEXT(""));      
#endif

  UnlockComm();

}



void DefocusInfoBox() {
  FocusOnWindow(InfoFocus,false);
  InfoFocus = -1;
  if (MapWindow::isPan()) {
    InputEvents::setMode(TEXT("pan"));
  } else {
    InputEvents::setMode(TEXT("default"));
  }
  InfoWindowActive = FALSE;
}


void FocusOnWindow(int i, bool selected) {
    //hWndTitleWindow

  if (i<0) return; // error

  #if NEWINFOBOX>0
    InfoBoxes[i]->SetFocus(selected);
    // todo defocus all other?
  #else

  HWND wind = hWndInfoWindow[i];

  if (selected) {
    SetWindowLong(wind,GWL_STYLE,WS_VISIBLE|WS_CHILD
		  |WS_TABSTOP|SS_CENTER|SS_NOTIFY|WS_BORDER);
  } else {
    SetWindowLong(wind,GWL_STYLE,WS_VISIBLE|WS_CHILD
		  |WS_TABSTOP|SS_CENTER|SS_NOTIFY);
  }

  wind = hWndTitleWindow[i];
  if (selected) {
    SetWindowLong(wind, GWL_USERDATA, 1);
  } else {
    SetWindowLong(wind, GWL_USERDATA, 0);
  }
  #endif

}


void TriggerRedraws(NMEA_INFO *nmea_info,
		    DERIVED_INFO *derived_info) {
  if (MapWindow::IsDisplayRunning()) {
    if (NMEAParser::GpsUpdated) {
      MapWindow::MapDirty = true;
      PulseEvent(drawTriggerEvent); 
      // only ask for redraw if the thread was waiting,
      // this causes the map thread to try to synchronise
      // with the calculation thread, which is desirable
      // to reduce latency
      // it also ensures that if the display is lagging,
      // it will have a chance to catch up.
    }
  }
}


DWORD InstrumentThread (LPVOID lpvoid) {
  // wait for proper startup signal
  while (!MapWindow::IsDisplayRunning()) {
    Sleep(100);
  }


  while (!MapWindow::CLOSETHREAD) {

    WaitForSingleObject(varioTriggerEvent, 5000);
    ResetEvent(varioTriggerEvent);
    if (MapWindow::CLOSETHREAD) break; // drop out on exit

    if (NMEAParser::VarioUpdated) {
      NMEAParser::VarioUpdated = false;
      if (MapWindow::IsDisplayRunning()) {
	if (EnableVarioGauge) {
	  GaugeVario::Render();
	}
      }
    }
  }
  return 0;
}


DWORD CalculationThread (LPVOID lpvoid) {
  bool needcalculationsslow;

  NMEA_INFO     tmp_GPS_INFO;
  DERIVED_INFO  tmp_CALCULATED_INFO;

  needcalculationsslow = false;

  // wait for proper startup signal
  while (!MapWindow::IsDisplayRunning()) {
    Sleep(100);
  }

  while (!MapWindow::CLOSETHREAD) {

    WaitForSingleObject(dataTriggerEvent, 5000);
    if (MapWindow::CLOSETHREAD) break; // drop out on exit

    // set timer to determine latency (including calculations)
    if (NMEAParser::GpsUpdated) { 
      MapWindow::UpdateTimeStats(true);
    }

    // make local copy before editing...
    LockFlightData();
    if (NMEAParser::GpsUpdated) { // timeout on FLARM objects
      FLARM_RefreshSlots(&GPS_INFO);
    }
    memcpy(&tmp_GPS_INFO,&GPS_INFO,sizeof(NMEA_INFO));
    memcpy(&tmp_CALCULATED_INFO,&CALCULATED_INFO,sizeof(DERIVED_INFO));
    UnlockFlightData();

    // Do vario first to reduce audio latency
    if (GPS_INFO.VarioAvailable) {
      if (NMEAParser::VarioUpdated) {
	if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {
	  
	}
	// assume new vario data has arrived, so infoboxes
	// need to be redrawn
      }
    } else {
      // run the function anyway, because this gives audio functions
      // if no vario connected
      if (NMEAParser::GpsUpdated) {
	if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {
	}
	NMEAParser::VarioUpdated = true; // emulate vario update
	PulseEvent(varioTriggerEvent);
      }
    }
    
    if (NMEAParser::GpsUpdated) {
      if(DoCalculations(&tmp_GPS_INFO,&tmp_CALCULATED_INFO))
	{
	  MapWindow::MapDirty = true;
	}
      needcalculationsslow = true;
      InfoBoxesDirty = true;
    }

    ResetEvent(dataTriggerEvent);
        
    TriggerRedraws(&tmp_GPS_INFO, &tmp_CALCULATED_INFO);

    if (needcalculationsslow) {
      DoCalculationsSlow(&tmp_GPS_INFO,&tmp_CALCULATED_INFO);
      needcalculationsslow = false;
    }

    // values changed, so copy them back now: ONLY CALCULATED INFO
    // should be changed in DoCalculations, so we only need to write
    // that one back (otherwise we may write over new data)
    LockFlightData();
    memcpy(&CALCULATED_INFO,&tmp_CALCULATED_INFO,sizeof(DERIVED_INFO));
    UnlockFlightData();

    NMEAParser::GpsUpdated = false;

  }
  return 0;
}


void CreateCalculationThread() {
  HANDLE hCalculationThread;
  DWORD dwCalcThreadID;

  // Create a read thread for performing calculations
  if (hCalculationThread =
      CreateThread (NULL, 0, 
		    (LPTHREAD_START_ROUTINE )CalculationThread, 
		    0, 0, &dwCalcThreadID))
  {
    SetThreadPriority(hCalculationThread, THREAD_PRIORITY_NORMAL); 
    CloseHandle (hCalculationThread);
  } else {
    ASSERT(1);
  }

  HANDLE hInstrumentThread;
  DWORD dwInstThreadID;

  if (hInstrumentThread =
      CreateThread (NULL, 0, 
		    (LPTHREAD_START_ROUTINE )InstrumentThread, 
		    0, 0, &dwInstThreadID))
  {
    SetThreadPriority(hInstrumentThread, THREAD_PRIORITY_NORMAL); 
    CloseHandle (hInstrumentThread);
  } else {
    ASSERT(1);
  }

}

void dlgStartupShowModal(void);


void PreloadInitialisation(bool ask) {
  SetToRegistry(TEXT("XCV"), 1);

  // Registery (early)

  if (ask) {

    RestoreRegistry();
    ReadRegistrySettings();
    StatusFileInit();
  } else {
    //#if (WINDOWSPC<1) 
#if (NEWINFOBOX>0)
    dlgStartupShowModal();
    RestoreRegistry();
    ReadRegistrySettings();
#endif
    //#endif
    CreateProgressDialog(gettext(TEXT("Initialising")));
  }

  // Interface (before interface)
  if (!ask) {
    ReadLanguageFile();
    ReadStatusFile();
    InputEvents::readFile();
  }

}

HANDLE drawTriggerEvent;
HANDLE dataTriggerEvent;
HANDLE varioTriggerEvent;

bool ProgramStarted = false;

int WINAPI WinMain(     HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPTSTR    lpCmdLine,
                        int       nCmdShow)
{
  MSG msg;
  HACCEL hAccelTable;
  INITCOMMONCONTROLSEX icc;

  // JMW we need a global version string!

  // Version String
#ifdef GNAV
  wcscat(XCSoar_Version, TEXT("Altair "));
#endif
#if (NEWINFOBOX>0) 
  wcscat(XCSoar_Version, TEXT("4.7.3 "));
#else
  // wcscat(XCSoar_Version, TEXT("Alpha "));
  // wcscat(XCSoar_Version, TEXT(__DATE__));
  wcscat(XCSoar_Version, TEXT("BETA 4.5.5")); // Yet to be released
#endif

  XCSoarGetOpts(lpCmdLine);

  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = ICC_UPDOWN_CLASS;
  InitCommonControls();
  InitSineTable();

  // Perform application initialization:
  if (!InitInstance (hInstance, nCmdShow))
    {
      return FALSE;
    }

  // find unique ID of this PDA
  ReadAssetNumber();

  hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_XCSOAR);

  SHSetAppKeyWndAssoc(VK_APP1, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP2, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP3, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP4, hWndMainWindow);
  // Typical Record Button
  //	Why you can't always get this to work
  //	http://forums.devbuzz.com/m_1185/mpage_1/key_/tm.htm
  //	To do with the fact it is a global hotkey, but you can with code above
  //	Also APPA is record key on some systems
  SHSetAppKeyWndAssoc(VK_APP5, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP6, hWndMainWindow);

  InitializeCriticalSection(&CritSec_FlightData);
  InitializeCriticalSection(&CritSec_NavBox);
  InitializeCriticalSection(&CritSec_Comm);
  InitializeCriticalSection(&CritSec_TerrainDataGraphics);
  InitializeCriticalSection(&CritSec_TerrainDataCalculations);
  drawTriggerEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("drawTriggerEvent"));
  dataTriggerEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("dataTriggerEvent"));
  varioTriggerEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("varioTriggerEvent"));

  memset( &(GPS_INFO), 0, sizeof(GPS_INFO));
  memset( &(CALCULATED_INFO), 0,sizeof(CALCULATED_INFO));
  memset( &SnailTrail[0],0,TRAILSIZE*sizeof(SNAIL_POINT));

  // display start up screen
  //  StartupScreen();
  // not working very well at all

  PreloadInitialisation(false);

  GPS_INFO.SwitchState.AirbrakeLocked = false;
  GPS_INFO.SwitchState.FlapPositive = false;
  GPS_INFO.SwitchState.FlapNeutral = false;
  GPS_INFO.SwitchState.FlapNegative = false;
  GPS_INFO.SwitchState.GearExtended = false;
  GPS_INFO.SwitchState.Acknowledge = false;
  GPS_INFO.SwitchState.Repeat = false;
  GPS_INFO.SwitchState.SpeedCommand = false;
  GPS_INFO.SwitchState.UserSwitchUp = false;
  GPS_INFO.SwitchState.UserSwitchMiddle = false;
  GPS_INFO.SwitchState.UserSwitchDown = false;
  GPS_INFO.SwitchState.VarioCircling = false;

#ifdef _SIM_
  SYSTEMTIME pda_time;
  GetSystemTime(&pda_time);

  GPS_INFO.Time  = pda_time.wHour*3600+pda_time.wMinute*60+pda_time.wSecond;
  GPS_INFO.Year  = pda_time.wYear;
  GPS_INFO.Month = pda_time.wMonth;
  GPS_INFO.Day	 = pda_time.wDay;
  #if _SIM_STARTUPSPEED
  GPS_INFO.Speed = _SIM_STARTUPSPEED;
  #endif
  #if _SIM_STARTUPALTITUDE
  GPS_INFO.Altitude = _SIM_STARTUPALTITUDE;
  #endif
#endif

#ifdef DEBUG
  DebugStore("# Start\r\n");
#endif

  LoadWindFromRegistry();
  CalculateNewPolarCoef();
  GlidePolar::SetBallast();

  OpenTerrain();

  ReadWayPoints();
  if(NumberOfWayPoints)
    {
      SetHome();
    }

  ReadAirfieldFile();
  ReadAirspace();
  SortAirspace();

  OpenTopology();
  ReadTopology();

  OpenFLARMDetails();

  VarioSound_Init();
  VarioSound_EnableSound(EnableSoundVario);
  VarioSound_SetVdead(SoundDeadband);
  VarioSound_SetV(0);

  VarioSound_SetSoundVolume(SoundVolume);

  // ... register all supported devices
  cai302Register();
  ewRegister();
  atrRegister();
  vgaRegister();

  //JMW disabled  devInit(lpCmdLine);

#ifndef _SIM_
  RestartCommPorts();
#endif
#if (WINDOWSPC>0)
  devInit(TEXT(""));      
#endif

  // re-set polar in case devices need the data
  GlidePolar::SetBallast();

#if (EXPERIMENTAL > 0)
  CreateProgressDialog(gettext(TEXT("Bluetooth dialup SMS")));
  bsms.Initialise();
#endif

  CreateProgressDialog(gettext(TEXT("Initialising display")));

  // just about done....

  DoSunEphemeris(147.0,-36.0);

  // Finally ready to go
  MapWindow::CreateDrawingThread();
  GlobalRunning = true;
  CloseProgressDialog();
  Sleep(100);
  ShowInfoBoxes();

  SwitchToMapWindow();
  CreateCalculationThread();
  Sleep(500);

  // Da-da, start everything now
  ProgramStarted = true;

  // NOTE: Must show errors AFTER all windows ready

  int olddelay = StatusMessageData[0].delay_ms;
  StatusMessageData[0].delay_ms = 20000; // 20 seconds

#ifdef _SIM_
  InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
#else
  InputEvents::processGlideComputer(GCE_STARTUP_REAL);
#endif
  StatusMessageData[0].delay_ms = olddelay; 

#ifdef _INPUTDEBUG_
  InputEvents::showErrors();
#endif

  // Main message loop:
  while (GetMessage(&msg, NULL, 0, 0))
    {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
    }

  return msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    It is important to call this function so that the application
//    will get 'well formed' small icons associated with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{

  WNDCLASS wc;
  WNDCLASS dc;

  GetClassInfo(hInstance,TEXT("DIALOG"),&dc);

  wc.style                      = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc                = (WNDPROC) WndProc;
  wc.cbClsExtra                 = 0;
#if (WINDOWSPC>0)
  wc.cbWndExtra = 0;
#else
  wc.cbWndExtra                 = dc.cbWndExtra ;
#endif
  wc.hInstance                  = hInstance;
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOARSWIFT));
  wc.hCursor                    = 0;
  wc.hbrBackground              = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName               = 0;
  wc.lpszClassName              = szWindowClass;

  if (!RegisterClass (&wc))
    return FALSE;

  wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = (WNDPROC)MapWindow::MapWndProc;
  wc.cbClsExtra = 0;

#if (WINDOWSPC>0)
  wc.cbWndExtra = 0 ;
#else
  wc.cbWndExtra = dc.cbWndExtra ;
#endif

  wc.hInstance = hInstance;
  wc.hIcon = (HICON)NULL;
  wc.hCursor = NULL;
  wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = TEXT("MapWindowClass");

  return RegisterClass(&wc);

}


void ApplyClearType(LOGFONT *logfont) {
  logfont->lfQuality = ANTIALIASED_QUALITY;
  if (0) {
#ifndef NOCLEARTYPE
  if (!InfoBoxLayout::landscape) {
    logfont->lfQuality = CLEARTYPE_COMPAT_QUALITY;
  }
#endif
  }
}


static void InitialiseFonts(RECT rc) {
  LOGFONT logfont;
  int FontHeight, FontWidth;
  int fontsz1 = (rc.bottom - rc.top );
  int fontsz2 = (rc.right - rc.left );

  if (fontsz1>fontsz2) {
    FontHeight = (int)(fontsz1/FONTHEIGHTRATIO);
    FontWidth = (int)(FontHeight*0.4);
  } else {
    FontHeight = (int)(fontsz2/FONTHEIGHTRATIO);
    FontWidth = (int)(FontHeight*0.4);
  }

  int iFontHeight = (int)(FontHeight*1.4);
  // oversize first so can then scale down

  FontWidth = 0; // JMW this should be done so closest font is found

  // sgi todo

  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T("DejaVu Sans Condensed"));

  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = iFontHeight;
  logfont.lfWidth =  FontWidth;
  logfont.lfWeight = FW_BOLD;
  logfont.lfItalic = TRUE;
  logfont.lfCharSet = ANSI_CHARSET;
  ApplyClearType(&logfont);

  //  #if NEWINFOBOX > 0
  // todo
  //  #else
  // JMW algorithm to auto-size info window font.
  // this is still required in case title font property doesn't exist.
  SIZE tsize;
  HDC iwhdc = GetDC(hWndMainWindow);
  do {
    iFontHeight--;
    logfont.lfHeight = iFontHeight;
    InfoWindowFont = CreateFontIndirect (&logfont);
    SelectObject(iwhdc, InfoWindowFont);
    GetTextExtentPoint(iwhdc, TEXT("00:00"), 5, &tsize);
    DeleteObject(InfoWindowFont);
  } while (tsize.cx>InfoBoxLayout::ControlWidth);
  ReleaseDC(hWndMainWindow, iwhdc);
  //  #endif

  iFontHeight++;
  logfont.lfHeight = iFontHeight;

  propGetFontSettings(TEXT("InfoWindowFont"), &logfont);
  InfoWindowFont = CreateFontIndirect (&logfont);

  // next font..

#if (WINDOWSPC>0)
  FontHeight= (int)(FontHeight/1.35);
  FontWidth= (int)(FontWidth/1.35);
#endif

  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight/TITLEFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth/TITLEFONTWIDTHRATIO);
  logfont.lfWeight = FW_BOLD;
  //  ApplyClearType(&logfont);

  propGetFontSettings(TEXT("TitleWindowFont"), &logfont);
  TitleWindowFont = CreateFontIndirect (&logfont);

  memset ((char *)&logfont, 0, sizeof (logfont));

  // new font for CDI Scale

  logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*CDIFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*CDIFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
  ApplyClearType(&logfont);

  propGetFontSettings(TEXT("CDIWindowFont"), &logfont);
  CDIWindowFont = CreateFontIndirect (&logfont);

  // new font for map labels
  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*MAPFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*MAPFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
  logfont.lfItalic = TRUE; // JMW
  ApplyClearType(&logfont);

  propGetFontSettings(TEXT("MapLabelFont"), &logfont);
  MapLabelFont = CreateFontIndirect (&logfont);


  // Font for map other text
  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*STATISTICSFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*STATISTICSFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
  ApplyClearType(&logfont);

  propGetFontSettings(TEXT("StatisticsFont"), &logfont);
  StatisticsFont = CreateFontIndirect (&logfont);

  // new font for map labels

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*MAPFONTHEIGHTRATIO*1.3);
  logfont.lfWidth =  (int)(FontWidth*MAPFONTWIDTHRATIO*1.3);
  logfont.lfWeight = FW_MEDIUM;
  ApplyClearType(&logfont);

  propGetFontSettings(TEXT("MapWindowFont"), &logfont);
  MapWindowFont = CreateFontIndirect (&logfont);

  SendMessage(hWndMapWindow,WM_SETFONT,
              (WPARAM)MapWindowFont,MAKELPARAM(TRUE,0));

  // Font for map bold text

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfWeight = FW_BOLD;
  logfont.lfWidth =  0; // JMW (int)(FontWidth*MAPFONTWIDTHRATIO*1.3) +2;

  propGetFontSettings(TEXT("MapWindowBoldFont"), &logfont);
  MapWindowBoldFont = CreateFontIndirect (&logfont);

  #if NEWINFOBOX > 0
  // NOP not needed
  #else
  int i;
  for(i=0;i<numInfoWindows;i++)
    {
      SendMessage(hWndInfoWindow[i],WM_SETFONT,
		  (WPARAM)InfoWindowFont,MAKELPARAM(TRUE,0));
      SendMessage(hWndTitleWindow[i],WM_SETFONT,
		  (WPARAM)TitleWindowFont,MAKELPARAM(TRUE,0));
    }
  #endif
}

//
//  FUNCTION: InitInstance(HANDLE, int)
//
//  PURPOSE: Saves instance handle and creates main window
//
//  COMMENTS:
//
//    In this function, we save the instance handle in a global variable and
//    create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  TCHAR szTitle[MAX_LOADSTRING];                        // The title bar text
  TCHAR szWindowClass[MAX_LOADSTRING];                  // The window class name
  RECT rc;

  hInst = hInstance;            // Store instance handle in our global variable
  LoadString(hInstance, IDC_XCSOAR, szWindowClass, MAX_LOADSTRING);
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

  //If it is already running, then focus on the window
  hWndMainWindow = FindWindow(szWindowClass, szTitle);
  if (hWndMainWindow)
    {
      SetForegroundWindow((HWND)((ULONG) hWndMainWindow | 0x00000001));
      return 0;
    }

  PreloadInitialisation(true);

  MyRegisterClass(hInst, szWindowClass);

  RECT WindowSize;

  WindowSize.left = 0;
  WindowSize.top = 0;
  WindowSize.right = GetSystemMetrics(SM_CXSCREEN);
  WindowSize.bottom = GetSystemMetrics(SM_CYSCREEN);

  #ifdef SCREENWIDTH
    WindowSize.right = SCREENWIDTH + 2*GetSystemMetrics( SM_CXFIXEDFRAME);
    WindowSize.left = (GetSystemMetrics(SM_CXSCREEN) - WindowSize.right) / 2;
  #endif
  #ifdef SCREENHEIGHT
    WindowSize.bottom = SCREENHEIGHT + 2*GetSystemMetrics( SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);
    WindowSize.top = (GetSystemMetrics(SM_CYSCREEN) - WindowSize.bottom) / 2;
  #endif

  hWndMainWindow = CreateWindow(szWindowClass, szTitle,
				WS_SYSMENU|WS_CLIPCHILDREN
				| WS_CLIPSIBLINGS,
                                WindowSize.left, WindowSize.top,
				WindowSize.right, WindowSize.bottom,
                                NULL, NULL,
				hInstance, NULL);

  if (!hWndMainWindow)
    {
      return FALSE;
    }

  SendMessage(hWndMainWindow, WM_SETICON,
	      (WPARAM)ICON_BIG, (LPARAM)IDI_XCSOARSWIFT);
  SendMessage(hWndMainWindow, WM_SETICON,
	      (WPARAM)ICON_SMALL, (LPARAM)IDI_XCSOARSWIFT);

  hBrushSelected = (HBRUSH)CreateSolidBrush(ColorSelected);
  hBrushUnselected = (HBRUSH)CreateSolidBrush(ColorUnselected);
  hBrushButton = (HBRUSH)CreateSolidBrush(ColorButton);

  GetClientRect(hWndMainWindow, &rc);

#if (WINDOWSPC>0)
  rc.left = 0;
  rc.right = SCREENWIDTH;
  rc.top = 0;
  rc.bottom = SCREENHEIGHT;
#endif
  InfoBoxLayout::ScreenGeometry(rc);

  ///////////////////////////////////////// create infoboxes

  Units::LoadUnitBitmap(hInstance);
  InfoBoxLayout::CreateInfoBoxes(rc);

  GaugeFLARM::Create();

  ButtonLabel::CreateButtonLabels(rc);
  ButtonLabel::SetLabelText(0,TEXT("MODE"));


  ////////////////// do fonts
  InitialiseFonts(rc);

  #if NEWINFOBOX > 0
  ButtonLabel::SetFont(MapWindowBoldFont);
  #endif

  Message::Initialize(rc); // creates window, sets fonts

  ShowWindow(hWndMainWindow, SW_SHOW);

  ///////////////////////////////////////////////////////
  //// create map window

  hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,
			       WS_VISIBLE | WS_CHILD
			       | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                               0, 0, (rc.right - rc.left),
			       (rc.bottom-rc.top) ,
                               hWndMainWindow, NULL ,hInstance,NULL);

  hWndMenuButton = CreateWindow(TEXT("BUTTON"),gettext(TEXT("Menu")),
				WS_VISIBLE | WS_CHILD
				| WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                0, 0,0,0,hWndMainWindow,NULL,hInstance,NULL);

  SendMessage(hWndMenuButton,WM_SETFONT,(WPARAM)TitleWindowFont,
	      MAKELPARAM(TRUE,0));

  // JMW moved menu button to center, to make room for thermal indicator

  int menubuttonsize = (int)(max(InfoBoxLayout::ControlWidth,
				 InfoBoxLayout::ControlHeight)*0.8);

  SetWindowPos(hWndMenuButton,HWND_TOP,
               (int)(rc.right-rc.left-menubuttonsize)/2+rc.left,
               (int)((rc.bottom - rc.top)/10),
	       menubuttonsize, menubuttonsize,
	       SWP_SHOWWINDOW);

  GaugeCDI::Create();
  GaugeVario::Create();

  if (EnableVarioGauge) {
    ShowWindow(hWndVarioWindow,SW_SHOW);
  } else {
    ShowWindow(hWndVarioWindow,SW_HIDE);
  }

  ShowWindow(hWndMenuButton, SW_HIDE);
  
  ShowWindow(hWndMainWindow, nCmdShow);
  UpdateWindow(hWndMainWindow);
    
#if NEWINFOBOX>0
  // NOP not needed
#else
  for(int i=0;i<numInfoWindows;i++)
    {
      UpdateWindow(hWndInfoWindow[i]);
      UpdateWindow(hWndTitleWindow[i]);
    }
#endif
  
  FullScreen();

  return TRUE;
}


int getInfoType(int i) {
  if (i<0) return 0; // error

  if (EnableAuxiliaryInfo) {
    return (InfoType[i] >> 24) & 0xff; // auxiliary
  } else {
    if (CALCULATED_INFO.Circling == TRUE)
      return InfoType[i] & 0xff; // climb
    else if (CALCULATED_INFO.FinalGlide == TRUE) {
      return (InfoType[i] >> 16) & 0xff; //final glide
    } else {
      return (InfoType[i] >> 8) & 0xff; // cruise
    }
  }
}


void setInfoType(int i, char j) {
  if (i<0) return; // error

  if (EnableAuxiliaryInfo) {
    InfoType[i] &= 0x00ffffff;
    InfoType[i] += (j<<24);
  } else {
    if (CALCULATED_INFO.Circling == TRUE) {
      InfoType[i] &= 0xffffff00;
      InfoType[i] += (j);
    } else if (CALCULATED_INFO.FinalGlide == TRUE) {
      InfoType[i] &= 0xff00ffff;
      InfoType[i] += (j<<16);
    } else {
      InfoType[i] &= 0xffff00ff;
      InfoType[i] += (j<<8);
    }
  }
}


void DoInfoKey(int keycode) {
  int i;

  if (InfoFocus<0) return; // paranoid

  HideMenu();

  LockNavBox();
  i = getInfoType(InfoFocus);

  // XXX This could crash if MapWindow does not capture

  LockFlightData();
  Data_Options[i].Process(keycode);
  UnlockFlightData();

  UnlockNavBox();

  InfoBoxesDirty = true;

  SetEvent(dataTriggerEvent);
  NMEAParser::GpsUpdated = TRUE; // emulate update to trigger calculations

  InfoBoxFocusTimeOut = 0;
  DisplayTimeOut = 0;

}


// Debounce input buttons (does not matter which button is pressed)

int debounceTimeout=200;

bool Debounce(void) {
  static DWORD fpsTimeLast= 0;
  DWORD fpsTimeThis = ::GetTickCount();
  DWORD dT = fpsTimeThis-fpsTimeLast;

  DisplayTimeOut = 0;

  if (ScreenBlanked) {
    // prevent key presses working if screen is blanked,
    // so a key press just triggers turning the display on again
    return false;
  }

  if (dT>(unsigned int)debounceTimeout) {
    fpsTimeLast = fpsTimeThis;
    return true;
  } else {
    return false;
  }
}


void Shutdown(void) {
  int i;

  // turn off all displays
  GlobalRunning = false;

  // Save settings

  StoreRegistry();

  // Stop sound

  SaveSoundSettings();
  
  VarioSound_EnableSound(false);
  
  VarioSound_Close();

  // Stop SMS device
#if (EXPERIMENTAL > 0)
  bsms.Shutdown();
#endif
  
  // Stop drawing
  
  MapWindow::CloseDrawingThread();

  // Stop calculating too (wake up)
  SetEvent(dataTriggerEvent);
  SetEvent(drawTriggerEvent);
  SetEvent(varioTriggerEvent);

  // Clear data
  Task[0].Index = -1;  ActiveWayPoint = -1; 
  CloseWayPoints();
  AATEnabled = FALSE;
  NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0; 
  NumberOfAirspaceCircles = 0;
  CloseTerrain();
  CloseTopology();
  CloseTerrainRenderer();

  // Stop COM devices
  devCloseAll();

  #if GNAV
    InputEvents::eventDLLExecute(TEXT("altairplatform.dll SetShutdown 1"));
    while(1);
  #endif

    
#if (WINDOWSPC<1)
  if(Port1Available)
    Port1Close();
  if (Port2Available)
    Port2Close();
#endif

  CloseFLARMDetails();

  // Kill windows

  GaugeCDI::Destroy();
  GaugeVario::Destroy();
  GaugeFLARM::Destroy();
  
  Message::Destroy();
  
  Units::UnLoadUnitBitmap();
  
#if NEWINFOBOX > 0
  InfoBoxLayout::DestroyInfoBoxes();
#else
  for(i=0;i<numInfoWindows;i++)
    {
      DestroyWindow(hWndInfoWindow[i]);
      DestroyWindow(hWndTitleWindow[i]);
    }
#endif
  
  ButtonLabel::Destroy();
  
  CommandBar_Destroy(hWndCB);
  for (i=0; i<NUMSELECTSTRINGS; i++) {
    delete Data_Options[i].Formatter;
  }

  // Kill graphics objects

  DeleteObject(hBrushSelected);
  DeleteObject(hBrushUnselected);
  DeleteObject(hBrushButton);
  
  DeleteObject(InfoWindowFont);
  DeleteObject(TitleWindowFont);
  DeleteObject(CDIWindowFont);
  DeleteObject(MapLabelFont);
  DeleteObject(MapWindowFont);
  DeleteObject(MapWindowBoldFont);
  DeleteObject(StatisticsFont);  
  
  if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
  if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
  if(AirspaceScreenPoint != NULL)  LocalFree((HLOCAL)AirspaceScreenPoint);
  if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);
  
  DeleteCriticalSection(&CritSec_FlightData);
  DeleteCriticalSection(&CritSec_NavBox);
  DeleteCriticalSection(&CritSec_Comm);
  DeleteCriticalSection(&CritSec_TerrainDataCalculations);
  DeleteCriticalSection(&CritSec_TerrainDataGraphics);
  
  CloseProgressDialog();

  CloseCalculations();

  DestroyWindow(hWndMapWindow);
  DestroyWindow(hWndMainWindow);
  DestroyWindow(hWndMenuButton);
      
  CloseHandle(drawTriggerEvent);
  CloseHandle(dataTriggerEvent);
  CloseHandle(varioTriggerEvent);

#if (WINDOWSPC>0) 
#if _DEBUG
  _CrtDumpMemoryLeaks();
#endif
#endif
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static bool lastpress = false;
  long wdata;

  switch (message)
    {

    case WM_ERASEBKGND:
      return TRUE; // JMW trying to reduce screen flicker
      break;
    case WM_COMMAND:
      return MainMenu(hWnd, message, wParam, lParam);
      break;
    case WM_CTLCOLORSTATIC:
      wdata = GetWindowLong((HWND)lParam, GWL_USERDATA);
      if (wdata==1) {
        SetBkColor((HDC)wParam, ColorSelected);
        SetTextColor((HDC)wParam, RGB(0x00,0x00,0x00));
        return (LRESULT)hBrushSelected;
      }
      if (wdata==0) {
        SetBkColor((HDC)wParam, ColorUnselected);
        SetTextColor((HDC)wParam, RGB(0x00,0x00,0x00));
        return (LRESULT)hBrushUnselected;
      }
      if (wdata==2) {
	SetBkColor((HDC)wParam, ColorUnselected);
        SetTextColor((HDC)wParam, ColorWarning);
	return (LRESULT)hBrushUnselected;
      }
      if (wdata==3) {
	SetBkColor((HDC)wParam, ColorUnselected);
        SetTextColor((HDC)wParam, ColorOK);
	return (LRESULT)hBrushUnselected;
      }
      if (wdata==4) { // this one used for buttons
	// black on light green
	SetBkColor((HDC)wParam, ColorButton);
        SetTextColor((HDC)wParam, RGB(0x00,0x00,0x00));
	return (LRESULT)hBrushButton;
      }
      break;
    case WM_CREATE:
      memset (&s_sai, 0, sizeof (s_sai));
      s_sai.cbSize = sizeof (s_sai);

      iTimerID = SetTimer(hWnd,1000,500,NULL); // 2 times per second

      hWndCB = CreateRpCommandBar(hWnd);

      break;

    case WM_ACTIVATE:
      if(LOWORD(wParam) != WA_INACTIVE)
        {
          SetWindowPos(hWndMainWindow,HWND_TOP,
                 0, 0, 0, 0,
                 SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
          if(TopWindow)
            SHFullScreen(hWndMainWindow,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
          else
            SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR|SHFS_SHOWSIPBUTTON|SHFS_SHOWSTARTICON);
        }
      SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
      break;

    case WM_SETTINGCHANGE:
      SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
      break;

    case WM_SETFOCUS:
      // JMW not sure this ever does anything useful..
      if (ProgramStarted) {

	if(InfoWindowActive) {
	  
	  if(DisplayLocked) {
	    FocusOnWindow(InfoFocus,true);
	  } else {
	    FocusOnWindow(InfoFocus,true);
	  }
	} else {
	  DefocusInfoBox();
	  HideMenu();
	  SetFocus(hWndMapWindow);
	}
      }
      break;

      // TODO Capture KEYDOWN time
      // 	- Pass that (otpionally) to processKey, allowing
      // 	  processKey to handle long events - at any length
      // 	- Not sure how to do double click... (need timer call back
      // 	process unless reset etc... tricky)
#ifdef GNAV
    case WM_KEYDOWN: // JMW was keyup
#else
    case WM_KEYUP: // JMW was keyup
#endif
      /* DON'T PROCESS KEYS HERE WITH NEWINFOBOX, IT CAUSES CRASHES! */
#if (NEWINFOBOX<1)       
      if (ProgramStarted) {
	if (!DialogActive) {
	  
	  if (InputEvents::processKey(wParam)) {
	    //	  TODO debugging - DoStatusMessage(TEXT("Event in infobox"));
	  }
	} else {
	  //	TODO debugging - DoStatusMessage(TEXT("Event in dlg"));
	  if (InputEvents::processKey(wParam)) {
	  }
	}
	return TRUE; // JMW trying to fix multiple processkey bug
      }
#endif 
      break;
    case WM_TIMER:
      if (ProgramStarted) {
#ifdef _SIM_
	SIMProcessTimer();
#else
	ProcessTimer();
#endif
      }
      break;

    case WM_INITMENUPOPUP:
      if (ProgramStarted) {
	if(DisplayLocked)
	  CheckMenuItem((HMENU) wParam,IDM_FILE_LOCK,MF_CHECKED|MF_BYCOMMAND);
	else
	  CheckMenuItem((HMENU) wParam,IDM_FILE_LOCK,MF_UNCHECKED|MF_BYCOMMAND);
	
	if(LoggerActive)
	  CheckMenuItem((HMENU) wParam,IDM_FILE_LOGGER,MF_CHECKED|MF_BYCOMMAND);
	else
	  CheckMenuItem((HMENU) wParam,IDM_FILE_LOGGER,MF_UNCHECKED|MF_BYCOMMAND);
      }
      break;

    case WM_CLOSE:
      if(iTimerID)
        KillTimer(hWnd,iTimerID);

      Shutdown();
      break;

    case WM_DESTROY:
      CommandBar_Destroy(hWndCB);
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  return 0;
}



HWND CreateRpCommandBar(HWND hwnd)
{
  SHMENUBARINFO mbi;

  memset(&mbi, 0, sizeof(SHMENUBARINFO));
  mbi.cbSize     = sizeof(SHMENUBARINFO);
  mbi.hwndParent = hwnd;
  mbi.dwFlags = SHCMBF_EMPTYBAR|SHCMBF_HIDDEN;
  mbi.nToolBarId = IDM_MENU;
  mbi.hInstRes   = hInst;
  mbi.nBmpId     = 0;
  mbi.cBmpImages = 0;

  if (!SHCreateMenuBar(&mbi))
    return NULL;

  return mbi.hwndMB;
}


LRESULT MainMenu(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  HWND wmControl;
  int i;
  WORD wID;

  wmId    = LOWORD(wParam);
  wmEvent = HIWORD(wParam);
  wmControl = (HWND)lParam;

  if(wmControl != NULL)
    {
      if (ProgramStarted) {

      if(wmControl == hWndMenuButton)
        {
	  if (InfoBoxLayout::landscape) {
	    wID = DialogBox(hInst, (LPCTSTR)IDD_MENU_LANDSCAPE,
			    hWnd, (DLGPROC)Menu);
	  } else {
	    wID = DialogBox(hInst, (LPCTSTR)IDD_MENU, hWnd, (DLGPROC)Menu);
	  }
          DialogActive = true;

          switch (wID)
            {
            case IDD_EXIT:
              if(
                #ifdef _SIM_
                (true)
                #else
		MessageBoxX(hWnd,
			    gettext(TEXT("Do you wish to exit?")),
			    gettext(TEXT("Exit?")),
			    MB_YESNO|MB_ICONQUESTION) == IDYES
                #endif
		) {

		SendMessage(hWnd, 
			    WM_ACTIVATE, 
			    MAKEWPARAM(WA_INACTIVE, 0), 
			    (LPARAM)hWnd);
		SendMessage (hWnd, WM_CLOSE, 0, 0);
	      }
              MapWindow::MapDirty = true;
	      HideMenu();
              FullScreen();
	      Debounce();
              DialogActive = false;
              return 0;

	    case IDD_BACK:
	      HideMenu();
	      FullScreen();
	      Debounce();
              DialogActive = false;
	      return 0;

            case IDD_STATUS:
              ShowStatus();
              MenuActive = false;
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
	      Debounce();
              DialogActive = false;
              return 0;

            case IDD_BUGS:
              DWORD dwError;
              MenuActive = true;

              ShowWindow(hWndCB,SW_SHOW);
              SHFullScreen(hWndMainWindow,SHFS_SHOWSIPBUTTON|SHFS_SHOWTASKBAR);
              DialogBox(hInst, (LPCTSTR)IDD_BUGSBALLAST, hWnd, (DLGPROC)SetBugsBallast);
              dwError = GetLastError();
              ShowWindow(hWndCB,SW_HIDE);
              MenuActive = false;
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
	      Debounce();
              DialogActive = false;
              return 0;

            case IDD_PRESSURE:
              MenuActive = true;
              ShowWindow(hWndCB,SW_SHOW);
              SHFullScreen(hWndMainWindow,SHFS_SHOWSIPBUTTON|SHFS_SHOWTASKBAR);
              DialogBox(hInst, (LPCTSTR)IDD_AIRSPACEPRESS, hWnd, (DLGPROC)AirspacePress);
              ConvertFlightLevels();
              ShowWindow(hWndCB,SW_HIDE);
              MenuActive = false;
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
	      Debounce();
              DialogActive = false;
              return 0;

            case IDD_TASK:
              MenuActive = true;
              SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR);
              DialogBox(hInst, (LPCTSTR)IDD_TASK, hWnd, (DLGPROC)SetTask);
              ShowWindow(hWndCB,SW_HIDE);
              MenuActive = false;
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
	      Debounce();
              DialogActive = false;
              return 0;

            case IDD_LOCK:
              DisplayLocked = ! DisplayLocked;
              ShowWindow(hWndCB,SW_HIDE);
              SwitchToMapWindow();
	      if (!DisplayLocked) {
		ShowMenu(); // must show menu here otherwise trapped
	      } else {
		HideMenu();
	      }
	      FullScreen();
              DialogActive = false;
	      Debounce();
              return 0;

            case IDC_ABORTTASK:

              LockFlightData();
              ResumeAbortTask();
              UnlockFlightData();

              ShowWindow(hWndCB,SW_HIDE);
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
              DialogActive = false;
	      Debounce();
              return 0;

            case IDC_ANALYSIS:

              ShowWindow(hWndCB,SW_HIDE);
	      FullScreen();
              PopupAnalysis();
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
              DialogActive = false;
	      Debounce();
              return 0;

            case IDD_SETTINGS:

	      if (LockSettingsInFlight && CALCULATED_INFO.Flying) {
		DoStatusMessage(TEXT("Settings locked in flight"));
	      } else {

		SettingsEnter();

		ShowWindow(hWndCB,SW_SHOW);
		SetWindowPos(hWndMainWindow,HWND_TOP,
			     0, 0, 0, 0,
			     SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
		
		SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR);
		DialogBox(hInst, (LPCTSTR)IDD_SETTINGS, hWndMainWindow, (DLGPROC)Settings);
		ShowWindow(hWndCB,SW_HIDE);
		
		SettingsLeave();
		StoreRegistry();
	      }
		
		SwitchToMapWindow();
		FullScreen();
		ShowWindow(hWndCB,SW_HIDE);
		HideMenu();
		DialogActive = false;
		Debounce();
		return 0;

            case IDD_LOGGER:
              MenuActive = true;
	      DialogActive = true;

	      guiToggleLogger(true);

              MenuActive = false;
	      FullScreen();
              SwitchToMapWindow();
	      HideMenu();
	      Debounce();
              DialogActive = false;
              return 0;
            }
        }
      DialogActive = false;

      FullScreen();

      InfoBoxFocusTimeOut = 0;
      if (!InfoWindowActive) {
        ShowMenu();
      }

      for(i=0;i<numInfoWindows;i++)
        {
          #if NEWINFOBOX > 0
          if(wmControl == InfoBoxes[i]->GetHandle())
          #else
          if(wmControl == hWndInfoWindow[i])
          #endif
            {
              InfoWindowActive = TRUE;
              #if NEWINFOBOX > 0
              #else
              SetFocus(hWnd);
              #endif

              if(DisplayLocked)
                {
                  if( i!= InfoFocus)
                    {
                      FocusOnWindow(i,true);
                      FocusOnWindow(InfoFocus,false);

                      InfoFocus = i;
                      InfoWindowActive = TRUE;
                    }
                  DisplayText();
                  InputEvents::setMode(TEXT("infobox"));

                }
	            else
                {
                  PopUpSelect(i);
                  DisplayText();
                }
              return 0;
            }
        }
      Message::CheckTouch(wmControl);
      
      if (ButtonLabel::CheckButtonPress(wmControl)) {
        return TRUE; // don't continue processing..
      }

      }
    }
  return DefWindowProc(hWnd, message, wParam, lParam);
}


void ProcessChar1 (char c)
{
  static TCHAR BuildingString[100];
  static int i = 0;

  if (!ProgramStarted) return; // ignore everything until started

  if (i<90) {
    if(c=='\n') {
      BuildingString[i] = '\0';
      LockFlightData();
      devParseNMEA(0, BuildingString, &GPS_INFO);
      UnlockFlightData();
    } else {
      BuildingString[i++] = c;
      return;
    }
  }
  
  i = 0;
}


void ProcessChar2 (char c)
{
  static TCHAR BuildingString[100];
  static int i = 0;

  if (!ProgramStarted) return; // ignore everything until started

  if (i<90) {
    if(c=='\n') {
      BuildingString[i] = '\0';
      LockFlightData();
      devParseNMEA(1, BuildingString, &GPS_INFO);
      UnlockFlightData();
    } else {
      BuildingString[i++] = c;
      return;
    }
  }
  
  i = 0;
}

/* I think this is redundant now, since EW is handled in the devEW code
void ProcessChar2 (char c)
{
#define WAIT 0
#define FILL 1
  static TCHAR BuildingString[100];
  static int i = 0;
  static int State = WAIT;
  int OK_Flag = 1; // Set flag to failed state
  int IO_Flag = 1; // Set flag to failed state

  if(State == WAIT)
    {
      if(c=='$') // we're only going to parse NMEA strings here
        {
          BuildingString[0] = c;
          BuildingString[1] = '\0';
          i=1;
          State = FILL;
        }
    }
  else
    {
      if(i>90)
        {
          State = WAIT;
        }
      else
        {
          if(c=='\n')
            {
              BuildingString[i] = '\0';
              State = WAIT;

              //#ifdef DEBUG
              //              DebugStore(BuildingString);
              //#endif

              if(BuildingString[0]=='$')  // Additional "if" to find GPS strings
                {
                  LockFlightData();
		  NMEAParser::ParseNMEAString(1,
					      BuildingString,
					      &GPS_INFO);
                  UnlockFlightData();
                }
              else //   else parse EW logger string
                if(_tcscmp(BuildingString,TEXT("OK\r"))==0)     OK_Flag = 0;
              if(_tcscmp(BuildingString,TEXT("IO Mode.\r"))==0) IO_Flag = 0;
            }
          else
            {
              BuildingString[i++] = c;
            }
        }
    }
}
*/

extern int DetectStartTime();


void    AssignValues(void)
{
  if (InfoBoxesHidden) {
    // no need to assign values
    return;
  }

  DetectStartTime();

  // nothing to do here now!
}


void DisplayText(void)
{
  if (InfoBoxesHidden)
    return;

  int i;
  static TCHAR Caption[MAXINFOWINDOWS][100];
  static int DisplayType[MAXINFOWINDOWS];
  static bool first=true;
  static int InfoFocusLast = -1;
  static int DisplayTypeLast[MAXINFOWINDOWS];

  LockNavBox();

  // JMW note: this is updated every GPS time step

  if (InfoFocus != InfoFocusLast) {
    first = true; // force re-setting title
  }
  if ((InfoFocusLast>=0)&&(!InfoWindowActive)) {
    first = true;
  }
  InfoFocusLast = InfoFocus;

  for(i=0;i<numInfoWindows;i++) {
    Caption[i][0]= 0;
    
    if (EnableAuxiliaryInfo) {
      DisplayType[i] = (InfoType[i] >> 24) & 0xff;
    } else {
      if (CALCULATED_INFO.Circling == TRUE)
	DisplayType[i] = InfoType[i] & 0xff;
      else if (CALCULATED_INFO.FinalGlide == TRUE) {
	DisplayType[i] = (InfoType[i] >> 16) & 0xff;
      } else {
	DisplayType[i] = (InfoType[i] >> 8) & 0xff;
      }
    }
    
    Data_Options[DisplayType[i]].Formatter->AssignValue(DisplayType[i]);
    
#if NEWINFOBOX>0
    TCHAR sTmp[32];
#endif
    
    int color = 0;

    bool needupdate = ((DisplayType[i] != DisplayTypeLast[i])||first);

#if NEWINFOBOX>0
    
    // set values, title
    switch (DisplayType[i]) {
    case 14: // Next waypoint
      if (ActiveWayPoint != -1){
	InfoBoxes[i]->
	  SetTitle(Data_Options[DisplayType[i]].Formatter->
		   Render(&color));	  
	InfoBoxes[i]->
	  SetValue(Data_Options[47].Formatter->Render(&color));
	InfoBoxes[i]->SetColor(color);
      }else{
	InfoBoxes[i]->SetTitle(TEXT("Next"));
	InfoBoxes[i]->SetValue(TEXT("---"));
	InfoBoxes[i]->SetColor(0);
      }
	break;
    default:
      if (needupdate)
	InfoBoxes[i]->SetTitle(Data_Options[DisplayType[i]].Title);

      InfoBoxes[i]->
	SetValue(Data_Options[DisplayType[i]].Formatter->Render(&color));
      
      // to be optimized!
      if (needupdate)
	InfoBoxes[i]->
	  SetValueUnit(Units::GetUserUnitByGroup(
		       Data_Options[DisplayType[i]].UnitGroup));
      
      InfoBoxes[i]->SetColor(0);
    };
    
    switch (DisplayType[i]) {
    case 14: // Next waypoint
      if (ActiveWayPoint != -1){
	InfoBoxes[i]->
	  SetComment(WayPointList[ Task[ActiveWayPoint].Index ].Comment);
      }else{
	InfoBoxes[i]->SetComment(TEXT(""));
      }
      break;
    case 10:
      if (CALCULATED_INFO.AutoMacCready)
	InfoBoxes[i]->SetComment(TEXT("AUTO"));
      else
	InfoBoxes[i]->SetComment(TEXT("MANUAL"));
      break;
    case 0: // GPS Alt
      Units::FormatAlternateUserAltitude(GPS_INFO.Altitude, 
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
      InfoBoxes[i]->SetComment(sTmp);
      break;
    case 1: // AGL
      Units::FormatAlternateUserAltitude(CALCULATED_INFO.AltitudeAGL, 
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
      InfoBoxes[i]->SetComment(sTmp);
      break;
    case 33:
      Units::FormatAlternateUserAltitude(GPS_INFO.BaroAltitude, 
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
      InfoBoxes[i]->SetComment(sTmp);
      break;
    case 43:
      if (EnableBlockSTF) {
	InfoBoxes[i]->SetComment(TEXT("Block"));
      } else {
	InfoBoxes[i]->SetComment(TEXT("Dolphin"));
      }
      break;
    default:
      InfoBoxes[i]->SetComment(TEXT(""));
    };
    
#else
    Data_Options[DisplayType[i]].Formatter->Render(hWndInfoWindow[i]);

      // JMW only update captions if text has really changed.
      // this avoids unnecesary gettext lookups
      _stprintf(Caption[i],gettext(Data_Options[DisplayType[i]].Title) );
      SetWindowText(hWndTitleWindow[i],Caption[i]);
#endif

    DisplayTypeLast[i] = DisplayType[i];
    
  }

  first = false;

  UnlockNavBox();

}


void CommonProcessTimer()
{

#if (WINDOWSPC<1)
  SystemIdleTimerReset();
#endif

  if(InfoWindowActive)
    {
      if(InfoBoxFocusTimeOut == FOCUSTIMEOUTMAX)
        {
          SwitchToMapWindow();
        }
      InfoBoxFocusTimeOut ++;

    }

  if (DisplayLocked) {
    if(MenuTimeOut==MenuTimeoutMax) {
      ShowWindow(hWndMenuButton, SW_HIDE);
      if (!MapWindow::isPan()) {
	InputEvents::setMode(TEXT("default"));
      }
    }
    MenuTimeOut++;
  }

  if (DisplayTimeOut >= DISPLAYTIMEOUTMAX) {
    BlankDisplay(true);
  } else {
    BlankDisplay(false);
  }
  if (!DialogActive) {
    DisplayTimeOut++;
  } else {
    DisplayTimeOut=0;
  }

  if (MapWindow::IsDisplayRunning()) {
    // No need to redraw map or infoboxes if screen is blanked.
    // This should save lots of battery power due to CPU usage
    // of drawing the screen
    if (InfoBoxesDirty) {
      //JMWTEST    LockFlightData();
      AssignValues();
      DisplayText();
      InfoBoxesDirty = false;
      //JMWTEST    UnlockFlightData();
    }
  }

  //
  // maybe block/delay this if a dialog is active?
  //
  Message::Render();

#if (EXPERIMENTAL > 0)

  if (bsms.Poll()) {
    // turn screen on if blanked and receive a new message
    DisplayTimeOut = 0;
  }

#endif

}

void ProcessTimer(void)
{
#ifndef _SIM_

  CommonProcessTimer();

  // processing moved to its own thread

  // now check GPS status

  static int itimeout = -1;
  itimeout++;
  
  // write settings to vario every second
  if (itimeout %2==0) {
    VarioWriteSettings();
  }
    
  // also service replay logger
  ReplayLogger::Update();
  if (ReplayLogger::IsEnabled()) {
    static double timeLast = 0;
    if (GPS_INFO.Time-timeLast>0) {
      NMEAParser::GpsUpdated = TRUE;
      SetEvent(dataTriggerEvent);
    }
    timeLast = GPS_INFO.Time;
    GPSCONNECT = TRUE;
    extGPSCONNECT = TRUE;
    GPS_INFO.NAVWarning = FALSE;
    GPS_INFO.SatellitesUsed = 6;
    return;
  }
  
  if (itimeout % 10 != 0) {
    // timeout if no new data in 5 seconds
    return;
  }
  
  LockComm();
  NMEAParser::UpdateMonitor();
  UnlockComm();
  
  static BOOL LastGPSCONNECT = FALSE;
  static BOOL CONNECTWAIT = FALSE;
  static BOOL LOCKWAIT = FALSE;
  
  //
  // replace bool with BOOL to correct warnings and match variable
  // declarations RB
  //
  BOOL gpsconnect = GPSCONNECT;
  
  if (GPSCONNECT) {
    extGPSCONNECT = TRUE;
  }

  GPSCONNECT = FALSE;
  BOOL navwarning = (BOOL)(GPS_INFO.NAVWarning);
  
  if((gpsconnect == FALSE) && (LastGPSCONNECT == FALSE))
    {
      
      MapWindow::RequestFastRefresh();
      
      devLinkTimeout(devAll());

      if(LOCKWAIT == TRUE)
	{
	  // gps was waiting for fix, now waiting for connection
	  NMEAParser::GpsUpdated = true;
	  MapWindow::MapDirty = true;
	  SetEvent(drawTriggerEvent);
	  LOCKWAIT = FALSE;
	}
      if(!CONNECTWAIT)
	{
	  // gps is waiting for connection first time
	  
	  NMEAParser::GpsUpdated = true;
	  MapWindow::MapDirty = true;
	  SetEvent(drawTriggerEvent);
	  extGPSCONNECT = FALSE;
	  
	  InputEvents::processGlideComputer(GCE_GPS_CONNECTION_WAIT);

	  //            SetDlgItemText(hGPSStatus,IDC_GPSMESSAGE,szLoadText);
	  
            CONNECTWAIT = TRUE;
#ifndef DISABLEAUDIO
            MessageBeep(MB_ICONEXCLAMATION);
#endif
            FullScreen();
	    
	} else {
	
	if (itimeout % 30 == 0) {
	  // we've been waiting for connection a long time
	  
	  // no activity for 30 seconds, so assume PDA has been
	  // switched off and on again
	  //
#if (WINDOWSPC<1)
#ifndef GNAV
	  NMEAParser::GpsUpdated = true;
	  MapWindow::MapDirty = true;
	  SetEvent(drawTriggerEvent);

	  extGPSCONNECT = FALSE;
	  
	  InputEvents::processGlideComputer(GCE_COMMPORT_RESTART);
	  
	  RestartCommPorts();
#endif
#endif
	  
#if (EXPERIMENTAL > 0)
	  // if comm port shut down, probably so did bluetooth dialup
	  // so restart it here also.
	  bsms.Shutdown();
	  bsms.Initialise();
#endif
	  
	  itimeout = 0;
	}
      }
    }
  
  if((gpsconnect == TRUE) && (LastGPSCONNECT == FALSE))
    {
      itimeout = 0; // reset timeout
      
      if(CONNECTWAIT)
	{
	  NMEAParser::GpsUpdated = true;
	  MapWindow::MapDirty = true;
	  SetEvent(drawTriggerEvent);
	  CONNECTWAIT = FALSE;
	}
    }
  
  if((gpsconnect == TRUE) && (LastGPSCONNECT == TRUE))
    {
      if((navwarning == TRUE) && (LOCKWAIT == FALSE))
	{
	  InputEvents::processGlideComputer(GCE_GPS_FIX_WAIT);
	  
	  NMEAParser::GpsUpdated = true;
	  MapWindow::MapDirty = true;
	  SetEvent(drawTriggerEvent);
	  
	  LOCKWAIT = TRUE;
#ifndef DISABLEAUDIO
	  MessageBeep(MB_ICONEXCLAMATION);
#endif
	  FullScreen();
	  
	}
      else if((navwarning == FALSE) && (LOCKWAIT == TRUE))
	{
	  NMEAParser::GpsUpdated = true;
	  MapWindow::MapDirty = true;
	  SetEvent(drawTriggerEvent);
	  LOCKWAIT = FALSE;
	}
    }
  
  LastGPSCONNECT = gpsconnect;
  
#endif // end processing of non-simulation mode
  
}


void SIMProcessTimer(void)
{
#ifdef _SIM_

  CommonProcessTimer();

  GPSCONNECT = TRUE;
  extGPSCONNECT = TRUE;
  static int i=0;
  i++;

  if (!ReplayLogger::Update()) {

    if (i%2==0) return;

    LockFlightData();

    GPS_INFO.NAVWarning = FALSE;
    GPS_INFO.SatellitesUsed = 6;
    GPS_INFO.Latitude = 
      FindLatitude(GPS_INFO.Latitude, GPS_INFO.Longitude, 
		   GPS_INFO.TrackBearing, GPS_INFO.Speed*1.0 );
    GPS_INFO.Longitude = 
      FindLongitude(GPS_INFO.Latitude, GPS_INFO.Longitude, 
		    GPS_INFO.TrackBearing, GPS_INFO.Speed*1.0);
    GPS_INFO.Time+= 1.0;

    UnlockFlightData();
  }

  if (i%2==0) return;

#ifdef DEBUG
  //  NMEAParser::TestRoutine(&GPS_INFO);
#endif

  NMEAParser::GpsUpdated = TRUE;
  SetEvent(dataTriggerEvent);

  VarioWriteSettings();
#endif
}




void SwitchToMapWindow(void)
{
  DefocusInfoBox();

  SetFocus(hWndMapWindow);
  if (  MenuTimeOut< MenuTimeoutMax) {
    MenuTimeOut = MenuTimeoutMax;
  }
  if (  InfoBoxFocusTimeOut< FOCUSTIMEOUTMAX) {
    InfoBoxFocusTimeOut = FOCUSTIMEOUTMAX;
  }
}


void PopupAnalysis()
{
  DialogActive = true;
  #if NEWINFOBOX>0
  extern void dlgAnalysisShowModal(void);
  dlgAnalysisShowModal();
  /*
  if (InfoBoxLayout::landscape) {
    DialogBox(hInst, (LPCTSTR)IDD_ANALYSIS_LANDSCAPE, hWndMapWindow, (DLGPROC)AnalysisProc);
  } else {
    DialogBox(hInst, (LPCTSTR)IDD_ANALYSIS, hWndMapWindow, (DLGPROC)AnalysisProc);
  }
  */
  #else
  if (InfoBoxLayout::landscape) {
    DialogBox(hInst, (LPCTSTR)IDD_ANALYSIS_LANDSCAPE, hWndInfoWindow[0],
	      (DLGPROC)AnalysisProc);
  } else {
    DialogBox(hInst, (LPCTSTR)IDD_ANALYSIS, hWndInfoWindow[0],
	      (DLGPROC)AnalysisProc);
  }
  #endif
  DialogActive = false;
}


void PopupWaypointDetails()
{
#if NEWINFOBOX>0
  extern void dlgWayPointDetailsShowModal(void);

  dlgWayPointDetailsShowModal();
#else

	if (SelectedWaypoint<0)
		return;

  DialogActive = true;
  #if NEWINFOBOX>0
  if (InfoBoxLayout::landscape) {
    DialogBox(hInst, (LPCTSTR)IDD_WAYPOINTDETAILS_LANDSCAPE,
	      hWndMapWindow, (DLGPROC)WaypointDetails);
  } else {
    DialogBox(hInst, (LPCTSTR)IDD_WAYPOINTDETAILS,
	      hWndMapWindow, (DLGPROC)WaypointDetails);
  }
  #else
  if (InfoBoxLayout::landscape) {
    DialogBox(hInst, (LPCTSTR)IDD_WAYPOINTDETAILS_LANDSCAPE,
	      hWndInfoWindow[0], (DLGPROC)WaypointDetails);
  } else {
    DialogBox(hInst, (LPCTSTR)IDD_WAYPOINTDETAILS,
	      hWndInfoWindow[0], (DLGPROC)WaypointDetails);
  }
  #endif
  DialogActive = false;
#endif
}


void PopupBugsBallast(int UpDown)
{
  DialogActive = true;
  #if NEWINFOBOX>0
  DialogBox(hInst, (LPCTSTR)IDD_BUGSBALLAST, hWndMapWindow, (DLGPROC)SetBugsBallast);
  #else
  DialogBox(hInst, (LPCTSTR)IDD_BUGSBALLAST, hWndInfoWindow[0], (DLGPROC)SetBugsBallast);
  #endif
  ShowWindow(hWndCB,SW_HIDE);
  FullScreen();
  SwitchToMapWindow();
  DialogActive = false;
}


void PopUpSelect(int Index)
{
  DialogActive = true;
  CurrentInfoType = InfoType[Index];
  #if NEWINFOBOX>0
  InfoType[Index] = DialogBox(hInst, (LPCTSTR)IDD_SELECT, hWndMapWindow, (DLGPROC)Select);
  #else
  InfoType[Index] = DialogBox(hInst, (LPCTSTR)IDD_SELECT, hWndInfoWindow[Index], (DLGPROC)Select);
  #endif
  StoreType(Index, InfoType[Index]);
  ShowWindow(hWndCB,SW_HIDE);
  FullScreen();
  SwitchToMapWindow();
  DialogActive = false;
}

#include <stdio.h>

void DebugStore(char *Str)
{
#ifdef DEBUG
  FILE *stream;
  static TCHAR szFileName[] = TEXT("\\xcsoar-debug.log");

  stream = _wfopen(szFileName,TEXT("a+"));

  fwrite(Str,strlen(Str),1,stream);

  fclose(stream);
#endif
}


void LockNavBox() {
}

void UnlockNavBox() {
}


void LockFlightData() {
  EnterCriticalSection(&CritSec_FlightData);
}

void UnlockFlightData() {
  LeaveCriticalSection(&CritSec_FlightData);
}

void LockTerrainDataCalculations() {
  EnterCriticalSection(&CritSec_TerrainDataCalculations);
}

void UnlockTerrainDataCalculations() {
  LeaveCriticalSection(&CritSec_TerrainDataCalculations);
}

void LockTerrainDataGraphics() {
  EnterCriticalSection(&CritSec_TerrainDataGraphics);
}

void UnlockTerrainDataGraphics() {
  LeaveCriticalSection(&CritSec_TerrainDataGraphics);
}



void HideInfoBoxes() {
  int i;
  InfoBoxesHidden = true;
  #if NEWINFOBOX > 0
  for (i=0; i<numInfoWindows; i++) {
    InfoBoxes[i]->SetVisible(false);
  }
  #else
  for (i=0; i<numInfoWindows; i++) {
    ShowWindow(hWndInfoWindow[i], SW_HIDE);
    ShowWindow(hWndTitleWindow[i], SW_HIDE);
  }
  #endif
}


void ShowInfoBoxes() {
  int i;
  InfoBoxesHidden = false;
  #if NEWINFOBOX > 0
  for (i=0; i<numInfoWindows; i++) {
    InfoBoxes[i]->SetVisible(true);
  }
  #else
  for (i=0; i<numInfoWindows; i++) {
    ShowWindow(hWndInfoWindow[i], SW_SHOW);
    ShowWindow(hWndTitleWindow[i], SW_SHOW);
  }
  #endif
}



/////////////////////


DWORD GetBatteryInfo(BATTERYINFO* pBatteryInfo)
{
    // set default return value
    DWORD result = 0;

    // check incoming pointer
    if(NULL == pBatteryInfo)
    {
        return 0;
    }

#if (WINDOWSPC<1)
    SYSTEM_POWER_STATUS_EX2 sps;

    // request the power status
    result = GetSystemPowerStatusEx2(&sps, sizeof(sps), TRUE);

    // only update the caller if the previous call succeeded
    if(0 != result)
    {
        pBatteryInfo->acStatus = sps.ACLineStatus;
        pBatteryInfo->chargeStatus = sps.BatteryFlag;
        pBatteryInfo->BatteryLifePercent = sps.BatteryLifePercent;
    }
#endif

    return result;
}


//////////////

// GDI Escapes for ExtEscape()
#define QUERYESCSUPPORT    8
// The following are unique to CE
#define GETVFRAMEPHYSICAL   6144
#define GETVFRAMELEN    6145
#define DBGDRIVERSTAT    6146
#define SETPOWERMANAGEMENT   6147
#define GETPOWERMANAGEMENT   6148


typedef enum _VIDEO_POWER_STATE {
    VideoPowerOn = 1,
    VideoPowerStandBy,
    VideoPowerSuspend,
    VideoPowerOff
} VIDEO_POWER_STATE, *PVIDEO_POWER_STATE;


typedef struct _VIDEO_POWER_MANAGEMENT {
    ULONG Length;
    ULONG DPMSVersion;
    ULONG PowerState;
} VIDEO_POWER_MANAGEMENT, *PVIDEO_POWER_MANAGEMENT;


void BlankDisplay(bool doblank) {
  static bool oldblank = false;

#if (WINDOWSPC>0)
  return;
#endif

  if (!EnableAutoBlank) {
    return;
  }
  if (doblank == oldblank) {
    return;
  }

  HDC gdc;
  int iESC=SETPOWERMANAGEMENT;

  gdc = ::GetDC(NULL);
  if (ExtEscape(gdc, QUERYESCSUPPORT, sizeof(int), (LPCSTR)&iESC,
                0, NULL)==0) {
    // can't do it, not supported
  } else {

    VIDEO_POWER_MANAGEMENT vpm;
    vpm.Length = sizeof(VIDEO_POWER_MANAGEMENT);
    vpm.DPMSVersion = 0x0001;

	// TODO - Trigger a GCE (Glide Computer Event) when switching to battery mode
	//	This can be used to warn users that power has been lost and you are now
	//	on battery power - ie: something else is wrong

    if (doblank) {
      BATTERYINFO BatteryInfo;
      GetBatteryInfo(&BatteryInfo);

	  /* Battery status - simulator only - for safety of battery data
		note: Simulator only - more important to keep running in your plane
	  */

#ifdef _SIM_
      // JMW, maybe this should be active always...
      // we don't want the PDA to be completely depleted.

	  if (BatteryInfo.acStatus==0) {
		  if (BatteryInfo.BatteryLifePercent < BATTERY_EXIT) {
		    // TODO - Debugging and warning message
		    exit(0);
		  } else if (BatteryInfo.BatteryLifePercent < BATTERY_WARNING) {
		    DWORD LocalWarningTime = ::GetTickCount();
		    if ((LocalWarningTime - BatteryWarningTime) > BATTERY_REMINDER) {
		      BatteryWarningTime = LocalWarningTime;
		      // TODO - Show the user what the status is.
		      DoStatusMessage(TEXT("Organiser Battery Low"));
		    }
		  }
	  }

#endif

      if (BatteryInfo.acStatus==0) {

        // Power off the display
        vpm.PowerState = VideoPowerOff;
        ExtEscape(gdc, SETPOWERMANAGEMENT, vpm.Length, (LPCSTR) &vpm,
                  0, NULL);
        oldblank = true;
        ScreenBlanked = true;
      } else {
        DisplayTimeOut = 0;
      }
    } else {
      if (oldblank) { // was blanked
        // Power on the display
        vpm.PowerState = VideoPowerOn;
        ExtEscape(gdc, SETPOWERMANAGEMENT, vpm.Length, (LPCSTR) &vpm,
                  0, NULL);
        oldblank = false;
        ScreenBlanked = false;
      }
    }

  }
  ::ReleaseDC(NULL, gdc);
}



void Event_SelectInfoBox(int i) {
  int oldinfofocus = InfoFocus;

  // must do this
  InfoBoxFocusTimeOut = 0;

  if (InfoFocus>= 0) {
    FocusOnWindow(InfoFocus,false);
  }
  InfoFocus+= i;
  if (InfoFocus>=numInfoWindows) {
    InfoFocus = -1; // deactivate if wrap around
  }
  if (InfoFocus<0) {
    InfoFocus = -1; // deactivate if wrap around
  }
  if (InfoFocus<0) {
    DefocusInfoBox();
    SwitchToMapWindow();
    return;
  }

  //  SetFocus(hWndInfoWindow[InfoFocus]);
  FocusOnWindow(InfoFocus,true);
  InfoWindowActive = TRUE;
  DisplayText();

  InputEvents::setMode(TEXT("infobox"));
}


void Event_ChangeInfoBoxType(int i) {
  int j, k;

  if (InfoFocus<0) {
    return;
  }

  k = getInfoType(InfoFocus);
  if (i>0) {
    j = Data_Options[k].next_screen;
  }
  if (i<0) {
    j = Data_Options[k].prev_screen;
  }

  // TODO: if i==0, go to default or reset

  setInfoType(InfoFocus, j);
  DisplayText();

}



///////////////////

/* Memory checking 

void GlobalMemoryStatus( 
  LPMEMORYSTATUS lpBuffer 
);


typedef struct _MEMORYSTATUS { 
  DWORD dwLength; 
  DWORD dwMemoryLoad; 
  DWORD dwTotalPhys; 
  DWORD dwAvailPhys; 
  DWORD dwTotalPageFile; 
  DWORD dwAvailPageFile; 
  DWORD dwTotalVirtual; 
  DWORD dwAvailVirtual; 
} MEMORYSTATUS, *LPMEMORYSTATUS; 
Members
dwLength 
Specifies the size, in bytes, of the MEMORYSTATUS structure. 
Set this member to sizeof(MEMORYSTATUS) when passing it to the GlobalMemoryStatus function. 

dwMemoryLoad 
Specifies a number between zero and 100 that gives a general idea of current memory use, in which zero indicates no memory use and 100 indicates full memory use. 
dwTotalPhys 
Indicates the total number of bytes of physical memory. 
dwAvailPhys 
Indicates the number of bytes of physical memory available. 
dwTotalPageFile 
Indicates the total number of bytes that can be stored in the paging file. 
This number does not represent the physical size of the paging file on disk. 

dwAvailPageFile 
Indicates the number of bytes available in the paging file. 
dwTotalVirtual 
Indicates the total number of bytes that can be described in the user mode portion of the virtual address space of the calling process. 
dwAvailVirtual 
Indicates the number of bytes of unreserved and uncommitted memory in the user mode portion of the virtual address space of the calling process. 
Requirements
OS Versions: Windows CE 1.0 and later.
Header: Winbase.h.

See Also
GlobalMemoryStatus 


/////

Files: 
  CopyFile
  GetFileSize
GetDiskFreeSpaceEx


*/
