/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

  $Id$
}
*/
#include "StdAfx.h"
#include "compatibility.h"
#include "Compatibility/string.h"
#include "XCSoar.h"
#include "MapWindow.h"
#include "Parser.h"
#include "Calculations.h"
#include "Calculations2.h"
#include "Task.h"
#include "Dialogs.h"
#include "Trigger.hpp"

#ifdef OLDPPCx
#include "XCSoarProcess.h"
#else
#include "Process.h"
#endif

#include "Utils.h"
#include "Utils2.h"
#include "Port.h"
#include "Waypointparser.h"
#include "Airspace.h"
#include "Logger.h"
#include "McReady.h"
#include "AirfieldDetails.h"
#include "InfoBoxLayout.h"

#include <commctrl.h>
#if (WINDOWSPC<1)
#include <sipapi.h>
#if !defined(CECORE) || UNDER_CE >= 300 || _WIN32_WCE >= 0x0300
#include <aygshell.h>
#endif
#endif

#include "Terrain.h"
#include "VarioSound.h"
#include "device.h"

#include "externs.h"
#include "Units.h"
#include "InputEvents.h"
#include "Message.h"
#include "Atmosphere.h"
#include "Geoid.h"

#include "InfoBox.h"
#include "RasterTerrain.h"

#ifdef DEBUG_TRANSLATIONS
#include <map>
static std::map<TCHAR*, TCHAR*> unusedTranslations;
#endif

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
  false,
  apIbBox,
  false,
  true,
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
  tiHighScore,
  false,
  false,
  false,
  false,
  false,
  gvnsLongNeedle,
  true,
  apIbBox,
#if defined(PNA) || defined(FIVV)  // VENTA-ADDON Model type
  apIg0,  // VENTA-ADDON GEOM
  apImPnaGeneric,
#endif
  false,
  true,
  false
};

#endif


TCHAR XCSoar_Version[256] = TEXT("");

static bool ForceShutdown = false;

HINSTANCE hInst; // The current instance
//HWND hWndCB; // The command bar handle
HWND hWndMainWindow; // Main Windows
HWND hWndMapWindow;  // MapWindow

int numInfoWindows = 8;



InfoBox *InfoBoxes[MAXINFOWINDOWS];

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

bool RequestAirspaceWarningDialog= false;
bool RequestAirspaceWarningForce=false;
static bool DisplayLocked = true;
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

// Display Gobals
HFONT                                   InfoWindowFont;
HFONT                                   TitleWindowFont;
HFONT                                   MapWindowFont;
HFONT                                   TitleSmallWindowFont;
HFONT                                   MapWindowBoldFont;
HFONT                                   CDIWindowFont; // New
HFONT                                   MapLabelFont;
HFONT                                   StatisticsFont;


LOGFONT                                   autoInfoWindowLogFont; // these are the non-custom parameters
LOGFONT                                   autoTitleWindowLogFont;
LOGFONT                                   autoMapWindowLogFont;
LOGFONT                                   autoTitleSmallWindowLogFont;
LOGFONT                                   autoMapWindowBoldLogFont;
LOGFONT                                   autoCDIWindowLogFont; // New
LOGFONT                                   autoMapLabelLogFont;
LOGFONT                                   autoStatisticsLogFont;

int  UseCustomFonts;
int                                             CurrentInfoType;
int                                             InfoFocus = 0;
int                                             DisplayOrientation = TRACKUP;
int                                             DisplayTextType = DISPLAYNONE;

int                                             AltitudeMode = ALLON;
int                                             ClipAltitude = 1000;
int                                             AltWarningMargin = 100;
int                                             AutoAdvance = 1;
bool                                            AdvanceArmed = false;


bool EnableBlockSTF = false;

short   ScreenSize=0;

bool GlobalRunning = false;

#if defined(PNA) || defined(FIVV)  // VENTA-ADDON we call it model and not PNA for possible future usage even for custom PDAs
int	GlobalModelType=0;	// see XCSoar.h for modeltype definitions
TCHAR	GlobalModelName[MAX_PATH]; // there are currently no checks.. TODO check it fits here
float	GlobalEllipse=1.1f;	// default ellipse type VENTA2-ADDON
#endif


// this controls all displays, to make sure everything is
// properly initialised.


//SI to Local Units
double        SPEEDMODIFY = TOKNOTS;
double        LIFTMODIFY  = TOKNOTS;
double        DISTANCEMODIFY = TONAUTICALMILES;
double        ALTITUDEMODIFY = TOFEET;
double        TASKSPEEDMODIFY = TOKPH;

//Flight Data Globals
double        MACCREADY = 0; // JMW now in SI units (m/s) for consistency
double        QNH = (double)1013.2;
double        BUGS = 1;
double        BALLAST = 0;

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
static int iTimerID= 0;

// Final Glide Data
double SAFETYALTITUDEARRIVAL = 500;
double SAFETYALTITUDEBREAKOFF = 700;
double SAFETYALTITUDETERRAIN = 200;
double SAFTEYSPEED = 50.0;

// polar info
int              POLARID = 0;
double POLAR[POLARSIZE] = {0,0,0};
double POLARV[POLARSIZE] = {21,27,40};
double POLARLD[POLARSIZE] = {33,30,20};
double WEIGHTS[POLARSIZE] = {250,70,100};

// Team code info
int TeamCodeRefWaypoint = -1;
TCHAR TeammateCode[10];
bool TeamFlarmTracking = false;
TCHAR TeamFlarmCNTarget[4]; // CN of the glider to track
int TeamFlarmIdTarget;      // FlarmId of the glider to track
double TeammateLatitude;
double TeammateLongitude;
bool TeammateCodeValid = false;


// Waypoint Database
WAYPOINT *WayPointList = NULL;
WPCALC *WayPointCalc = NULL; // VENTA3 additional infos calculated, parallel to WPs
unsigned int NumberOfWayPoints = 0;
int SectorType = 1; // FAI sector
DWORD SectorRadius = 500;
int StartLine = TRUE;
DWORD StartRadius = 3000;

int HomeWaypoint = -1;
int AirfieldsHomeWaypoint = -1; // VENTA3 force Airfields home to be HomeWaypoint if
                                // an H flag in waypoints file is not available..
// Alternates
int Alternate1 = -1; // VENTA3
int Alternate2 = -1; // VENTA3
int BestAlternate = -1; // VENTA3
int ActiveAlternate = -1; // VENTA3
bool OnBestAlternate=false;
bool OnAlternate1=false;
bool OnAlternate2=false;

// Specials
#ifdef FIVV
double GPSAltitudeOffset = 0; // VENTA3
#endif
double QFEAltitudeOffset = 0;
int OnAirSpace=1; // VENTA3 toggle DrawAirSpace, normal behaviour is "true"
bool WasFlying = false; // VENTA3 used by auto QFE: do not reset QFE if previously in flight. So you can check QFE
			//   on the ground, otherwise it turns to zero at once!
double LastFlipBoxTime = 0; // VENTA3 need this global for slowcalculations cycle
#if defined(PNA) || defined(FIVV)
bool needclipping=false; // flag to activate extra clipping for some PNAs
#endif
bool EnableAutoBacklight=true;
bool EnableAutoSoundVolume=true;
bool ExtendedVisualGlide=false;
bool VirtualKeys=false;
short AverEffTime=0;

ldrotary_s rotaryLD;

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
int FinalGlideTerrain = 0;
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

BOOL COMPORTCHANGED = FALSE;
BOOL MAPFILECHANGED = FALSE;
BOOL AIRSPACEFILECHANGED = FALSE;
BOOL AIRFIELDFILECHANGED = FALSE;
BOOL WAYPOINTFILECHANGED = FALSE;
BOOL TERRAINFILECHANGED = FALSE;
BOOL TOPOLOGYFILECHANGED = FALSE;
BOOL POLARFILECHANGED = FALSE;
BOOL LANGUAGEFILECHANGED = FALSE;
BOOL STATUSFILECHANGED = FALSE;
BOOL INPUTFILECHANGED = FALSE;
static bool MenuActive = false;

//Task Information
Task_t Task = {{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0}};
Start_t StartPoints;
TaskStats_t TaskStats;
int ActiveWayPoint = -1;

// Assigned Area Task
double AATTaskLength = 120;
BOOL AATEnabled = FALSE;
DWORD FinishMinHeight = 0;
DWORD StartMaxHeight = 0;
DWORD StartMaxSpeed = 0;
DWORD StartMaxHeightMargin = 0;
DWORD StartMaxSpeedMargin = 0;


// Statistics
Statistics flightstats;

#if (((UNDER_CE >= 300)||(_WIN32_WCE >= 0x0300)) && (WINDOWSPC<1))
#define HAVE_ACTIVATE_INFO
static SHACTIVATEINFO s_sai;
#endif

static BOOL InfoBoxesHidden = false;

void PopupBugsBallast(int updown);

#include "GaugeCDI.h"
#include "GaugeFLARM.h"
#include "GaugeVario.h"

// Battery status for SIMULATOR mode
//	30% reminder, 20% exit, 30 second reminders on warnings

#ifndef GNAV
#define BATTERY_WARNING 30
#define BATTERY_EXIT 20
#define BATTERY_REMINDER 30000
DWORD BatteryWarningTime = 0;
#endif

// Groups:
//   Altitude 0,1,20,33
//   Aircraft info 3,6,23,32,37,47,54
//   LD 4,5,19,38,53, 66    VENTA-ADDON added 66 for GR final
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
	  {ugVerticalSpeed,   TEXT("Thermal last 30 sec"), TEXT("TC 30s"), new FormatterLowWarning(TEXT("%-2.1f"),0.0), NoProcessing, 7, 44},
	  // 3
#ifdef FIVV
	  {ugNone,            TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), NoProcessing, 6, 54},
#else
	  {ugNone,            TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), NoProcessing, 6, 54},
#endif
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
	  {ugTaskSpeed, TEXT("Speed Task Average"), TEXT("V Task Av"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
	  // 18
	  {ugDistance,        TEXT("Final Distance"), TEXT("Fin Dis"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 27, 17},
	  // 19
	  {ugNone,            TEXT("Final LD"), TEXT("Fin LD"), new InfoBoxFormatter(TEXT("%1.0f")), NoProcessing, 38, 5},
	  // 20
	  {ugAltitude,        TEXT("Terrain Elevation"), TEXT("H Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 33, 1},
	  // 21
	  {ugVerticalSpeed,   TEXT("Thermal Average"), TEXT("TC Avg"), new FormatterLowWarning(TEXT("%-2.1f"),0.0), NoProcessing, 22, 9},
	  // 22
	  {ugAltitude,        TEXT("Thermal Gain"), TEXT("TC Gain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 24, 21},
	  // 23
#ifdef FIVV
	  {ugNone,            TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), DirectionProcessing, 32, 6},
#else
	  {ugNone,            TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), DirectionProcessing, 32, 6},
#endif
	  // 24
	  {ugVerticalSpeed,   TEXT("Vario"), TEXT("Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 44, 22},
	  // 25
	  {ugWindSpeed,       TEXT("Wind Speed"), TEXT("Wind V"), new InfoBoxFormatter(TEXT("%2.0f")), WindSpeedProcessing, 26, 50},
	  // 26
#ifdef FIVV
	  {ugNone,            TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), WindDirectionProcessing, 48, 25},
#else
	  {ugNone,            TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), WindDirectionProcessing, 48, 25},
#endif
	  // 27
	  {ugNone,            TEXT("AA Time"), TEXT("AA Time"), new FormatterAATTime(TEXT("%2.0f")), NoProcessing, 28, 18},
	  // 28
	  {ugDistance,        TEXT("AA Distance Max"), TEXT("AA Dmax"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 29, 27},
	  // 29
	  {ugDistance,        TEXT("AA Distance Min"), TEXT("AA Dmin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 30, 28},
	  // 30
	  {ugTaskSpeed, TEXT("AA Speed Max"), TEXT("AA Vmax"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 31, 29},
	  // 31
	  {ugTaskSpeed, TEXT("AA Speed Min"), TEXT("AA Vmin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 51, 30},
	  // 32
	  {ugHorizontalSpeed, TEXT("Airspeed IAS"), TEXT("V IAS"), new InfoBoxFormatter(TEXT("%2.0f")), AirspeedProcessing, 37, 23},
	  // 33
	  {ugAltitude,        TEXT("Pressure Altitude"), TEXT("H Baro"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 0, 20},
	  // 34
	  {ugHorizontalSpeed, TEXT("Speed MacReady"), TEXT("V Mc"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 35, 10},
	  // 35
	  {ugNone,            TEXT("Percentage climb"), TEXT("% Climb"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 43, 34},
	  // 36
	  {ugNone,            TEXT("Time of flight"), TEXT("Time flt"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 39, 14},
	  // 37
	  {ugNone,            TEXT("G load"), TEXT("G"), new InfoBoxFormatter(TEXT("%2.2f")), AccelerometerProcessing, 47, 32},
	  // 38
	  {ugNone,            TEXT("Next LD"), TEXT("WP LD"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 53, 19},
	  // 39
	  {ugNone,            TEXT("Time local"), TEXT("Time loc"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 40, 36},
	  // 40
	  {ugNone,            TEXT("Time UTC"), TEXT("Time UTC"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 41, 39},
	  // 41
	  {ugNone,            TEXT("Task Time To Go"), TEXT("Fin ETE"), new FormatterAATTime(TEXT("%04.0f")), NoProcessing, 42, 40},
	  // 42
	  {ugNone,            TEXT("Next Time To Go"), TEXT("WP ETE"), new FormatterAATTime(TEXT("%04.0f")), NoProcessing, 45, 41},
	  // 43
	  {ugHorizontalSpeed, TEXT("Speed Dolphin"), TEXT("V Opt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 10, 35},
	  // 44
	  {ugVerticalSpeed,   TEXT("Netto Vario"), TEXT("Netto"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 2, 24},
	  // 45
	  {ugNone,            TEXT("Task Arrival Time"), TEXT("Fin ETA"), new FormatterAATTime(TEXT("%04.0f")), NoProcessing, 46, 42},
	  // 46
	  {ugNone,            TEXT("Next Arrival Time"), TEXT("WP ETA"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 14, 45},
	  // 47
	  {ugNone,            TEXT("Bearing Difference"), TEXT("Brng D"), new FormatterDiffBearing(TEXT("")), NoProcessing, 54, 37},
	  // 48
	  {ugNone,            TEXT("Outside Air Temperature"), TEXT("OAT"), new InfoBoxFormatter(TEXT("%2.1f")TEXT(DEG)), NoProcessing, 49, 26},
	  // 49
	  {ugNone,            TEXT("Relative Humidity"), TEXT("RelHum"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 50, 48},
	  // 50
	  {ugNone,            TEXT("Forecast Temperature"), TEXT("MaxTemp"), new InfoBoxFormatter(TEXT("%2.1f")TEXT(DEG)), ForecastTemperatureProcessing, 49, 25},
	  // 51
	  {ugDistance,        TEXT("AA Distance Tgt"), TEXT("AA Dtgt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 52, 31},
	  // 52
	  {ugTaskSpeed, TEXT("AA Speed Tgt"), TEXT("AA Vtgt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 11, 51},
	  // 53
	  {ugNone,            TEXT("L/D vario"), TEXT("L/D vario"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 4, 38},
	  // 54
	  {ugHorizontalSpeed, TEXT("Airspeed TAS"), TEXT("V TAS"), new InfoBoxFormatter(TEXT("%2.0f")), AirspeedProcessing, 3, 47},
	  // 55
	  {ugNone,            TEXT("Own Team Code"), TEXT("TeamCode"), new FormatterTeamCode(TEXT("\0")), TeamCodeProcessing, 56, 54},
	  // 56
#ifdef FIVV
	  {ugNone,            TEXT("Team Bearing"), TEXT("Tm Brng"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), NoProcessing, 57, 55},
#else
	  {ugNone,            TEXT("Team Bearing"), TEXT("Tm Brng"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), NoProcessing, 57, 55},
#endif
	  // 57
	  {ugNone,            TEXT("Team Bearing Diff"), TEXT("Team Bd"), new FormatterDiffTeamBearing(TEXT("")), NoProcessing, 58, 56},
	  // 58
	  {ugNone,            TEXT("Team Range"), TEXT("Team Dis"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 55, 57},
          // 59
	  {ugTaskSpeed, TEXT("Speed Task Instantaneous"), TEXT("V Tsk Ins"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
          // 60
	  {ugDistance, TEXT("Distance Home"), TEXT("Home Dis"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
	  // 61
	  {ugTaskSpeed, TEXT("Speed Task Achieved"), TEXT("V Tsk Ach"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
          // 62
	  {ugNone,            TEXT("AA Delta Time"), TEXT("AA dT"), new FormatterAATTime(TEXT("%2.0f")), NoProcessing, 28, 18},
          // 63
	  {ugVerticalSpeed,   TEXT("Thermal All"), TEXT("TC All"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
          // 64
	  {ugVerticalSpeed,   TEXT("Distance Vario"), TEXT("D Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
	  // 65
#ifndef GNAV
	  {ugNone,            TEXT("Battery Percent"), TEXT("Battery"), new InfoBoxFormatter(TEXT("%2.0f%%")), NoProcessing, 49, 26},
#else
	  {ugNone,            TEXT("Battery Voltage"), TEXT("Battery"), new InfoBoxFormatter(TEXT("%2.1fV")), NoProcessing, 49, 26},
#endif
	  // 66  VENTA-ADDON added Final GR
	  // VENTA-TODO: fix those 38,5 numbers to point correctly menu items
	  {ugNone,            TEXT("Final GR"), TEXT("Fin GR"), new InfoBoxFormatter(TEXT("%1.1f")), NoProcessing, 38, 5},

	  // 67 VENTA3-ADDON Alternate1 destinations infoboxes  TODO> fix 36 46 to something correct
	  {ugNone,            TEXT("Alternate1 GR"), TEXT("Altern 1"), new FormatterAlternate(TEXT("\0")), Alternate1Processing, 36, 46},
	  // 68 Alternate 2
	  {ugNone,            TEXT("Alternate2 GR"), TEXT("Altern 2"), new FormatterAlternate(TEXT("\0")), Alternate2Processing, 36, 46},
	  // 69 BestAlternate aka BestLanding
	  {ugNone,            TEXT("Best Alternate"), TEXT("BestAltr"), new FormatterAlternate(TEXT("\0")), BestAlternateProcessing, 36, 46},
          // 70
	  {ugAltitude,        TEXT("QFE GPS"), TEXT("QFE GPS"), new InfoBoxFormatter(TEXT("%2.0f")), QFEAltitudeProcessing, 1, 33},
          // 71 TODO FIX those 19,4 values
	  {ugNone,            TEXT("L/D Average"), TEXT("L/D Avg"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 19, 4},
	  // 72 //
	  {ugNone,   TEXT("Experimental1"), TEXT("Exp1"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
	  // 73 //
	  {ugNone,   TEXT("Experimental2"), TEXT("Exp2"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
	};

const int NUMSELECTSTRINGS = 74;


CRITICAL_SECTION  CritSec_FlightData;
bool csFlightDataInitialized = false;
CRITICAL_SECTION  CritSec_EventQueue;
bool csEventQueueInitialized = false;
CRITICAL_SECTION  CritSec_TerrainDataGraphics;
bool csTerrainDataGraphicsInitialized = false;
CRITICAL_SECTION  CritSec_TerrainDataCalculations;
bool csTerrainDataCalculationsInitialized = false;
CRITICAL_SECTION  CritSec_NavBox;
bool csNavBoxInitialized = false;
CRITICAL_SECTION  CritSec_Comm;
bool csCommInitialized = false;
CRITICAL_SECTION  CritSec_TaskData;
bool csTaskDataInitialized = false;


static BOOL GpsUpdated;
static Trigger dataTriggerEvent(TEXT("dataTriggerEvent"));

static BOOL VarioUpdated;
static Trigger varioTriggerEvent(TEXT("varioTriggerEvent"));


// Forward declarations of functions included in this code module:
ATOM                                                    MyRegisterClass (HINSTANCE, LPTSTR);
BOOL                                                    InitInstance    (HINSTANCE, int);
LRESULT CALLBACK        WndProc                 (HWND, UINT, WPARAM, LPARAM);
LRESULT                                         MainMenu(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void                                                    AssignValues(void);
void                                                    DisplayText(void);

void CommonProcessTimer    (void);
#ifdef _SIM_
void SIMProcessTimer(void);
#else
void ProcessTimer    (void);
#endif
void                                                    PopUpSelect(int i);

//HWND CreateRpCommandBar(HWND hwnd);


void TriggerGPSUpdate()
{
  GpsUpdated = true;
  dataTriggerEvent.trigger();
}

void TriggerVarioUpdate()
{
  VarioUpdated = true;
  varioTriggerEvent.pulse();
}

void HideMenu() {
  // ignore this if the display isn't locked -- must keep menu visible
  if (DisplayLocked) {
    MenuTimeOut = MenuTimeoutMax;
    DisplayTimeOut = 0;
  }
}

void ShowMenu() {
#if !defined(GNAV) && !defined(PCGNAV)
  // Popup exit button if in .xci
  //InputEvents::setMode(TEXT("Exit"));
  InputEvents::setMode(TEXT("Menu")); // VENTA3
#endif
  MenuTimeOut = 0;
  DisplayTimeOut = 0;
}


#if (EXPERIMENTAL > 0)
BlueDialupSMS bsms;
#endif

void SettingsEnter() {
  MenuActive = true;

  MapWindow::SuspendDrawingThread();
  // This prevents the map and calculation threads from doing anything
  // with shared data while it is being changed.

  MAPFILECHANGED = FALSE;
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
  if (!GlobalRunning) return;

  SwitchToMapWindow();

  // Locking everything here prevents the calculation thread from running,
  // while shared data is potentially reloaded.

  LockFlightData();
  LockTaskData();
  LockNavBox();

  MenuActive = false;

  if(MAPFILECHANGED) {
    AIRSPACEFILECHANGED = TRUE;
    AIRFIELDFILECHANGED = TRUE;
    WAYPOINTFILECHANGED = TRUE;
    TERRAINFILECHANGED = TRUE;
    TOPOLOGYFILECHANGED = TRUE;
  }

  if((WAYPOINTFILECHANGED) || (TERRAINFILECHANGED) || (AIRFIELDFILECHANGED))
    {
      ClearTask();

      // re-load terrain
      RasterTerrain::CloseTerrain();
      RasterTerrain::OpenTerrain();

      // re-load waypoints
      ReadWayPoints();
      InitWayPointCalc(); // VENTA3
      ReadAirfieldFile();

      // re-set home
      if (WAYPOINTFILECHANGED || TERRAINFILECHANGED) {
	SetHome(WAYPOINTFILECHANGED==TRUE);
      }

      //
      RasterTerrain::ServiceFullReload(GPS_INFO.Latitude,
                                       GPS_INFO.Longitude);

      MapWindow::ForceVisibilityScan = true;
    }

  if (TOPOLOGYFILECHANGED)
    {
      CloseTopology();
      OpenTopology();
      MapWindow::ForceVisibilityScan = true;
    }

  if(AIRSPACEFILECHANGED)
    {
      CloseAirspace();
      ReadAirspace();
      SortAirspace();
      MapWindow::ForceVisibilityScan = true;
    }

  if (POLARFILECHANGED) {
    CalculateNewPolarCoef();
    GlidePolar::SetBallast();
  }

  if (AIRFIELDFILECHANGED
      || AIRSPACEFILECHANGED
      || WAYPOINTFILECHANGED
      || TERRAINFILECHANGED
      || TOPOLOGYFILECHANGED
      ) {
    CloseProgressDialog();
    SetFocus(hWndMapWindow);
  }

  UnlockNavBox();
  UnlockTaskData();
  UnlockFlightData();

#ifndef _SIM_
  if(COMPORTCHANGED)
    {
      // JMW disabled com opening in sim mode
      RestartCommPorts();
    }

#endif

  MapWindow::ResumeDrawingThread();
  // allow map and calculations threads to continue on their merry way
}


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



void FullScreen() {

  if (!MenuActive) {
    SetForegroundWindow(hWndMainWindow);
#if (WINDOWSPC>0)
    SetWindowPos(hWndMainWindow,HWND_TOP,
                 0, 0, 0, 0,
                 SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
#else
#ifndef CECORE
    SHFullScreen(hWndMainWindow,
                 SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif
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
#ifdef HAVEEXCEPTIONS
  if (!csCommInitialized) throw TEXT("LockComm Error");
#endif
  EnterCriticalSection(&CritSec_Comm);
}

void UnlockComm() {
#ifdef HAVEEXCEPTIONS
  if (!csCommInitialized) throw TEXT("LockComm Error");
#endif
  LeaveCriticalSection(&CritSec_Comm);
}


void RestartCommPorts() {
  static bool first = true;
  /*
#if (WINDOWSPC>0)
  if (!first) {
    NMEAParser::Reset();
    return;
  }
#endif
  */
  StartupStore(TEXT("RestartCommPorts\n"));

  LockComm();

  devClose(devA());
  devClose(devB());

  NMEAParser::Reset();

  first = false;

  devInit(TEXT(""));

  UnlockComm();

}



void DefocusInfoBox() {
  FocusOnWindow(InfoFocus,false);
  InfoFocus = -1;
  if (MapWindow::isPan() && !MapWindow::isTargetPan()) {
    InputEvents::setMode(TEXT("pan"));
  } else {
    InputEvents::setMode(TEXT("default"));
  }
  InfoWindowActive = FALSE;
}


void FocusOnWindow(int i, bool selected) {
    //hWndTitleWindow

  if (i<0) return; // error

  InfoBoxes[i]->SetFocus(selected);
  // todo defocus all other?

}


void TriggerRedraws(NMEA_INFO *nmea_info,
		    DERIVED_INFO *derived_info) {
	(void)nmea_info;
	(void)derived_info;
  if (MapWindow::IsDisplayRunning()) {
    if (GpsUpdated) {
      MapWindow::MapDirty = true;
      drawTriggerEvent.pulse();
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
	(void)lpvoid;
  // wait for proper startup signal
  while (!MapWindow::IsDisplayRunning()) {
    Sleep(100);
  }

  while (!MapWindow::CLOSETHREAD) {

    varioTriggerEvent.wait(5000);
    if (MapWindow::CLOSETHREAD) break; // drop out on exit

    if (VarioUpdated) {
      VarioUpdated = false;
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
	(void)lpvoid;
  bool needcalculationsslow;

  NMEA_INFO     tmp_GPS_INFO;
  DERIVED_INFO  tmp_CALCULATED_INFO;

  needcalculationsslow = false;

  // wait for proper startup signal
  while (!MapWindow::IsDisplayRunning()) {
    Sleep(100);
  }

  while (!MapWindow::CLOSETHREAD) {

    dataTriggerEvent.wait(5000);
    if (MapWindow::CLOSETHREAD) break; // drop out on exit

    // set timer to determine latency (including calculations)
    if (GpsUpdated) {
      //      MapWindow::UpdateTimeStats(true);
    }

    // make local copy before editing...
    LockFlightData();
    if (GpsUpdated) { // timeout on FLARM objects
      FLARM_RefreshSlots(&GPS_INFO);
    }
    memcpy(&tmp_GPS_INFO,&GPS_INFO,sizeof(NMEA_INFO));
    memcpy(&tmp_CALCULATED_INFO,&CALCULATED_INFO,sizeof(DERIVED_INFO));

    UnlockFlightData();

    // Do vario first to reduce audio latency
    if (GPS_INFO.VarioAvailable) {
      // if (VarioUpdated) {  20060511/sgi commented out dueto asynchronus reset of VarioUpdate in InstrumentThread
      if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {

      }
      // assume new vario data has arrived, so infoboxes
      // need to be redrawn
      //} 20060511/sgi commented out
    } else {
      // run the function anyway, because this gives audio functions
      // if no vario connected
      if (GpsUpdated) {
	if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {
	}
	TriggerVarioUpdate(); // emulate vario update
      }
    }

    if (GpsUpdated) {
      if(DoCalculations(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)){

        DisplayMode_t lastDisplayMode = DisplayMode;

        MapWindow::MapDirty = true;
        needcalculationsslow = true;

        switch (UserForceDisplayMode){
          case dmCircling:
            DisplayMode = dmCircling;
          break;
          case dmCruise:
            DisplayMode = dmCruise;
          break;
          case dmFinalGlide:
            DisplayMode = dmFinalGlide;
          break;
          case dmNone:
            if (tmp_CALCULATED_INFO.Circling){
              DisplayMode = dmCircling;
            } else if (tmp_CALCULATED_INFO.FinalGlide){
              DisplayMode = dmFinalGlide;
            } else
              DisplayMode = dmCruise;
          break;
        }

        if (lastDisplayMode != DisplayMode){
          MapWindow::SwitchZoomClimb();
        }

      }
      InfoBoxesDirty = true;
    }

    if (MapWindow::CLOSETHREAD) break; // drop out on exit

    TriggerRedraws(&tmp_GPS_INFO, &tmp_CALCULATED_INFO);

    if (MapWindow::CLOSETHREAD) break; // drop out on exit

#if defined(_SIM_)
    if (needcalculationsslow || ( (OnBestAlternate == true) && (ReplayLogger::IsEnabled()) )) { // VENTA3, needed for BestAlternate SIM
#else
    if (needcalculationsslow) {
#endif
      DoCalculationsSlow(&tmp_GPS_INFO,&tmp_CALCULATED_INFO);
      needcalculationsslow = false;
    }

    if (MapWindow::CLOSETHREAD) break; // drop out on exit

    // values changed, so copy them back now: ONLY CALCULATED INFO
    // should be changed in DoCalculations, so we only need to write
    // that one back (otherwise we may write over new data)
    LockFlightData();
    memcpy(&CALCULATED_INFO,&tmp_CALCULATED_INFO,sizeof(DERIVED_INFO));
    UnlockFlightData();

    GpsUpdated = false;

  }
  return 0;
}


void CreateCalculationThread() {
  HANDLE hCalculationThread;
  DWORD dwCalcThreadID;

  // Create a read thread for performing calculations
  if ((hCalculationThread =
      CreateThread (NULL, 0,
        (LPTHREAD_START_ROUTINE )CalculationThread,
         0, 0, &dwCalcThreadID)) != NULL)
  {
    SetThreadPriority(hCalculationThread, THREAD_PRIORITY_NORMAL);
    CloseHandle (hCalculationThread);
  } else {
    ASSERT(1);
  }

  HANDLE hInstrumentThread;
  DWORD dwInstThreadID;

  if ((hInstrumentThread =
      CreateThread (NULL, 0,
       (LPTHREAD_START_ROUTINE )InstrumentThread,
        0, 0, &dwInstThreadID)) != NULL)
  {
    SetThreadPriority(hInstrumentThread, THREAD_PRIORITY_NORMAL);
    CloseHandle (hInstrumentThread);
  } else {
    ASSERT(1);
  }

}


void PreloadInitialisation(bool ask) {
  SetToRegistry(TEXT("XCV"), 1);

#ifdef DEBUG_TRANSLATIONS
  ReadLanguageFile();
#endif

  // Registery (early)

  if (ask) {
    RestoreRegistry();
    ReadRegistrySettings();
    StatusFileInit();

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
    ReadStatusFile();
    InputEvents::readFile();
  }

}

Trigger drawTriggerEvent(TEXT("drawTriggerEvent"));


StartupState_t ProgramStarted = psInitInProgress;
// 0: not started at all
// 1: everything is alive
// 2: done first draw
// 3: normal operation

void AfterStartup() {

  StartupStore(TEXT("CloseProgressDialog\n"));
  CloseProgressDialog();

  // NOTE: Must show errors AFTER all windows ready
  int olddelay = StatusMessageData[0].delay_ms;
  StatusMessageData[0].delay_ms = 20000; // 20 seconds

#ifdef _SIM_
  StartupStore(TEXT("GCE_STARTUP_SIMULATOR\n"));
  InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
#else
  StartupStore(TEXT("GCE_STARTUP_REAL\n"));
  InputEvents::processGlideComputer(GCE_STARTUP_REAL);
#endif
  StatusMessageData[0].delay_ms = olddelay;

#ifdef _INPUTDEBUG_
  InputEvents::showErrors();
#endif

  // Create default task if none exists
  StartupStore(TEXT("Create default task\n"));
  DefaultTask();

  // Trigger first redraw
  GpsUpdated = true;
  MapWindow::MapDirty = true;
  FullScreen();
  drawTriggerEvent.trigger();
}


extern int testmain();

void StartupLogFreeRamAndStorage() {
  int freeram = CheckFreeRam()/1024;
  TCHAR buffer[MAX_PATH];
  LocalPath(buffer);
  int freestorage = FindFreeSpace(buffer);
  StartupStore(TEXT("Free ram %d\nFree storage %d\n"), freeram, freestorage);
}


int WINAPI WinMain(     HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPTSTR    lpCmdLine,
                        int       nCmdShow)
{
  MSG msg;
  HACCEL hAccelTable;
  (void)hPrevInstance;

#ifdef GNAV
#ifdef FORCEPORTRAIT
  // JMW testing only for portrait mode of Altair
  RotateScreen();
#endif
#endif

  // Version String
#ifdef GNAV
  _tcscat(XCSoar_Version, TEXT("Altair "));
#elif PNA  // VENTA-ADDON
  _tcscat(XCSoar_Version, TEXT("PNA "));
#else
#if (WINDOWSPC>0)
  _tcscat(XCSoar_Version, TEXT("PC "));
#else
  _tcscat(XCSoar_Version, TEXT("PPC "));
  // TODO code: consider adding PPC, 2002, 2003 (not enough room now)
#endif
#endif

  // experimental CVS

#ifdef FIVV
  _tcscat(XCSoar_Version, TEXT("5.2.4F "));
#elif defined(__MINGW32__)
  _tcscat(XCSoar_Version, TEXT("5.2.4 "));
#else
  _tcscat(XCSoar_Version, TEXT("5.2.4 "));
#endif

  _tcscat(XCSoar_Version, TEXT(__DATE__));

// VENTA2- delete registries at startup, but not on PC!
#if defined(FIVV) && ( !defined(WINDOWSPC) || WINDOWSPC==0 )
#ifndef PNA
RegDeleteKey(HKEY_CURRENT_USER, _T(REGKEYNAME));
#endif
#endif

#ifdef PNA // VENTA2-ADDON MODEL TYPE
/*
  LocalPath is called for the very first time by CreateDirectoryIfAbsent.
  In order to be able in the future to behave differently for each PNA device
  and maybe also for common PDAs, we need to know the PNA/PDA Model Type
  BEFORE calling LocalPath. This was critical.
*/

  SmartGlobalModelType(); // First we check the exec filename, which
			  // has priority over registry values

  if (!_tcscmp(GlobalModelName, _T("UNKNOWN"))) // Then if there is no smart name...
    SetModelType();                         // get the modeltype from
					    // the registry as usual
#endif

// VENTA2-ADDON install fonts on PDAs and check XCSoarData existance
#if defined(FIVV) && ( !defined(WINDOWSPC) || WINDOWSPC==0 )
//#ifndef PNA

bool datadir=CheckDataDir(); // VENTA3 changed to bool
if (datadir) StartupStore(TEXT("XCSoarData directory found.\n"));
else StartupStore(TEXT("ERROR: NO XCSOARDATA DIRECTORY FOUND!\n"));

StartupStore(TEXT("Check for installing fonts\n"));
short didfonts=InstallFonts();  // check if really did it, and maybe restart
TCHAR nTmp[100];
_stprintf(nTmp,TEXT("InstallFonts() result=%d (0=installed >0 not installed)\n"), didfonts);
StartupStore(nTmp);

//#endif
#endif


// VENTA2- TODO fix these directories are not used always!
  CreateDirectoryIfAbsent(TEXT(""));  // RLD make sure the LocalPath folder actually exists
  CreateDirectoryIfAbsent(TEXT("persist"));
  CreateDirectoryIfAbsent(TEXT("logs")); // VENTA3 we use this one for logging
  CreateDirectoryIfAbsent(TEXT("config"));

  StartupStore(TEXT("Starting XCSoar %s\n"), XCSoar_Version);

  //
  StartupLogFreeRamAndStorage();

  XCSoarGetOpts(lpCmdLine);

  InitCommonControls();
  InitSineTable();

  StartupStore(TEXT("Initialise application instance\n"));

  // Perform application initialization:
  if (!InitInstance (hInstance, nCmdShow))
    {
      return FALSE;
    }

  hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_XCSOAR);

#ifdef HAVE_ACTIVATE_INFO
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
#endif

  StartupStore(TEXT("Initialising critical sections and events\n"));

  InitializeCriticalSection(&CritSec_EventQueue);
  csEventQueueInitialized = true;
  InitializeCriticalSection(&CritSec_TaskData);
  csTaskDataInitialized = true;
  InitializeCriticalSection(&CritSec_FlightData);
  csFlightDataInitialized = true;
  InitializeCriticalSection(&CritSec_NavBox);
  csNavBoxInitialized = true;
  InitializeCriticalSection(&CritSec_Comm);
  csCommInitialized = true;
  InitializeCriticalSection(&CritSec_TerrainDataGraphics);
  csTerrainDataGraphicsInitialized = true;
  InitializeCriticalSection(&CritSec_TerrainDataCalculations);
  csTerrainDataCalculationsInitialized = true;

  // Initialise main blackboard data

  memset( &(Task), 0, sizeof(Task_t));
  memset( &(StartPoints), 0, sizeof(Start_t));
  ClearTask();
  memset( &(GPS_INFO), 0, sizeof(GPS_INFO));
  memset( &(CALCULATED_INFO), 0,sizeof(CALCULATED_INFO));
  memset( &SnailTrail[0],0,TRAILSIZE*sizeof(SNAIL_POINT));

  InitCalculations(&GPS_INFO,&CALCULATED_INFO);

  LinkGRecordDLL(); // try to link DLL if it exists

  OpenGeoid();

  PreloadInitialisation(false);

  GaugeCDI::Create();
  GaugeVario::Create();

  GPS_INFO.NAVWarning = true; // default, no gps at all!

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

  SYSTEMTIME pda_time;
  GetSystemTime(&pda_time);
  GPS_INFO.Time  = pda_time.wHour*3600+pda_time.wMinute*60+pda_time.wSecond;
  GPS_INFO.Year  = pda_time.wYear;
  GPS_INFO.Month = pda_time.wMonth;
  GPS_INFO.Day	 = pda_time.wDay;
  GPS_INFO.Hour  = pda_time.wHour;
  GPS_INFO.Minute = pda_time.wMinute;
  GPS_INFO.Second = pda_time.wSecond;

#ifdef _SIM_
  #if _SIM_STARTUPSPEED
  GPS_INFO.Speed = _SIM_STARTUPSPEED;
  #endif
  #if _SIM_STARTUPALTITUDE
  GPS_INFO.Altitude = _SIM_STARTUPALTITUDE;
  #endif
#endif

#ifndef NDEBUG
  DebugStore("# Start\r\n");
#endif

  LoadWindFromRegistry();
  CalculateNewPolarCoef();
  StartupStore(TEXT("GlidePolar::SetBallast\n"));
  GlidePolar::SetBallast();

// VENTA-ADDON
#ifdef VENTA_DEBUG_KEY
  CreateProgressDialog(gettext(TEXT("DEBUG KEY MODE ACTIVE")));
  Sleep(2000);
#endif
#ifdef VENTA_DEBUG_EVENT
  CreateProgressDialog(gettext(TEXT("DEBUG EVENT MODE ACTIVE")));
  Sleep(2000);
#endif
#ifdef VENTA_NOREGFONT
  CreateProgressDialog(gettext(TEXT("NO REGISTRY FONT LOAD")));
  Sleep(2000);
#endif

#ifdef CREDITS_FIVV
CreateProgressDialog(gettext(TEXT("Special ITA version")));
 Sleep(1000);
#endif

#ifdef PNA // VENTA-ADDON

	TCHAR sTmp[MAX_PATH];
        _stprintf(sTmp,TEXT("Conf=%s%S"), gmfpathname(),XCSDATADIR ); // VENTA2 FIX double backslash
	CreateProgressDialog(sTmp); Sleep(3000);

/*
	if (  !wcscmp(GlobalModelName, _T("UNKNOWN")) ) SetModelType();
*/
	_stprintf(sTmp, TEXT("PNA MODEL=%s (%d)"), GlobalModelName, GlobalModelType);
	CreateProgressDialog(sTmp); Sleep(3000);
#else
#if defined(FIVV) && ( !defined(WINDOWSPC) || WINDOWSPC==0 )
  if ( didfonts == 0 ) {
    CreateProgressDialog(TEXT("NEW FONTS INSTALLED!")); Sleep(3000);
  } else
    if ( didfonts == 5 ) {
      CreateProgressDialog(TEXT("ERROR NO FONTS!")); Sleep(3000);
    }
#endif
  TCHAR sTmpA[MAX_PATH], sTmpB[MAX_PATH];
  LocalPath(sTmpA,_T(""));
#if defined(FIVV) && ( !defined(WINDOWSPC) || WINDOWSPC==0 )
  if ( !datadir ) {
    CreateProgressDialog(TEXT("ERROR NO DIRECTORY:"));
    Sleep(3000);
  }
#endif
  _stprintf(sTmpB, TEXT("Conf=%s"),sTmpA);
  CreateProgressDialog(sTmpB); Sleep(3000);
#if defined(FIVV) && ( !defined(WINDOWSPC) || WINDOWSPC==0 )
  if ( !datadir ) {
    CreateProgressDialog(TEXT("CHECK INSTALLATION!"));
    Sleep(3000);
  }
#endif
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

  RasterTerrain::OpenTerrain();

  ReadWayPoints();
  InitWayPointCalc(); // VENTA3
  InitLDRotary(&rotaryLD);
/*
  for (int i=0; i< NumberOfWayPoints; i++) WayPointList[i].Preferred = false;
*/


  ReadAirfieldFile();
  SetHome(false);

  RasterTerrain::ServiceFullReload(GPS_INFO.Latitude,
                                   GPS_INFO.Longitude);

  CreateProgressDialog(gettext(TEXT("Scanning weather forecast")));
  StartupStore(TEXT("RASP load\n"));
  RASP.Scan(GPS_INFO.Latitude, GPS_INFO.Longitude);

  ReadAirspace();
  SortAirspace();

  OpenTopology();
  TopologyInitialiseMarks();

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

#ifndef _SIM_
  StartupStore(TEXT("RestartCommPorts\n"));
  RestartCommPorts();
#endif
#if (WINDOWSPC>0)
  devInit(TEXT(""));
#endif

  // re-set polar in case devices need the data
  StartupStore(TEXT("GlidePolar::SetBallast\n"));
  GlidePolar::SetBallast();

#if (EXPERIMENTAL > 0)
  CreateProgressDialog(gettext(TEXT("Bluetooth dialup SMS")));
  bsms.Initialise();
#endif

  CreateProgressDialog(gettext(TEXT("Initialising display")));

  // just about done....

  DoSunEphemeris(GPS_INFO.Longitude, GPS_INFO.Latitude);

  // Finally ready to go
  StartupStore(TEXT("CreateDrawingThread\n"));
  MapWindow::CreateDrawingThread();
  Sleep(100);
  StartupStore(TEXT("ShowInfoBoxes\n"));
  ShowInfoBoxes();

  SwitchToMapWindow();
  StartupStore(TEXT("CreateCalculationThread\n"));
  CreateCalculationThread();
  Sleep(500);

  StartupStore(TEXT("AirspaceWarnListInit\n"));
  AirspaceWarnListInit();
  StartupStore(TEXT("dlgAirspaceWarningInit\n"));
  dlgAirspaceWarningInit();

  // find unique ID of this PDA
  ReadAssetNumber();

  // Da-da, start everything now
  StartupStore(TEXT("ProgramStarted=1\n"));
  ProgramStarted = psInitDone;

  GlobalRunning = true;

#if _DEBUG
 // _crtBreakAlloc = -1;     // Set this to the number in {} brackets to
                           // break on a memory leak
#endif

  // Main message loop:
  while (/* GlobalRunning && */
         GetMessage(&msg, NULL, 0, 0))
    {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
    }

#if (WINDOWSPC>0)
#if _DEBUG
  _CrtCheckMemory();
  _CrtDumpMemoryLeaks();
#endif
#endif

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
//  wc.style                      = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // VENTA3 NO USE
  wc.lpfnWndProc                = (WNDPROC) WndProc;
  wc.cbClsExtra                 = 0;
#if (WINDOWSPC>0)
  wc.cbWndExtra = 0;
#else
  wc.cbWndExtra                 = dc.cbWndExtra ;
#endif
  wc.hInstance                  = hInstance;
#if defined(GNAV) && !defined(PCGNAV)
  wc.hIcon = NULL;
#else
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOARSWIFT));
#endif
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
    logfont->lfQuality = CLEARTYPE_COMPAT_QUALITY; // VENTA TODO FIX HERE. WHY NOT LANDSCAPE? cleartype is not native, but better than nothing!
#ifndef NOCLEARTYPE
  if (!InfoBoxLayout::landscape) {
    logfont->lfQuality = CLEARTYPE_COMPAT_QUALITY; // VENTA TODO FIX HERE. WHY NOT LANDSCAPE? cleartype is not native, but better than nothing!
  }
#endif
  }
}

bool IsNullLogFont(LOGFONT logfont) {
  bool bRetVal=false;

  LOGFONT LogFontBlank;
  memset ((char *)&LogFontBlank, 0, sizeof (LOGFONT));

  if ( memcmp(&logfont, &LogFontBlank, sizeof(LOGFONT)) == 0) {
    bRetVal=true;
  }
  return bRetVal;
}

void InitializeOneFont (HFONT * theFont,
                               const TCHAR FontRegKey[] ,
                               LOGFONT autoLogFont,
                               LOGFONT * LogFontUsed)
{
  LOGFONT logfont;
  int iDelStatus = 0;
  if (GetObjectType(*theFont) == OBJ_FONT) {
    iDelStatus=DeleteObject(*theFont); // RLD the EditFont screens use the Delete
  }

  memset ((char *)&logfont, 0, sizeof (LOGFONT));

  if (UseCustomFonts) {
    propGetFontSettings((TCHAR * )FontRegKey, &logfont);
    if (!IsNullLogFont(logfont)) {
      *theFont = CreateFontIndirect (&logfont);
      if (GetObjectType(*theFont) == OBJ_FONT) {
        if (LogFontUsed != NULL) *LogFontUsed = logfont; // RLD save for custom font GUI
      }
    }
  }

  if (GetObjectType(*theFont) != OBJ_FONT) {
    if (!IsNullLogFont(autoLogFont)) {
      ApplyClearType(&autoLogFont);
      *theFont = CreateFontIndirect (&autoLogFont);
      if (GetObjectType(*theFont) == OBJ_FONT) {
        if (LogFontUsed != NULL) *LogFontUsed = autoLogFont; // RLD save for custom font GUI
      }
    }
  }
}

void InitialiseFontsHardCoded(RECT rc,
                        LOGFONT * ptrhardInfoWindowLogFont,
                        LOGFONT * ptrhardTitleWindowLogFont,
                        LOGFONT * ptrhardMapWindowLogFont,
                        LOGFONT * ptrhardTitleSmallWindowLogFont,
                        LOGFONT * ptrhardMapWindowBoldLogFont,
                        LOGFONT * ptrhardCDIWindowLogFont, // New
                        LOGFONT * ptrhardMapLabelLogFont,
                        LOGFONT * ptrhardStatisticsLogFont) {



  memset ((char *)ptrhardInfoWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrhardTitleWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrhardMapWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrhardTitleSmallWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrhardMapWindowBoldLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrhardCDIWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrhardMapLabelLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrhardStatisticsLogFont, 0, sizeof (LOGFONT));


/*
 * VENTA-ADDON 2/2/08
 * Adding custom font settings for PNAs
 *
 * InfoWindowFont	= values inside infoboxes  like numbers, etc.
 * TitleWindowFont	= Titles of infoboxes like Next, WP L/D etc.
 * TitleSmallWindowFont =
 * CDIWindowFont	= vario display, runway informations
 * MapLabelFont		= Flarm Traffic draweing and stats, map labels in italic
 * StatisticsFont
 * MapWindowFont	= text names on the map
 * MapWindowBoldFont = menu buttons, waypoint selection, messages, etc.
 *
 *
 */
// VENTA2-ADDON  different infobox fonts for different geometries on HP31X.
// VENTA2-ADDON	 different ELLIPSE values for different geometries!
// RLD this loads the elipses each time and handles the fonts with the new font system
// VENTA4  ok but should apply only for PNAs, not for PC and PDAs..  For PDA there was a font registry problem,
//         but RLD fontsystem should have fixed it once forever.
//
//#if defined(PNA) || defined(FIVV)


  int iWidth=rc.right-rc.left;
  int iHeight=rc.bottom-rc.top;

  if (iWidth == 240 && iHeight == 320) ScreenSize=(ScreenSize_t)ss240x320; // QVGA	portrait
  if (iWidth == 480 && iHeight == 640) ScreenSize=(ScreenSize_t)ss480x640; //  VGA
  if (iWidth == 480 && iHeight == 800) ScreenSize=(ScreenSize_t)ss480x800;

  if (iWidth == 480 && iHeight == 272) ScreenSize=(ScreenSize_t)ss480x272; // WQVGA	landscape
  if (iWidth == 320 && iHeight == 240) ScreenSize=(ScreenSize_t)ss320x240; //  QVGA
  if (iWidth == 480 && iHeight == 234) ScreenSize=(ScreenSize_t)ss480x234; //   iGo
  if (iWidth == 640 && iHeight == 480) ScreenSize=(ScreenSize_t)ss640x480; //   VGA
  if (iWidth == 800 && iHeight == 480) ScreenSize=(ScreenSize_t)ss800x480; //  WVGA

  TCHAR tbuf[80];
  if (ScreenSize==0) {
	wsprintf(tbuf,_T("--- ERROR UNKNOWN RESOLUTION %dx%d !\r\n"),iWidth,iHeight);
	StartupStore(tbuf);
  }

#if defined(PNA)  // VENTA4

  if (ScreenSize==(ScreenSize_t)ss480x272) { // WQVGA  e.g. MIO
    propGetFontSettingsFromString(TEXT("28,0,0,0,800,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleSmallWindowLogFont);
    propGetFontSettingsFromString(TEXT("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardCDIWindowLogFont);
    propGetFontSettingsFromString(TEXT("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardMapLabelLogFont); // RLD 16 works well too
    propGetFontSettingsFromString(TEXT("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardStatisticsLogFont);//  (RLD is this used?)
    propGetFontSettingsFromString(TEXT("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardMapWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardMapWindowBoldLogFont);
    if (Appearance.InfoBoxGeom == 5) {
      GlobalEllipse=1.32f; // We don't use vario gauge in landscape geo5 anymore.. but doesn't hurt.
    }
    else {
      GlobalEllipse=1.1f;
    }
  }

  else if (ScreenSize==(ScreenSize_t)ss480x234) { // e.g. Messada 2440
    propGetFontSettingsFromString(TEXT("22,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
    propGetFontSettingsFromString(TEXT("18,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
    propGetFontSettingsFromString(TEXT("20,0,0,0,400,1,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleSmallWindowLogFont);
    propGetFontSettingsFromString(TEXT("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardCDIWindowLogFont);
    propGetFontSettingsFromString(TEXT("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardMapLabelLogFont); // RLD 16 works well too
    propGetFontSettingsFromString(TEXT("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardStatisticsLogFont);//  (RLD is this used?)
    propGetFontSettingsFromString(TEXT("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardMapWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardMapWindowBoldLogFont);
    GlobalEllipse=1.1f; // to be checked, TODO
  }

  else if (ScreenSize==(ScreenSize_t)ss800x480) {// e.g. ipaq 31x {

    switch (Appearance.InfoBoxGeom) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 6: // standard landscape
            propGetFontSettingsFromString(TEXT("56,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
            propGetFontSettingsFromString(TEXT("20,0,0,0,200,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
            GlobalEllipse=1.1f;	// standard VENTA2-addon
            break;
      case 4:
      case 5:
            propGetFontSettingsFromString(TEXT("64,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
            propGetFontSettingsFromString(TEXT("26,0,0,0,600,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
            GlobalEllipse=1.32f;	// VENTA2-addon
            break;
      case 7:
            propGetFontSettingsFromString(TEXT("66,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
            propGetFontSettingsFromString(TEXT("23,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
		  break;

       // This is a failsafe with an impossible setting so that you know
      // something is going very wrong.
       default:
            propGetFontSettingsFromString(TEXT("30,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
            propGetFontSettingsFromString(TEXT("10,0,0,0,200,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
          //}
            break;
    } // special geometry cases for 31x


    propGetFontSettingsFromString(TEXT("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleSmallWindowLogFont);
    propGetFontSettingsFromString(TEXT("36,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardCDIWindowLogFont);
    propGetFontSettingsFromString(TEXT("28,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardMapLabelLogFont);
    propGetFontSettingsFromString(TEXT("48,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardStatisticsLogFont);
    propGetFontSettingsFromString(TEXT("36,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardMapWindowLogFont);
    propGetFontSettingsFromString(TEXT("32,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardMapWindowBoldLogFont);


  }

/* VENTA5 TEST automatic fallback for 320x240,640x480 and unusual resolutions
  // Fallback for any other resolution
  else if (InfoBoxLayout::landscape) {

    propGetFontSettingsFromString(TEXT("28,0,0,0,800,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleSmallWindowLogFont);
    propGetFontSettingsFromString(TEXT("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardCDIWindowLogFont);
    propGetFontSettingsFromString(TEXT("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardMapLabelLogFont);
    propGetFontSettingsFromString(TEXT("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardStatisticsLogFont);
    propGetFontSettingsFromString(TEXT("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardMapWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardMapWindowBoldLogFont);
  }
  else { // portrait

    propGetFontSettingsFromString(TEXT("28,0,0,0,800,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleSmallWindowLogFont);
    propGetFontSettingsFromString(TEXT("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardCDIWindowLogFont);
    propGetFontSettingsFromString(TEXT("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardMapLabelLogFont);
    propGetFontSettingsFromString(TEXT("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardStatisticsLogFont);
    propGetFontSettingsFromString(TEXT("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardMapWindowLogFont);
    propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardMapWindowBoldLogFont);
  }
*/

#endif //PNA

#if defined(GNAV) || defined(PCGNAV) || defined(GNAV_FONTEST)  // RLD Altair also loads these in registry and by default, uses the registry
   propGetFontSettingsFromString(TEXT("24,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"), ptrhardInfoWindowLogFont);
   propGetFontSettingsFromString(TEXT("10,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicNineCond"), ptrhardTitleWindowLogFont);
   propGetFontSettingsFromString(TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"), ptrhardCDIWindowLogFont);
   propGetFontSettingsFromString(TEXT("13,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicTwelveCond"), ptrhardMapLabelLogFont);
   propGetFontSettingsFromString(TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), ptrhardStatisticsLogFont);
   propGetFontSettingsFromString(TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), ptrhardMapWindowLogFont);
   propGetFontSettingsFromString(TEXT("15,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), ptrhardMapWindowBoldLogFont);
   propGetFontSettingsFromString(TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"), ptrhardTitleSmallWindowLogFont);

#endif //Altair




}

void InitialiseFontsAuto(RECT rc,
                        LOGFONT * ptrautoInfoWindowLogFont,
                        LOGFONT * ptrautoTitleWindowLogFont,
                        LOGFONT * ptrautoMapWindowLogFont,
                        LOGFONT * ptrautoTitleSmallWindowLogFont,
                        LOGFONT * ptrautoMapWindowBoldLogFont,
                        LOGFONT * ptrautoCDIWindowLogFont, // New
                        LOGFONT * ptrautoMapLabelLogFont,
                        LOGFONT * ptrautoStatisticsLogFont) {
  LOGFONT logfont;
  int FontHeight, FontWidth;
  int fontsz1 = (rc.bottom - rc.top );
  int fontsz2 = (rc.right - rc.left );

  memset ((char *)ptrautoInfoWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrautoTitleWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrautoMapWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrautoTitleSmallWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrautoMapWindowBoldLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrautoCDIWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrautoMapLabelLogFont, 0, sizeof (LOGFONT));
  memset ((char *)ptrautoStatisticsLogFont, 0, sizeof (LOGFONT));

  // VENTA TODO : reconsider all algorithms for unconventional screen resolutions, expecially wide screens where 1.66 and 2.03 multipliers apply
  if (fontsz1<fontsz2) { // portrait
    FontHeight = (int)(fontsz1/FONTHEIGHTRATIO*1.33);  // use small dimension, to work for widscreens and adjust so it works for 4x3 screens too.
    FontWidth = (int)(FontHeight*0.4);
  }
  else if (fontsz1==fontsz2)
  {  // square
    FontHeight = (int)(fontsz2/FONTHEIGHTRATIO);
    FontWidth = (int)(FontHeight*0.4);
  }
  else
  { // landscape
    FontHeight = (int)(fontsz2/FONTHEIGHTRATIO*1.33);
    FontWidth = (int)(FontHeight*0.4);
  }

  int iFontHeight = (int)(FontHeight*1.4);
  // oversize first so can then scale down

  FontWidth = 0; // JMW this should be done so closest font is found

  // sgi todo

  memset ((char *)&logfont, 0, sizeof (logfont));

// #if defined(PNA) || defined(FIVV)  // Only for PNA, since we still do not copy Fonts in their Windows memory.
				      // though we could already do it automatically.
#if defined(PNA)
	_tcscpy(logfont.lfFaceName, _T("Tahoma")); // VENTA TODO copy DejaVu fonts also for PNA like for PDAs in SD version
#else
  _tcscpy(logfont.lfFaceName, _T("DejaVu Sans Condensed"));
#endif


  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = iFontHeight;
  logfont.lfWidth =  FontWidth;
  logfont.lfWeight = FW_BOLD;
  logfont.lfItalic = TRUE;
  logfont.lfCharSet = ANSI_CHARSET;
  ApplyClearType(&logfont);

  // JMW algorithm to auto-size info window font.
  // this is still required in case title font property doesn't exist.
  SIZE tsize;
  HDC iwhdc = GetDC(hWndMainWindow);
  do {
    HFONT TempWindowFont;
    HFONT hfOld;

    iFontHeight--;
    logfont.lfHeight = iFontHeight;
//    InfoWindowFont = CreateFontIndirect (&logfont);
//    SelectObject(iwhdc, InfoWindowFont);

    TempWindowFont = CreateFontIndirect (&logfont);
    hfOld=(HFONT)SelectObject(iwhdc, TempWindowFont);


    GetTextExtentPoint(iwhdc, TEXT("00:00"), 5, &tsize);
//    DeleteObject(InfoWindowFont);
    SelectObject(iwhdc, hfOld); // unselect it before deleting it
    DeleteObject(TempWindowFont);

  } while (tsize.cx>InfoBoxLayout::ControlWidth);
  ReleaseDC(hWndMainWindow, iwhdc);

  iFontHeight++;
  logfont.lfHeight = iFontHeight;

//  propGetFontSettings(TEXT("InfoWindowFont"), &logfont);
//  InfoWindowFont = CreateFontIndirect (&logfont);
 memcpy ((void *)ptrautoInfoWindowLogFont, &logfont, sizeof (LOGFONT));


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
  // RLD this was the only auto font to not have "ApplyClearType()".  It does not apply to very small fonts
  // we now apply ApplyClearType to all fonts in CreateOneFont().

//  propGetFontSettings(TEXT("TitleWindowFont"), &logfont);
//  TitleWindowFont = CreateFontIndirect (&logfont);
  memcpy ((void *)ptrautoTitleWindowLogFont, &logfont, sizeof (LOGFONT));

  memset ((char *)&logfont, 0, sizeof (logfont));

  // new font for CDI Scale

  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));
  logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*CDIFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*CDIFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
//  ApplyClearType(&logfont);

//  propGetFontSettings(TEXT("CDIWindowFont"), &logfont);
//  CDIWindowFont = CreateFontIndirect (&logfont);
  memcpy ((void *)ptrautoCDIWindowLogFont, &logfont, sizeof (LOGFONT));

  // new font for map labels
  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*MAPFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*MAPFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
  logfont.lfItalic = TRUE;
//  ApplyClearType(&logfont);

//  propGetFontSettings(TEXT("MapLabelFont"), &logfont);
//  MapLabelFont = CreateFontIndirect (&logfont);
  memcpy ((void *)ptrautoMapLabelLogFont, &logfont, sizeof (LOGFONT));


  // Font for map other text
  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*STATISTICSFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*STATISTICSFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
//  ApplyClearType(&logfont);

//  propGetFontSettings(TEXT("StatisticsFont"), &logfont);
//  StatisticsFont = CreateFontIndirect (&logfont);
  memcpy ((void *)ptrautoStatisticsLogFont, &logfont, sizeof (LOGFONT));

  // new font for map labels

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*MAPFONTHEIGHTRATIO*1.3);
  logfont.lfWidth =  (int)(FontWidth*MAPFONTWIDTHRATIO*1.3);
  logfont.lfWeight = FW_MEDIUM;
//  ApplyClearType(&logfont);

//  propGetFontSettings(TEXT("MapWindowFont"), &logfont);
//  MapWindowFont = CreateFontIndirect (&logfont);

//  SendMessage(hWndMapWindow,WM_SETFONT,
//        (WPARAM)MapWindowFont,MAKELPARAM(TRUE,0));
  memcpy ((void *)ptrautoMapWindowLogFont, &logfont, sizeof (LOGFONT));

  // Font for map bold text

  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfWeight = FW_BOLD;
  logfont.lfWidth =  0; // JMW (int)(FontWidth*MAPFONTWIDTHRATIO*1.3) +2;

//  propGetFontSettings(TEXT("MapWindowBoldFont"), &logfont);
//  MapWindowBoldFont = CreateFontIndirect (&logfont);
  memcpy ((void *)ptrautoMapWindowBoldLogFont, &logfont, sizeof (LOGFONT));

  // TODO code: create font settings for this one...
  memset((char *)&logfont, 0, sizeof (logfont));
  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));

  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = IBLSCALE(20);
  logfont.lfWidth =  IBLSCALE(8);
  logfont.lfWeight = FW_MEDIUM;

  memcpy ((void *)ptrautoTitleSmallWindowLogFont, &logfont, sizeof (LOGFONT));
}

//  propGetFontSettings(TEXT("TeamCodeFont"), &logfont);
//  TitleSmallWindowFont = CreateFontIndirect (&logfont);


void InitialiseFonts(RECT rc)
{ //this routine must be called only at start/restart of XCSoar b/c there are many pointers to these fonts

  DeleteObject(InfoWindowFont);
  DeleteObject(TitleWindowFont);
  DeleteObject(MapWindowFont);
  DeleteObject(TitleSmallWindowFont);
  DeleteObject(MapWindowBoldFont);
  DeleteObject(CDIWindowFont);
  DeleteObject(MapLabelFont);
  DeleteObject(StatisticsFont);

  memset ((char *)&autoInfoWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&autoTitleWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&autoMapWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&autoTitleSmallWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&autoMapWindowBoldLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&autoCDIWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&autoMapLabelLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&autoStatisticsLogFont, 0, sizeof (LOGFONT));


  InitialiseFontsAuto(rc,
                        &autoInfoWindowLogFont,
                        &autoTitleWindowLogFont,
                        &autoMapWindowLogFont,
                        &autoTitleSmallWindowLogFont,
                        &autoMapWindowBoldLogFont,
                        &autoCDIWindowLogFont, // New
                        &autoMapLabelLogFont,
                        &autoStatisticsLogFont);


  LOGFONT hardInfoWindowLogFont;
  LOGFONT hardTitleWindowLogFont;
  LOGFONT hardMapWindowLogFont;
  LOGFONT hardTitleSmallWindowLogFont;
  LOGFONT hardMapWindowBoldLogFont;
  LOGFONT hardCDIWindowLogFont;
  LOGFONT hardMapLabelLogFont;
  LOGFONT hardStatisticsLogFont;

  memset ((char *)&hardInfoWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&hardTitleWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&hardMapWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&hardTitleSmallWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&hardMapWindowBoldLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&hardCDIWindowLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&hardMapLabelLogFont, 0, sizeof (LOGFONT));
  memset ((char *)&hardStatisticsLogFont, 0, sizeof (LOGFONT));

  InitialiseFontsHardCoded(rc,
                        &hardInfoWindowLogFont,
                        &hardTitleWindowLogFont,
                        &hardMapWindowLogFont,
                        &hardTitleSmallWindowLogFont,
                        &hardMapWindowBoldLogFont,
                        &hardCDIWindowLogFont, // New
                        &hardMapLabelLogFont,
                        &hardStatisticsLogFont);

// for PNA & GNAV, merge the "hard" into the "auto" if one exists
  if (!IsNullLogFont(hardInfoWindowLogFont))
    autoInfoWindowLogFont = hardInfoWindowLogFont;

  if (!IsNullLogFont(hardTitleWindowLogFont))
    autoTitleWindowLogFont = hardTitleWindowLogFont;

  if (!IsNullLogFont(hardMapWindowLogFont))
    autoMapWindowLogFont = hardMapWindowLogFont;

  if (!IsNullLogFont(hardTitleSmallWindowLogFont))
    autoTitleSmallWindowLogFont = hardTitleSmallWindowLogFont;

  if (!IsNullLogFont(hardMapWindowBoldLogFont))
    autoMapWindowBoldLogFont = hardMapWindowBoldLogFont;

  if (!IsNullLogFont(hardCDIWindowLogFont))
    autoCDIWindowLogFont = hardCDIWindowLogFont;

  if (!IsNullLogFont(hardMapLabelLogFont))
    autoMapLabelLogFont = hardMapLabelLogFont;

  if (!IsNullLogFont(hardStatisticsLogFont))
    autoStatisticsLogFont = hardStatisticsLogFont;

/////////////////////////////////////////////////////////

  InitializeOneFont (&InfoWindowFont,
                        szRegistryFontInfoWindowFont,
                        autoInfoWindowLogFont,
                        NULL);

  InitializeOneFont (&TitleWindowFont,
                        szRegistryFontTitleWindowFont,
                        autoTitleWindowLogFont,
                        NULL);

  InitializeOneFont (&CDIWindowFont,
                        szRegistryFontCDIWindowFont,
                        autoCDIWindowLogFont,
                        NULL);

  InitializeOneFont (&MapLabelFont,
                        szRegistryFontMapLabelFont,
                        autoMapLabelLogFont,
                        NULL);

  InitializeOneFont (&StatisticsFont,
                        szRegistryFontStatisticsFont,
                        autoStatisticsLogFont,
                        NULL);

  InitializeOneFont (&MapWindowFont,
                        szRegistryFontMapWindowFont,
                        autoMapWindowLogFont,
                        NULL);
  SendMessage(hWndMapWindow,WM_SETFONT,
              (WPARAM)MapWindowFont,MAKELPARAM(TRUE,0));

  InitializeOneFont (&MapWindowBoldFont,
                        szRegistryFontMapWindowBoldFont,
                        autoMapWindowBoldLogFont,
                        NULL);

  InitializeOneFont (&TitleSmallWindowFont,
                        szRegistryFontTitleSmallWindowFont,
                        autoTitleSmallWindowLogFont,
                        NULL);
}


#if (WINDOWSPC>0)
int SCREENWIDTH=640;
int SCREENHEIGHT=480;
#endif

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

#ifdef PNA
	CleanRegistry(); // VENTA2-FIX for PNA we can't delete all registries..by now
#endif
  PreloadInitialisation(true);

  MyRegisterClass(hInst, szWindowClass);

  RECT WindowSize;

  WindowSize.left = 0;
  WindowSize.top = 0;
  WindowSize.right = GetSystemMetrics(SM_CXSCREEN);
  WindowSize.bottom = GetSystemMetrics(SM_CYSCREEN);


#if (WINDOWSPC>0)
  WindowSize.right = SCREENWIDTH
    + 2*GetSystemMetrics( SM_CXFIXEDFRAME);
  WindowSize.left = (GetSystemMetrics(SM_CXSCREEN) - WindowSize.right) / 2;
  WindowSize.bottom = SCREENHEIGHT
    + 2*GetSystemMetrics( SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);
  WindowSize.top = (GetSystemMetrics(SM_CYSCREEN) - WindowSize.bottom) / 2;
#endif

  StartupStore(TEXT("Create main window\n"));

  hWndMainWindow = CreateWindow(szWindowClass, szTitle,
                                WS_SYSMENU
                                | WS_CLIPCHILDREN
				| WS_CLIPSIBLINGS,
                                WindowSize.left, WindowSize.top,
				WindowSize.right, WindowSize.bottom,
                                NULL, NULL,
				hInstance, NULL);

  if (!hWndMainWindow)
    {
      return FALSE;
    }

#if defined(GNAV) && !defined(PCGNAV)
  // TODO code: release the handle?
  HANDLE hTmp = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOARSWIFT));
  SendMessage(hWndMainWindow, WM_SETICON,
	      (WPARAM)ICON_BIG, (LPARAM)hTmp);
  SendMessage(hWndMainWindow, WM_SETICON,
	      (WPARAM)ICON_SMALL, (LPARAM)hTmp);
#endif

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

  StartupStore(TEXT("InfoBox geometry\n"));

  InfoBoxLayout::ScreenGeometry(rc);

  ///////////////////////////////////////// create infoboxes

  StartupStore(TEXT("Load unit bitmaps\n"));

  Units::LoadUnitBitmap(hInstance);

  StartupStore(TEXT("Create info boxes\n"));

  InfoBoxLayout::CreateInfoBoxes(rc);

  StartupStore(TEXT("Create FLARM gauge\n"));
  GaugeFLARM::Create();

  StartupStore(TEXT("Create button labels\n"));
  ButtonLabel::CreateButtonLabels(rc);
  ButtonLabel::SetLabelText(0,TEXT("MODE"));

  ////////////////// do fonts
  StartupStore(TEXT("Initialise fonts\n"));
  InitialiseFonts(rc);

  ButtonLabel::SetFont(MapWindowBoldFont);

  StartupStore(TEXT("Initialise message system\n"));
  Message::Initialize(rc); // creates window, sets fonts

  ShowWindow(hWndMainWindow, SW_SHOW);

  ///////////////////////////////////////////////////////
  //// create map window

  StartupStore(TEXT("Create map window\n"));

  hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,
			       WS_VISIBLE | WS_CHILD
			       | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                               0, 0, (rc.right - rc.left),
			       (rc.bottom-rc.top) ,
                               hWndMainWindow, NULL ,hInstance,NULL);

  // JMW gauge creation was here

  ShowWindow(hWndMainWindow, nCmdShow);

  UpdateWindow(hWndMainWindow);

  return TRUE;
}


int getInfoType(int i) {
  int retval = 0;
  if (i<0) return 0; // error

  if (EnableAuxiliaryInfo) {
    retval = (InfoType[i] >> 24) & 0xff; // auxiliary
  } else {
    if (DisplayMode == dmCircling)
      retval = InfoType[i] & 0xff; // climb
    else if (DisplayMode == dmFinalGlide) {
      retval = (InfoType[i] >> 16) & 0xff; //final glide
    } else {
      retval = (InfoType[i] >> 8) & 0xff; // cruise
    }
  }
  return min(NUMSELECTSTRINGS-1,retval);
}


void setInfoType(int i, char j) {
  if (i<0) return; // error

  if (EnableAuxiliaryInfo) {
    InfoType[i] &= 0x00ffffff;
    InfoType[i] += (j<<24);
  } else {
    if (DisplayMode == dmCircling) {
      InfoType[i] &= 0xffffff00;
      InfoType[i] += (j);
    } else if (DisplayMode == dmFinalGlide) {
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
  Data_Options[min(NUMSELECTSTRINGS-1,i)].Process(keycode);
  UnlockFlightData();

  UnlockNavBox();

  InfoBoxesDirty = true;

  TriggerGPSUpdate(); // emulate update to trigger calculations

  InfoBoxFocusTimeOut = 0;
  DisplayTimeOut = 0;

}


// Debounce input buttons (does not matter which button is pressed)
// VNT 090702 FIX Careful here: synthetic double clicks and virtual keys require some timing.
//				See Defines.h DOUBLECLICKINTERVAL . Not sure they are 100% independent.

int debounceTimeout=200;

bool Debounce(void) {
  static DWORD fpsTimeLast= 0;
  DWORD fpsTimeThis = ::GetTickCount();
  DWORD dT = fpsTimeThis-fpsTimeLast;

  DisplayTimeOut = 0;
  InterfaceTimeoutReset();

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

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));
  StartHourglassCursor();

  StartupStore(TEXT("Entering shutdown...\n"));
  StartupLogFreeRamAndStorage();

  // turn off all displays
  GlobalRunning = false;

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

  // Stop SMS device
#if (EXPERIMENTAL > 0)
  bsms.Shutdown();
#endif

  // Stop drawing
  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  StartupStore(TEXT("CloseDrawingThread\n"));
  MapWindow::CloseDrawingThread();

  // Stop calculating too (wake up)
  dataTriggerEvent.trigger();
  drawTriggerEvent.trigger();
  varioTriggerEvent.trigger();

  // Clear data

  CreateProgressDialog(gettext(TEXT("Shutdown, saving task...")));
  StartupStore(TEXT("Save default task\n"));
  LockTaskData();
  ResumeAbortTask(-1); // turn off abort if it was on.
  UnlockTaskData();
  SaveDefaultTask();

  StartupStore(TEXT("Clear task data\n"));

  LockTaskData();
  Task[0].Index = -1;  ActiveWayPoint = -1;
  AATEnabled = FALSE;
  NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0;
  NumberOfAirspaceCircles = 0;
  CloseWayPoints();
  UnlockTaskData();

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));
  StartupStore(TEXT("CloseTerrainTopology\n"));

  RASP.Close();
  RasterTerrain::CloseTerrain();

  CloseTopology();

  TopologyCloseMarks();

  CloseTerrainRenderer();

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

  ProgramStarted = psInitInProgress;

  // Kill windows

  StartupStore(TEXT("Close Gauges\n"));

  GaugeCDI::Destroy();
  GaugeVario::Destroy();
  GaugeFLARM::Destroy();

  StartupStore(TEXT("Close Messages\n"));
  Message::Destroy();

  Units::UnLoadUnitBitmap();

  StartupStore(TEXT("Destroy Info Boxes\n"));
  InfoBoxLayout::DestroyInfoBoxes();

  StartupStore(TEXT("Destroy Button Labels\n"));
  ButtonLabel::Destroy();

  StartupStore(TEXT("Delete Objects\n"));

  //  CommandBar_Destroy(hWndCB);
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
  DeleteObject(TitleSmallWindowFont);

  if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
  if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
  if(AirspaceScreenPoint != NULL)  LocalFree((HLOCAL)AirspaceScreenPoint);
  if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);

  StartupStore(TEXT("Delete Critical Sections\n"));

  DeleteCriticalSection(&CritSec_EventQueue);
  csEventQueueInitialized = false;
  DeleteCriticalSection(&CritSec_TaskData);
  csTaskDataInitialized = false;
  DeleteCriticalSection(&CritSec_FlightData);
  csFlightDataInitialized = false;
  DeleteCriticalSection(&CritSec_NavBox);
  csNavBoxInitialized = false;
  DeleteCriticalSection(&CritSec_Comm);
  csCommInitialized = false;
  DeleteCriticalSection(&CritSec_TerrainDataCalculations);
  csTerrainDataGraphicsInitialized = false;
  DeleteCriticalSection(&CritSec_TerrainDataGraphics);
  csTerrainDataCalculationsInitialized = false;

  StartupStore(TEXT("Close Progress Dialog\n"));

  CloseProgressDialog();

  StartupStore(TEXT("Close Calculations\n"));
  CloseCalculations();

  CloseGeoid();

  StartupStore(TEXT("Close Windows\n"));
  DestroyWindow(hWndMapWindow);
  DestroyWindow(hWndMainWindow);

#ifdef DEBUG_TRANSLATIONS
  StartupStore(TEXT("Writing missing translations\n"));
  WriteMissingTranslations();
#endif

  StartupLogFreeRamAndStorage();
  StartupStore(TEXT("Finished shutdown\n"));
  StopHourglassCursor();

#ifndef NDEBUG
  TCHAR foop[80];
  TASK_POINT wp;
  TASK_POINT *wpr = &wp;
  _stprintf(foop,TEXT("Sizes %d %d %d\n"),
	    sizeof(TASK_POINT),
	    ((long)&wpr->AATTargetLocked)-((long)wpr),
	    ((long)&wpr->Target)-((long)wpr)
	    );
  StartupStore(foop);
#endif
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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
      switch(wdata) {
      case 0:
        SetBkColor((HDC)wParam, ColorUnselected);
        SetTextColor((HDC)wParam, RGB(0x00,0x00,0x00));
        return (LRESULT)hBrushUnselected;
      case 1:
        SetBkColor((HDC)wParam, ColorSelected);
        SetTextColor((HDC)wParam, RGB(0x00,0x00,0x00));
        return (LRESULT)hBrushSelected;
      case 2:
	SetBkColor((HDC)wParam, ColorUnselected);
        SetTextColor((HDC)wParam, ColorWarning);
	return (LRESULT)hBrushUnselected;
      case 3:
	SetBkColor((HDC)wParam, ColorUnselected);
        SetTextColor((HDC)wParam, ColorOK);
	return (LRESULT)hBrushUnselected;
      case 4:
	// black on light green
	SetBkColor((HDC)wParam, ColorButton);
        SetTextColor((HDC)wParam, RGB(0x00,0x00,0x00));
	return (LRESULT)hBrushButton;
      case 5:
	// grey on light green
	SetBkColor((HDC)wParam, ColorButton);
        SetTextColor((HDC)wParam, RGB(0x80,0x80,0x80));
	return (LRESULT)hBrushButton;
      }
      break;
    case WM_CREATE:
#ifdef HAVE_ACTIVATE_INFO
      memset (&s_sai, 0, sizeof (s_sai));
      s_sai.cbSize = sizeof (s_sai);
#endif
      //if (hWnd==hWndMainWindow) {
      if (iTimerID == 0) {
        iTimerID = SetTimer(hWnd,1000,500,NULL); // 2 times per second
      }

      //      hWndCB = CreateRpCommandBar(hWnd);

      break;

    case WM_ACTIVATE:
      if(LOWORD(wParam) != WA_INACTIVE)
        {
          SetWindowPos(hWndMainWindow,HWND_TOP,
                 0, 0, 0, 0,
                 SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);

#ifdef HAVE_ACTIVATE_INFO
	  SHFullScreen(hWndMainWindow,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif

        }
#ifdef HAVE_ACTIVATE_INFO
      SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
#endif
      break;

    case WM_SETTINGCHANGE:
#ifdef HAVE_ACTIVATE_INFO
      SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
#endif
      break;

    case WM_SETFOCUS:
      // JMW not sure this ever does anything useful..
      if (ProgramStarted > psInitInProgress) {

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
      // TODO enhancement: Capture KEYDOWN time
      // 	- Pass that (otpionally) to processKey, allowing
      // 	  processKey to handle long events - at any length
      // 	- Not sure how to do double click... (need timer call back
      // 	process unless reset etc... tricky)
      // we do this in WindowControls
#if defined(GNAV) || defined(PCGNAV)
    case WM_KEYDOWN: // JMW was keyup
#else
    case WM_KEYUP: // JMW was keyup
#endif

      InterfaceTimeoutReset();

      /* DON'T PROCESS KEYS HERE WITH NEWINFOBOX, IT CAUSES CRASHES! */
      break;
	  //VENTA DBG
#ifdef VENTA_DEBUG_EVENT
	case WM_KEYDOWN:

		DoStatusMessage(TEXT("DBG KDOWN 1")); // VENTA
		InterfaceTimeoutReset();
	      break;
	case WM_SYSKEYDOWN:
		DoStatusMessage(TEXT("DBG SYSKDOWN 1")); // VENTA
		InterfaceTimeoutReset();
	      break;
#endif
	//END VENTA DBG

    case WM_TIMER:
      //      ASSERT(hWnd==hWndMainWindow);
      if (ProgramStarted > psInitInProgress) {
#ifdef _SIM_
	SIMProcessTimer();
#else
	ProcessTimer();
#endif
	if (ProgramStarted==psFirstDrawDone) {
	  AfterStartup();
	  ProgramStarted = psNormalOp;
          StartupStore(TEXT("ProgramStarted=3\n"));
          StartupLogFreeRamAndStorage();
	}
      }
      break;

    case WM_INITMENUPOPUP:
      if (ProgramStarted > psInitInProgress) {
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

#ifndef GNAV
      ASSERT(hWnd==hWndMainWindow);
      if(ForceShutdown || ((hWnd==hWndMainWindow) &&
         (MessageBoxX(gettext(TEXT("Quit program?")),
                      gettext(TEXT("XCSoar")),
                      MB_YESNO|MB_ICONQUESTION) == IDYES)))
#endif
        {
          if(iTimerID) {
            KillTimer(hWnd,iTimerID);
            iTimerID = 0;
          }

          Shutdown();
        }
      break;

    case WM_DESTROY:
      if (hWnd==hWndMainWindow) {
        PostQuitMessage(0);
      }
      break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  return 0;
}


/* JMW no longer needed
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
*/

LRESULT MainMenu(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  HWND wmControl;
  int i;

  wmId    = LOWORD(wParam);
  wmEvent = HIWORD(wParam);
  wmControl = (HWND)lParam;

  if(wmControl != NULL) {
    if (ProgramStarted==psNormalOp) {

      DialogActive = false;

      FullScreen();

      InfoBoxFocusTimeOut = 0;
      /*
      if (!InfoWindowActive) {
        ShowMenu();
      }
      */

      for(i=0;i<numInfoWindows;i++) {
        if(wmControl == InfoBoxes[i]->GetHandle()) {
          InfoWindowActive = TRUE;

          if(DisplayLocked) {
            if( i!= InfoFocus) {
              FocusOnWindow(i,true);
              FocusOnWindow(InfoFocus,false);

              InfoFocus = i;
              InfoWindowActive = TRUE;
            }
            DisplayText();
            InputEvents::setMode(TEXT("infobox"));

          } else {
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


void    AssignValues(void)
{
  if (InfoBoxesHidden) {
    // no need to assign values
    return;
  }

  //  DetectStartTime(); moved to Calculations

  // nothing to do here now!
}

extern int PDABatteryTemperature;
void DisplayText(void)
{
  if (InfoBoxesHidden)
    return;

  int i;
  static int DisplayType[MAXINFOWINDOWS];
  static bool first=true;
  static int InfoFocusLast = -1;
  static int DisplayTypeLast[MAXINFOWINDOWS];
// static double LastFlipBoxTime = 0; //  now global for SlowCalculations
  static bool FlipBoxValue = false;


  // VENTA3 - Dynamic box values
  if ( GPS_INFO.Time > LastFlipBoxTime + DYNABOXTIME ) {
/*
	static TCHAR ventabuffer[200];
	FILE *fp;
        wsprintf(ventabuffer,TEXT("GPS_INFO.Time=%d LastFlipBoxTime=%d Flip=%d"),(int)GPS_INFO.Time, (int)LastFlipBoxTime,
	FlipBoxValue == true ? 1 : 0);
        if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp)
;}
*/
	FlipBoxValue = ( FlipBoxValue == false );
	LastFlipBoxTime = GPS_INFO.Time;
  }



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

    // VENTA3
    // All calculations are made in a separate thread. Slow calculations should apply to
    // the function DoCalculationsSlow() . Do not put calculations here!

    DisplayType[i] = getInfoType(i);
    Data_Options[DisplayType[i]].Formatter->AssignValue(DisplayType[i]);

    TCHAR sTmp[32];

    int color = 0;

    bool needupdate = ((DisplayType[i] != DisplayTypeLast[i])||first);

    int theactive = ActiveWayPoint;
    if (!ValidTaskPoint(theactive)) {
      theactive = -1;
    }

    //
    // Set Infobox title and middle value. Bottom line comes next
    //
    switch (DisplayType[i]) {

    case 67: // VENTA3 alternate1 and 2
    case 68:
    case 69:
	if (DisplayType[i]==67) ActiveAlternate=Alternate1; else
	if (DisplayType[i]==68) ActiveAlternate=Alternate2;
		else ActiveAlternate=BestAlternate;
	InfoBoxes[i]->SetSmallerFont(false);
	if ( ActiveAlternate != -1 ) {
		InfoBoxes[i]->SetTitle(Data_Options[DisplayType[i]].Formatter->
			   RenderTitle(&color));
		InfoBoxes[i]->SetColor(color);
		InfoBoxes[i]->SetValue(Data_Options[DisplayType[i]].Formatter->
			   Render(&color));
		InfoBoxes[i]->SetColor(color);
	} else {
		if ( DisplayType[i]==67 )
			InfoBoxes[i]->SetTitle(TEXT("Altern1"));
		else if ( DisplayType[i]==68 )
			InfoBoxes[i]->SetTitle(TEXT("Altern2"));
		else	InfoBoxes[i]->SetTitle(TEXT("BestAltr"));
		InfoBoxes[i]->SetValue(TEXT("---"));
		InfoBoxes[i]->SetColor(-1);
	}
      if (needupdate)
	InfoBoxes[i]->SetValueUnit(Units::GetUserUnitByGroup(
          Data_Options[DisplayType[i]].UnitGroup));
	break;
    case 55:
      InfoBoxes[i]->SetSmallerFont(true);
      if (needupdate)
	InfoBoxes[i]->SetTitle(Data_Options[DisplayType[i]].Title);

      InfoBoxes[i]->
	SetValue(Data_Options[DisplayType[i]].Formatter->Render(&color));

      // to be optimized!
      if (needupdate)
	InfoBoxes[i]->
	  SetValueUnit(Units::GetUserUnitByGroup(
              Data_Options[DisplayType[i]].UnitGroup)
	  );
      InfoBoxes[i]->SetColor(color);
      break;
    case 14: // Next waypoint
      InfoBoxes[i]->SetSmallerFont(false);
      if (theactive != -1){
	InfoBoxes[i]->
	  SetTitle(Data_Options[DisplayType[i]].Formatter->
		   Render(&color));
	InfoBoxes[i]->SetColor(color);
	InfoBoxes[i]->
	  SetValue(Data_Options[47].Formatter->Render(&color));
      }else{
	InfoBoxes[i]->SetTitle(TEXT("Next"));
	InfoBoxes[i]->SetValue(TEXT("---"));
	InfoBoxes[i]->SetColor(-1);
      }
      if (needupdate)
	InfoBoxes[i]->SetValueUnit(Units::GetUserUnitByGroup(
          Data_Options[DisplayType[i]].UnitGroup)
      );
      break;
    default:
      InfoBoxes[i]->SetSmallerFont(false);
      if (needupdate)
	InfoBoxes[i]->SetTitle(Data_Options[DisplayType[i]].Title);

      InfoBoxes[i]->
          SetValue(Data_Options[DisplayType[i]].Formatter->Render(&color));

      // to be optimized!
      if (needupdate)
	InfoBoxes[i]->
	  SetValueUnit(Units::GetUserUnitByGroup(
            Data_Options[DisplayType[i]].UnitGroup)
	  );

      InfoBoxes[i]->SetColor(color);
    };

    //
    // Infobox bottom line
    //
    switch (DisplayType[i]) {
    case 14: // Next waypoint

      if (theactive != -1){
        int index;
        index = Task[theactive].Index;
        if (index>=0) {
          InfoBoxes[i]->
            SetComment(WayPointList[index].Comment);
        }
        break;
      }
      InfoBoxes[i]->SetComment(TEXT(""));
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
    case 27: // AAT time to go
    case 36: // flight time
    case 39: // current time
    case 40: // gps time
    case 41: // task time to go
    case 42: // task time to go
    case 45: // ete
    case 46: // leg ete
    case 62: // ete
      if (Data_Options[DisplayType[i]].Formatter->isValid()) {
        InfoBoxes[i]->
          SetComment(Data_Options[DisplayType[i]].Formatter->GetCommentText());
      } else {
        InfoBoxes[i]->
          SetComment(TEXT(""));
      }
      break;
    case 43:
      if (EnableBlockSTF) {
	InfoBoxes[i]->SetComment(TEXT("BLOCK"));
      } else {
	InfoBoxes[i]->SetComment(TEXT("DOLPHIN"));
      }
      break;
    case 55: // own team code
      InfoBoxes[i]->SetComment(TeammateCode);
      if (TeamFlarmTracking)
	{
	  if (IsFlarmTargetCNInRange())
	    {
	      InfoBoxes[i]->SetColorBottom(2);
	    }
	  else
	    {
	      InfoBoxes[i]->SetColorBottom(1);
	    }
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(0);
	}
      break;
    case 56: // team bearing

      if (TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(TeamFlarmCNTarget);
	    }
	  else
	    {
	      InfoBoxes[i]->SetComment(TEXT("???"));
	    }
	}
      else
	{
	  InfoBoxes[i]->SetComment(TEXT("---"));
	}

      if (IsFlarmTargetCNInRange())
	{
	  InfoBoxes[i]->SetColorBottom(2);
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(1);
	}

      break;
    case 57: // team bearing dif

      if (TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(TeamFlarmCNTarget);
	    }
	  else
	    {
	      InfoBoxes[i]->SetComment(TEXT("???"));
	    }
	}
      else
	{
	  InfoBoxes[i]->SetComment(TEXT("---"));
	}
      if (IsFlarmTargetCNInRange())
	{
	  InfoBoxes[i]->SetColorBottom(2);
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(1);
	}

      break;
    case 58: // team range

      if (TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(TeamFlarmCNTarget);
	    }
	  else
	    {
	      InfoBoxes[i]->SetComment(TEXT("???"));
	    }
	}
      else
	{
	  InfoBoxes[i]->SetComment(TEXT("---"));
	}
      if (IsFlarmTargetCNInRange())
	{
	  InfoBoxes[i]->SetColorBottom(2);
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(1);
	}

      break;
	// VENTA3 wind speed + bearing bottom line
	case 25:
		if ( CALCULATED_INFO.WindBearing == 0 )
                  _stprintf(sTmp,_T("0%s"),_T(DEG)); else
                  _stprintf(sTmp,_T("%1.0d%s"),(int)CALCULATED_INFO.WindBearing,_T(DEG));
		InfoBoxes[i]->SetComment(sTmp);
		break;

	// VENTA3 radial
	case 60:
		if ( HomeWaypoint == -1 ) {  // should be redundant
      			InfoBoxes[i]->SetComment(TEXT(""));
			break;
		}
		if ( CALCULATED_INFO.HomeRadial == 0 )
                  _stprintf(sTmp,_T("0%s"),_T(DEG)); else
                  _stprintf(sTmp,_T("%1.0d%s"),(int)CALCULATED_INFO.HomeRadial,_T(DEG));
		InfoBoxes[i]->SetComment(sTmp);
		break;

	// VENTA3 battery temperature under voltage. There is a good reason to see the temperature,
	// if available: many PNA/PDA will switch OFF during flight under direct sunlight for several
	// hours due to battery temperature too high!! The 314 does!

	// TODO: check temperature too high and set a warning flag to be used by an event or something
	#if (WINDOWSPC<1)
	case 65:
		if ( PDABatteryTemperature >0 ) {
                  _stprintf(sTmp,_T("%1.0d%SC"),(int)PDABatteryTemperature,_T(DEG));
			InfoBoxes[i]->SetComment(sTmp);
		} else
      			InfoBoxes[i]->SetComment(TEXT(""));
		break;
	#endif

	// VENTA3 alternates
	case 67:
	case 68:
	case 69:
		if ( ActiveAlternate == -1 ) {  // should be redundant
      			InfoBoxes[i]->SetComment(TEXT(""));
			break;
		}
		if (FlipBoxValue == true) {
			Units::FormatUserDistance(WayPointCalc[ActiveAlternate].Distance,
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
			InfoBoxes[i]->SetComment(sTmp);
		} else {
			Units::FormatUserArrival(WayPointCalc[ActiveAlternate].AltArriv,
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
			InfoBoxes[i]->SetComment(sTmp);
		}
		break;
	case 70: // QFE
		/*
		 // Showing the diff value offset was just interesting ;-)
		if (FlipBoxValue == true) {
			//Units::FormatUserArrival(QFEAltitudeOffset,
			Units::FormatUserAltitude(QFEAltitudeOffset,
				 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
			InfoBoxes[i]->SetComment(sTmp);
		} else {
		*/
		//Units::FormatUserArrival(GPS_INFO.Altitude,
		Units::FormatUserAltitude(GPS_INFO.Altitude,
			 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
		InfoBoxes[i]->SetComment(sTmp);
		break;

    default:
      InfoBoxes[i]->SetComment(TEXT(""));
    };

    DisplayTypeLast[i] = DisplayType[i];

  }
  InfoBoxLayout::Paint();

  first = false;

  UnlockNavBox();

}

#include "winbase.h"


void CommonProcessTimer()
{

  // service the GCE and NMEA queue
  if (ProgramStarted==psNormalOp) {
    InputEvents::DoQueuedEvents();
    if (RequestAirspaceWarningDialog) {
      DisplayTimeOut=0;
      RequestAirspaceWarningDialog= false;
      dlgAirspaceWarningShowDlg(RequestAirspaceWarningForce);
      RequestAirspaceWarningForce = false;
    }
    // update FLARM display (show/hide)
    GaugeFLARM::Show();
  }

#if (WINDOWSPC<1)
  SystemIdleTimerReset();
#endif

  if(InfoWindowActive)
    {
      if (!dlgAirspaceWarningVisible()) {
	// JMW prevent switching to map window if in airspace warning dialog

	if(InfoBoxFocusTimeOut >= FOCUSTIMEOUTMAX)
	  {
	    SwitchToMapWindow();
	  }
	InfoBoxFocusTimeOut ++;
      }
    }

  if (DisplayLocked) {
    if(MenuTimeOut==MenuTimeoutMax) {
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
    // JMW don't let display timeout while a dialog is active,
    // but allow button presses to trigger redisplay
    if (DisplayTimeOut>1) {
      DisplayTimeOut=1;
    }
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
  // JMW: is done in the message function now.
  if (!dlgAirspaceWarningVisible()) {
    if (Message::Render()) {
      // turn screen on if blanked and receive a new message
      DisplayTimeOut=0;
    }
  }

#if (EXPERIMENTAL > 0)

  if (bsms.Poll()) {
    // turn screen on if blanked and receive a new message
    DisplayTimeOut = 0;
  }

#endif

  static int iheapcompact = 0;
  // called 2 times per second, compact heap every minute.
  iheapcompact++;
  if (iheapcompact == 120) {
    MyCompactHeaps();
    iheapcompact = 0;
  }
}

////////////////


int ConnectionProcessTimer(int itimeout) {
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

  if (!extGPSCONNECT) {
    // if gps is not connected, set navwarning to true so
    // calculations flight timers don't get updated
    LockFlightData();
    GPS_INFO.NAVWarning = true;
    UnlockFlightData();
  }

  GPSCONNECT = FALSE;
  BOOL navwarning = (BOOL)(GPS_INFO.NAVWarning);

  if (gpsconnect && navwarning) {
    // If GPS connected but no lock, must be in hangar
    if (InterfaceTimeoutCheck()) {
#ifdef GNAV
      // TODO feature: ask question about shutdown or give warning
      // then shutdown if no activity.
      //     Shutdown();
#endif
    }
  }

  if((gpsconnect == FALSE) && (LastGPSCONNECT == FALSE))
    {
      // re-draw screen every five seconds even if no GPS
      TriggerGPSUpdate();

      devLinkTimeout(devAll());

      if(LOCKWAIT == TRUE)
	{
	  // gps was waiting for fix, now waiting for connection
	  LOCKWAIT = FALSE;
	}
      if(!CONNECTWAIT)
	{
	  // gps is waiting for connection first time

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
	  TriggerGPSUpdate();
	  CONNECTWAIT = FALSE;
	}
    }

  if((gpsconnect == TRUE) && (LastGPSCONNECT == TRUE))
    {
      if((navwarning == TRUE) && (LOCKWAIT == FALSE))
	{
	  InputEvents::processGlideComputer(GCE_GPS_FIX_WAIT);

	  TriggerGPSUpdate();

	  LOCKWAIT = TRUE;
#ifndef DISABLEAUDIO
	  MessageBeep(MB_ICONEXCLAMATION);
#endif
	  FullScreen();

	}
      else if((navwarning == FALSE) && (LOCKWAIT == TRUE))
	{
	  TriggerGPSUpdate();
	  LOCKWAIT = FALSE;
	}
    }

  LastGPSCONNECT = gpsconnect;
  return itimeout;
}


#ifndef _SIM_
void ProcessTimer(void)
{

  if (!GPSCONNECT && (DisplayTimeOut==0)) {
    // JMW 20071207
    // re-draw screen every five seconds even if no GPS
    // this prevents sluggish screen when inside hangar..
    TriggerGPSUpdate();
    DisplayTimeOut=1;
  }

  CommonProcessTimer();

  // now check GPS status
  devTick();

  static int itimeout = -1;
  itimeout++;

  // also service replay logger
  ReplayLogger::Update();
  if (ReplayLogger::IsEnabled()) {
    static double timeLast = 0;
    if (GPS_INFO.Time-timeLast>=1.0) {
      TriggerGPSUpdate();
    }
    timeLast = GPS_INFO.Time;
    GPSCONNECT = TRUE;
    extGPSCONNECT = TRUE;
    GPS_INFO.NAVWarning = FALSE;
    GPS_INFO.SatellitesUsed = 6;
    return;
  }

  if (itimeout % 10 == 0) {
    // check connection status every 5 seconds
    itimeout = ConnectionProcessTimer(itimeout);
  }
}
#endif // end processing of non-simulation mode


#ifdef _SIM_
void SIMProcessTimer(void)
{

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
    FindLatitudeLongitude(GPS_INFO.Latitude, GPS_INFO.Longitude,
                          GPS_INFO.TrackBearing, GPS_INFO.Speed*1.0,
                          &GPS_INFO.Latitude,
                          &GPS_INFO.Longitude);
    GPS_INFO.Time+= 1.0;
    long tsec = (long)GPS_INFO.Time;
    GPS_INFO.Hour = tsec/3600;
    GPS_INFO.Minute = (tsec-GPS_INFO.Hour*3600)/60;
    GPS_INFO.Second = (tsec-GPS_INFO.Hour*3600-GPS_INFO.Minute*60);

    UnlockFlightData();
  }

  if (i%2==0) return;

#ifndef NDEBUG
  // use this to test FLARM parsing/display
#ifndef GNAV
  NMEAParser::TestRoutine(&GPS_INFO);
#endif
#endif

  TriggerGPSUpdate();
}
#endif


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
  dlgAnalysisShowModal();
  DialogActive = false;
}


void PopupWaypointDetails()
{
  dlgWayPointDetailsShowModal();
}


void PopupBugsBallast(int UpDown)
{
	(void)UpDown;
  DialogActive = true;
  //  ShowWindow(hWndCB,SW_HIDE);
  FullScreen();
  SwitchToMapWindow();
  DialogActive = false;
}


void PopUpSelect(int Index)
{
  DialogActive = true;
  CurrentInfoType = InfoType[Index];
  StoreType(Index, InfoType[Index]);
  //  ShowWindow(hWndCB,SW_HIDE);
  FullScreen();
  SwitchToMapWindow();
  DialogActive = false;
}

#include <stdio.h>

void DebugStore(const char *Str, ...)
{
#if !defined(NDEBUG) && !defined(GNAV)
  char buf[MAX_PATH];
  va_list ap;
  int len;

  va_start(ap, Str);
  len = vsprintf(buf, Str, ap);
  va_end(ap);

  LockFlightData();
  FILE *stream;
  TCHAR szFileName[] = TEXT("xcsoar-debug.log");
  static bool initialised = false;
  if (!initialised) {
    initialised = true;
    stream = _wfopen(szFileName,TEXT("w"));
  } else {
    stream = _wfopen(szFileName,TEXT("a+"));
  }

  fwrite(buf,len,1,stream);

  fclose(stream);
  UnlockFlightData();
#endif
}


void StartupStore(const TCHAR *Str, ...)
{
  TCHAR buf[MAX_PATH];
  va_list ap;

  va_start(ap, Str);
  _vstprintf(buf, Str, ap);
  va_end(ap);

  if (csFlightDataInitialized) {
    LockFlightData();
  }
  FILE *startupStoreFile = NULL;
  static TCHAR szFileName[MAX_PATH];
  static bool initialised = false;
  if (!initialised) {
#ifdef GNAV
    LocalPath(szFileName, TEXT("persist/xcsoar-startup.log"));
#else
    LocalPath(szFileName, TEXT("xcsoar-startup.log"));
#endif
    startupStoreFile = _tfopen(szFileName, TEXT("wb"));
    if (startupStoreFile) {
      fclose(startupStoreFile);
    }
    initialised = true;
  }
  startupStoreFile = _tfopen(szFileName, TEXT("ab+"));
  if (startupStoreFile != NULL) {
    fprintf(startupStoreFile, "%S", buf);
    fclose(startupStoreFile);
  }
  if (csFlightDataInitialized) {
    UnlockFlightData();
  }
}


void LockNavBox() {
}

void UnlockNavBox() {
}

static int csCount_TaskData = 0;
static int csCount_FlightData = 0;
static int csCount_EventQueue = 0;

void LockTaskData() {
#ifdef HAVEEXCEPTIONS
  if (!csTaskDataInitialized) throw TEXT("LockTaskData Error");
#endif
  EnterCriticalSection(&CritSec_TaskData);
  csCount_TaskData++;
}

void UnlockTaskData() {
#ifdef HAVEEXCEPTIONS
  if (!csTaskDataInitialized) throw TEXT("LockTaskData Error");
#endif
  if (csCount_TaskData)
    csCount_TaskData--;
  LeaveCriticalSection(&CritSec_TaskData);
}


void LockFlightData() {
#ifdef HAVEEXCEPTIONS
  if (!csFlightDataInitialized) throw TEXT("LockFlightData Error");
#endif
  EnterCriticalSection(&CritSec_FlightData);
  csCount_FlightData++;
}

void UnlockFlightData() {
#ifdef HAVEEXCEPTIONS
  if (!csFlightDataInitialized) throw TEXT("LockFlightData Error");
#endif
  if (csCount_FlightData)
    csCount_FlightData--;
  LeaveCriticalSection(&CritSec_FlightData);
}

void LockTerrainDataCalculations() {
#ifdef HAVEEXCEPTIONS
  if (!csTerrainDataCalculationsInitialized) throw TEXT("LockTerrainDataCalculations Error");
#endif
  EnterCriticalSection(&CritSec_TerrainDataCalculations);
}

void UnlockTerrainDataCalculations() {
#ifdef HAVEEXCEPTIONS
  if (!csTerrainDataCalculationsInitialized) throw TEXT("LockTerrainDataCalculations Error");
#endif
  LeaveCriticalSection(&CritSec_TerrainDataCalculations);
}

void LockTerrainDataGraphics() {
#ifdef HAVEEXCEPTIONS
  if (!csTerrainDataGraphicsInitialized) throw TEXT("LockTerrainDataGraphics Error");
#endif
  EnterCriticalSection(&CritSec_TerrainDataGraphics);
}

void UnlockTerrainDataGraphics() {
#ifdef HAVEEXCEPTIONS
  if (!csTerrainDataGraphicsInitialized) throw TEXT("LockTerrainDataGraphics Error");
#endif
  LeaveCriticalSection(&CritSec_TerrainDataGraphics);
}


void LockEventQueue() {
#ifdef HAVEEXCEPTIONS
  if (!csEventQueueInitialized) throw TEXT("LockEventQueue Error");
#endif
  EnterCriticalSection(&CritSec_EventQueue);
  csCount_EventQueue++;
}

void UnlockEventQueue() {
#ifdef HAVEEXCEPTIONS
  if (!csEventQueueInitialized) throw TEXT("LockEventQueue Error");
#endif
  if (csCount_EventQueue)
    csCount_EventQueue--;
  LeaveCriticalSection(&CritSec_EventQueue);
}



void HideInfoBoxes() {
  int i;
  InfoBoxesHidden = true;
  for (i=0; i<numInfoWindows+1; i++) {
    InfoBoxes[i]->SetVisible(false);
  }
}


void ShowInfoBoxes() {
  int i;
  InfoBoxesHidden = false;
  for (i=0; i<numInfoWindows; i++) {
    InfoBoxes[i]->SetVisible(true);
  }
}



/////////////////////

#if (WINDOWSPC<1)
#ifndef GNAV
DWORD GetBatteryInfo(BATTERYINFO* pBatteryInfo)
{
    // set default return value
    DWORD result = 0;

    // check incoming pointer
    if(NULL == pBatteryInfo)
    {
        return 0;
    }

    SYSTEM_POWER_STATUS_EX2 sps;

    // request the power status
    result = GetSystemPowerStatusEx2(&sps, sizeof(sps), TRUE);

    // only update the caller if the previous call succeeded
    if(0 != result)
    {
        pBatteryInfo->acStatus = sps.ACLineStatus;
        pBatteryInfo->chargeStatus = sps.BatteryFlag;
        pBatteryInfo->BatteryLifePercent = sps.BatteryLifePercent;
	// VENTA get everything ready for PNAs battery control
	pBatteryInfo->BatteryVoltage = sps.BatteryVoltage;
	pBatteryInfo->BatteryAverageCurrent = sps.BatteryAverageCurrent;
	pBatteryInfo->BatteryCurrent = sps.BatteryCurrent;
	pBatteryInfo->BatterymAHourConsumed = sps.BatterymAHourConsumed;
	pBatteryInfo->BatteryTemperature = sps.BatteryTemperature;
    }

    return result;
}
#endif
#endif

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

int PDABatteryPercent = 100;
int PDABatteryTemperature = 0;

void BlankDisplay(bool doblank) {

#if (WINDOWSPC>0)
  return;
#else
#ifdef GNAV
  return;
#else
  static bool oldblank = false;

  BATTERYINFO BatteryInfo;
  BatteryInfo.acStatus = 0; // JMW initialise

  if (GetBatteryInfo(&BatteryInfo)) {
    PDABatteryPercent = BatteryInfo.BatteryLifePercent;
    PDABatteryTemperature = BatteryInfo.BatteryTemperature; // VENTA3
/*
	// All you need to display extra Battery informations...

	TCHAR vtemp[1000];
	_stprintf(vtemp,_T("Battpercent=%d Volt=%d Curr=%d AvCurr=%d mAhC=%d Temp=%d Lifetime=%d Fulllife=%d\n"),
 		BatteryInfo.BatteryLifePercent, BatteryInfo.BatteryVoltage,
 		BatteryInfo.BatteryCurrent, BatteryInfo.BatteryAverageCurrent,
 		BatteryInfo.BatterymAHourConsumed,
		BatteryInfo.BatteryTemperature, BatteryInfo.BatteryLifeTime, BatteryInfo.BatteryFullLifeTime);
	StartupStore( vtemp );
 */
  }

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

    // TODO feature: Trigger a GCE (Glide Computer Event) when
    // switching to battery mode This can be used to warn users that
    // power has been lost and you are now on battery power - ie:
    // something else is wrong

    if (doblank) {

      /* Battery status - simulator only - for safety of battery data
         note: Simulator only - more important to keep running in your plane
      */

      // JMW, maybe this should be active always...
      // we don't want the PDA to be completely depleted.

      if (BatteryInfo.acStatus==0) {
#ifdef _SIM_
        if ((PDABatteryPercent < BATTERY_EXIT) && !ForceShutdown) {
          StartupStore(TEXT("Battery low exit...\n"));
          // TODO feature: Warning message on battery shutdown
	  ForceShutdown = true;
          SendMessage(hWndMainWindow, WM_CLOSE, 0, 0);
        } else
#endif
          if (PDABatteryPercent < BATTERY_WARNING) {
            DWORD LocalWarningTime = ::GetTickCount();
            if ((LocalWarningTime - BatteryWarningTime) > BATTERY_REMINDER) {
              BatteryWarningTime = LocalWarningTime;
              // TODO feature: Show the user what the batt status is.
              DoStatusMessage(TEXT("Organiser Battery Low"));
            }
          } else {
            BatteryWarningTime = 0;
          }
      }

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
#endif
#endif
}



void Event_SelectInfoBox(int i) {
//  int oldinfofocus = InfoFocus;

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
  int j=0, k;

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

  // TODO code: if i==0, go to default or reset

  setInfoType(InfoFocus, j);
  DisplayText();

}



static void ReplaceInString(TCHAR *String, TCHAR *ToReplace,
                            TCHAR *ReplaceWith, size_t Size){
  TCHAR TmpBuf[MAX_PATH];
  int   iR, iW;
  TCHAR *pC;

  while((pC = _tcsstr(String, ToReplace)) != NULL){
    iR = _tcsclen(ToReplace);
    iW = _tcsclen(ReplaceWith);
    _tcscpy(TmpBuf, pC + iR);
    _tcscpy(pC, ReplaceWith);
    _tcscat(pC, TmpBuf);
  }

}

static void CondReplaceInString(bool Condition, TCHAR *Buffer,
                                TCHAR *Macro, TCHAR *TrueText,
                                TCHAR *FalseText, size_t Size){
  if (Condition)
    ReplaceInString(Buffer, Macro, TrueText, Size);
  else
    ReplaceInString(Buffer, Macro, FalseText, Size);
}

bool ExpandMacros(const TCHAR *In, TCHAR *OutBuffer, size_t Size){
  // ToDo, check Buffer Size
  bool invalid = false;
  _tcsncpy(OutBuffer, In, Size);
  OutBuffer[Size-1] = '\0';

  if (_tcsstr(OutBuffer, TEXT("$(")) == NULL) return false;

  if (TaskAborted) {
    if (_tcsstr(OutBuffer, TEXT("$(WaypointNext)"))) {
      // Waypoint\nNext
      invalid = !ValidTaskPoint(ActiveWayPoint+1);
      CondReplaceInString(!ValidTaskPoint(ActiveWayPoint+2),
                          OutBuffer,
                          TEXT("$(WaypointNext)"),
                          TEXT("Landpoint\nFurthest"),
                          TEXT("Landpoint\nNext"), Size);

    } else
    if (_tcsstr(OutBuffer, TEXT("$(WaypointPrevious)"))) {
      // Waypoint\nNext
      invalid = !ValidTaskPoint(ActiveWayPoint-1);
      CondReplaceInString(!ValidTaskPoint(ActiveWayPoint-2),
                          OutBuffer,
                          TEXT("$(WaypointPrevious)"),
                          TEXT("Landpoint\nClosest"),
                          TEXT("Landpoint\nPrevious"), Size);
    }
  } else {
    if (_tcsstr(OutBuffer, TEXT("$(WaypointNext)"))) {
      // Waypoint\nNext
      invalid = !ValidTaskPoint(ActiveWayPoint+1);
      CondReplaceInString(!ValidTaskPoint(ActiveWayPoint+2),
                          OutBuffer,
                          TEXT("$(WaypointNext)"),
                          TEXT("Waypoint\nFinish"),
                          TEXT("Waypoint\nNext"), Size);

    } else
    if (_tcsstr(OutBuffer, TEXT("$(WaypointPrevious)"))) {
      if (ActiveWayPoint==1) {
        invalid = !ValidTaskPoint(ActiveWayPoint-1);
        ReplaceInString(OutBuffer, TEXT("$(WaypointPrevious)"),
                        TEXT("Waypoint\nStart"), Size);
      } else if (EnableMultipleStartPoints) {
        invalid = !ValidTaskPoint(0);
        CondReplaceInString((ActiveWayPoint==0),
                            OutBuffer,
                            TEXT("$(WaypointPrevious)"),
                            TEXT("StartPoint\nCycle"), TEXT("Waypoint\nPrevious"), Size);
      } else {
        invalid = (ActiveWayPoint<=0);
        ReplaceInString(OutBuffer, TEXT("$(WaypointPrevious)"), TEXT("Waypoint\nPrevious"), Size);
      }
    }
  }

  if (_tcsstr(OutBuffer, TEXT("$(AdvanceArmed)"))) {
    switch (AutoAdvance) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(AdvanceArmed)"), TEXT("(manual)"), Size);
      invalid = true;
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(AdvanceArmed)"), TEXT("(auto)"), Size);
      invalid = true;
      break;
    case 2:
      if (ActiveWayPoint>0) {
        if (ValidTaskPoint(ActiveWayPoint+1)) {
          CondReplaceInString(AdvanceArmed, OutBuffer, TEXT("$(AdvanceArmed)"),
                              TEXT("Cancel"), TEXT("TURN"), Size);
        } else {
          ReplaceInString(OutBuffer, TEXT("$(AdvanceArmed)"),
                          TEXT("(finish)"), Size);
          invalid = true;
        }
      } else {
        CondReplaceInString(AdvanceArmed, OutBuffer, TEXT("$(AdvanceArmed)"),
                            TEXT("Cancel"), TEXT("START"), Size);
      }
      break;
    case 3:
      if (ActiveWayPoint==0) {
        CondReplaceInString(AdvanceArmed, OutBuffer, TEXT("$(AdvanceArmed)"),
                            TEXT("Cancel"), TEXT("START"), Size);
      } else if (ActiveWayPoint==1) {
        CondReplaceInString(AdvanceArmed, OutBuffer, TEXT("$(AdvanceArmed)"),
                            TEXT("Cancel"), TEXT("RESTART"), Size);
      } else {
        ReplaceInString(OutBuffer, TEXT("$(AdvanceArmed)"), TEXT("(auto)"), Size);
        invalid = true;
      }
      // TODO bug: no need to arm finish
    default:
      break;
    }
  }

  if (_tcsstr(OutBuffer, TEXT("$(CheckReplay)"))) {
    if (!ReplayLogger::IsEnabled() && GPS_INFO.MovementDetected) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckReplay)"), TEXT(""), Size);
  }

  if (_tcsstr(OutBuffer, TEXT("$(CheckWaypointFile)"))) {
    if (!ValidWayPoint(0)) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckWaypointFile)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckSettingsLockout)"))) {
#ifndef _SIM_
    if (LockSettingsInFlight && CALCULATED_INFO.Flying) {
      invalid = true;
    }
#endif
    ReplaceInString(OutBuffer, TEXT("$(CheckSettingsLockout)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckTaskResumed)"))) {
    if (TaskAborted) {
      // TODO code: check, does this need to be set with temporary task?
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckTaskResumed)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckTask)"))) {
    if (!ValidTaskPoint(ActiveWayPoint)) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckTask)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckAirspace)"))) {
    if (!ValidAirspace()) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckAirspace)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckFLARM)"))) {
    if (!GPS_INFO.FLARM_Available) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckFLARM)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckTerrain)"))) {
    if (!CALCULATED_INFO.TerrainValid) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckTerrain)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckAutoMc)"))) {
    if (!ValidTaskPoint(ActiveWayPoint)
        && ((AutoMcMode==0)
	    || (AutoMcMode==2))) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckAutoMc)"), TEXT(""), Size);
  }

  CondReplaceInString(LoggerActive, OutBuffer, TEXT("$(LoggerActive)"), TEXT("Stop"), TEXT("Start"), Size);

  if (_tcsstr(OutBuffer, TEXT("$(SnailTrailToggleName)"))) {
    switch(TrailActive) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(SnailTrailToggleName)"), TEXT("Long"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(SnailTrailToggleName)"), TEXT("Short"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, TEXT("$(SnailTrailToggleName)"), TEXT("Full"), Size);
      break;
    case 3:
      ReplaceInString(OutBuffer, TEXT("$(SnailTrailToggleName)"), TEXT("Off"), Size);
      break;
    }
  }
// VENTA3 VisualGlide
  if (_tcsstr(OutBuffer, TEXT("$(VisualGlideToggleName)"))) {
    switch(VisualGlide) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Steady"), Size);
      break;
    case 1:
	if (ExtendedVisualGlide)
		ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Moving"), Size);
	else
		ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Off"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Off"), Size);
      break;
    }
  }

// VENTA3 AirSpace event
  if (_tcsstr(OutBuffer, TEXT("$(AirSpaceToggleName)"))) {
    switch(OnAirSpace) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(AirSpaceToggleName)"), TEXT("ON"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(AirSpaceToggleName)"), TEXT("OFF"), Size);
      break;
    }
  }

  if (_tcsstr(OutBuffer, TEXT("$(TerrainTopologyToggleName)"))) {
    char val;
    val = 0;
    if (EnableTopology) val++;
    if (EnableTerrain) val += (char)2;
    switch(val) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(TerrainTopologyToggleName)"),
                      TEXT("Topology\nOn"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(TerrainTopologyToggleName)"),
                      TEXT("Terrain\nOn"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, TEXT("$(TerrainTopologyToggleName)"),
                      TEXT("Terrain\nTopology"), Size);
      break;
    case 3:
      ReplaceInString(OutBuffer, TEXT("$(TerrainTopologyToggleName)"),
                      gettext(TEXT("Terrain\nOff")), Size);
      break;
    }
  }

  //////

  CondReplaceInString(TaskIsTemporary(),
		      OutBuffer, TEXT("$(TaskAbortToggleActionName)"),
		      TEXT("Resume"), TEXT("Abort"), Size);

  if (_tcsstr(OutBuffer, TEXT("$(FinalForceToggleActionName)"))) {
    CondReplaceInString(ForceFinalGlide, OutBuffer,
                        TEXT("$(FinalForceToggleActionName)"),
                        TEXT("Unforce"),
                        TEXT("Force"), Size);
    if (AutoForceFinalGlide) {
      invalid = true;
    }
  }

  CondReplaceInString(MapWindow::IsMapFullScreen(), OutBuffer, TEXT("$(FullScreenToggleActionName)"), TEXT("Off"), TEXT("On"), Size);
  CondReplaceInString(MapWindow::isAutoZoom(), OutBuffer, TEXT("$(ZoomAutoToggleActionName)"), TEXT("Manual"), TEXT("Auto"), Size);
  CondReplaceInString(EnableTopology, OutBuffer, TEXT("$(TopologyToggleActionName)"), TEXT("Off"), TEXT("On"), Size);
  CondReplaceInString(EnableTerrain, OutBuffer, TEXT("$(TerrainToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  if (_tcsstr(OutBuffer, TEXT("$(MapLabelsToggleActionName)"))) {
    switch(MapWindow::DeclutterLabels) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(MapLabelsToggleActionName)"),
                      TEXT("MID"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(MapLabelsToggleActionName)"),
                      TEXT("OFF"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, TEXT("$(MapLabelsToggleActionName)"),
                      TEXT("ON"), Size);
      break;
    }
  }

  CondReplaceInString(CALCULATED_INFO.AutoMacCready != 0, OutBuffer, TEXT("$(MacCreadyToggleActionName)"), TEXT("Manual"), TEXT("Auto"), Size);
  CondReplaceInString(EnableAuxiliaryInfo, OutBuffer, TEXT("$(AuxInfoToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  CondReplaceInString(UserForceDisplayMode == dmCircling, OutBuffer, TEXT("$(DispModeClimbShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(UserForceDisplayMode == dmCruise, OutBuffer, TEXT("$(DispModeCruiseShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(UserForceDisplayMode == dmNone, OutBuffer, TEXT("$(DispModeAutoShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(UserForceDisplayMode == dmFinalGlide, OutBuffer, TEXT("$(DispModeFinalShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(AltitudeMode == ALLON, OutBuffer, TEXT("$(AirspaceModeAllShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == CLIP,  OutBuffer, TEXT("$(AirspaceModeClipShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == AUTO,  OutBuffer, TEXT("$(AirspaceModeAutoShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == ALLBELOW, OutBuffer, TEXT("$(AirspaceModeBelowShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == ALLOFF, OutBuffer, TEXT("$(AirspaceModeAllOffIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(TrailActive == 0, OutBuffer, TEXT("$(SnailTrailOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(TrailActive == 2, OutBuffer, TEXT("$(SnailTrailShortShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(TrailActive == 1, OutBuffer, TEXT("$(SnailTrailLongShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(TrailActive == 3, OutBuffer, TEXT("$(SnailTrailFullShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

// VENTA3 VisualGlide
  CondReplaceInString(VisualGlide == 0, OutBuffer, TEXT("$(VisualGlideOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(VisualGlide == 1, OutBuffer, TEXT("$(VisualGlideLightShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(VisualGlide == 2, OutBuffer, TEXT("$(VisualGlideHeavyShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
// VENTA3 AirSpace
  CondReplaceInString(OnAirSpace  == 0, OutBuffer, TEXT("$(AirSpaceOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(OnAirSpace  == 1, OutBuffer, TEXT("$(AirSpaceOnShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(EnableFLARMGauge != 0, OutBuffer, TEXT("$(FlarmDispToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  return invalid;
}

