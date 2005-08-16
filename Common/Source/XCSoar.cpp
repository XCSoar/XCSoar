/*
  XCSoar Glide Computer
  Copyright (C) 2000 - 2004  M Roberts

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

  $Id: XCSoar.cpp,v 1.64 2005/08/16 21:42:08 scottp Exp $
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

#include <commctrl.h>
#include <aygshell.h>
#include <sipapi.h>

#include "Terrain.h"
#include "VarioSound.h"
#include "device.h"
#include "devCAI302.h"
#include "devEW.h"
#include "Externs.h"
#include "units.h"

// Temporary version location (will be automatically generated)
extern TCHAR* XCSoar_Version = TEXT("5 ALPHA");

HWND hWnd1, hWnd2, hWnd3;

HINSTANCE                       hInst;                                  // The current instance
HWND                                    hWndCB;                                 // The command bar handle
HWND                                    hWndMainWindow; // Main Windows
HWND                                    hWndMapWindow;  // MapWindow
HWND          hWndMenuButton = NULL;

HWND                                    hWndCDIWindow = NULL; //CDI Window

HWND                                    hWndInfoWindow[NUMINFOWINDOWS];
HWND                                    hWndTitleWindow[NUMINFOWINDOWS];

int                                     InfoType[NUMINFOWINDOWS] = {921102,
                                                                    725525,
                                                                    262144,
                                                                    74518,
                                                                    657930,
                                                                    2236963,
                                                                    394758,
                                                                    1644825};

BOOL                                    DisplayLocked = TRUE;
BOOL                                    InfoWindowActive = TRUE;
int                                     FocusTimeOut = 0;
int                                     MenuTimeOut = 0;
int                                     DisplayTimeOut = 0;



HBRUSH hBrushSelected;
HBRUSH hBrushUnselected;
COLORREF ColorSelected = RGB(0xC0,0xC0,0xC0);
COLORREF ColorUnselected = RGB(0xFF,0xFF,0xFF);
COLORREF ColorWarning = RGB(0xFF,0x00,0x00);
COLORREF ColorOK = RGB(0x00,0x00,0xFF);

// Serial Port Globals

HANDLE                          hPort1 = INVALID_HANDLE_VALUE;    // Handle to the serial port
HANDLE                          hPort2 = INVALID_HANDLE_VALUE;    // Handle to the serial port
BOOL                                    Port1Available = NULL;
BOOL                                    Port2Available = NULL;

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
double                          QNH = (double)1013.2;


//SI to Local Units
double        SPEEDMODIFY = TOKNOTS;
double                          LIFTMODIFY  = TOKNOTS;
double                          DISTANCEMODIFY = TONAUTICALMILES;
double        ALTITUDEMODIFY = TOFEET; 

//Flight Data Globals
double        MCCREADY = 0;
bool          AutoMcCready = false;

int          NettoSpeed = 1000;

NMEA_INFO                       GPS_INFO;
DERIVED_INFO  CALCULATED_INFO;
BOOL GPSCONNECT = FALSE;
BOOL extGPSCONNECT = FALSE; // this one used by external functions

BOOL VARIOCONNECT = FALSE;
bool          TaskAborted = false;

bool InfoBoxesDirty= TRUE;

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
int FAISector = TRUE;
DWORD SectorRadius = 500;
int StartLine = TRUE;
DWORD StartRadius = 3000;

int HomeWaypoint = -1;

// Airspace Database
AIRSPACE_AREA *AirspaceArea = NULL;
AIRSPACE_POINT *AirspacePoint = NULL;
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
GetTextSTRUCT GetTextCache[MAXSTATUSMESSAGECACHE];
int GetTextCache_Size = 0;
StatusMessageSTRUCT StatusMessageCache[MAXSTATUSMESSAGECACHE];
int StatusMessageCache_Size = 0;
InputSTRUCT InputCache[MAXSTATUSMESSAGECACHE];
int InputCache_Size = 0;

//Snail Trial
SNAIL_POINT SnailTrail[TRAILSIZE];
int SnailNext = 0;
int TrailActive = TRUE;

// user interface settings
int CircleZoom = FALSE;
int WindUpdateMode = 0;
int EnableTopology = FALSE;
int EnableTerrain = FALSE;
int FinalGlideTerrain = FALSE;
int EnableSoundVario = TRUE;
int EnableSoundModes = TRUE;
int EnableSoundTask = TRUE;
int SoundVolume = 80;
int SoundDeadband = 5;

BOOL EnableAutoBlank = false;
bool ScreenBlanked = false;


//IGC Logger
BOOL LoggerActive = FALSE;

// Others
double FrameRate = 0;
int FrameCount = 0;
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
bool MenuActive = false;

//Task Information
Task_t Task = {{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0}};
int ActiveWayPoint = -1;

// Assigned Area Task
double AATTaskLength = 120;
BOOL AATEnabled = FALSE;


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

// Groups: 
//   Altitude 0,1,20,33
//   Aircraft info 3,6,23,32,37
//   LD 4,5,19,38
//   Vario 2,7,8,9,21,22,24,44
//   Wind 25,26
//   Mcready 10,34,35,43
//   Nav 11,12,13,15,16,17,18,27,28,29,30,31
//   Waypoint 14,36,39,40,41,42

SCREEN_INFO Data_Options[] = {
	  {TEXT("Height GPS"), TEXT("H GPS"), new InfoBoxFormatter(TEXT("%2.0f")), AltitudeProcessing, 1, 33}, 

	  // 1
	  {TEXT("Height AGL"), TEXT("H AGL"), new FormatterLowWarning(TEXT("%2.0f"),0.0), NoProcessing, 20, 0}, 

	  // 2
	  {TEXT("Thermal last 30 sec"), TEXT("TC 30s"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 7, 44}, 

	  // 3
	  {TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f°T")), NoProcessing, 6, 37}, 

	  // 4
	  {TEXT("L/D instantaneous"), TEXT("L/D Inst"), new InfoBoxFormatter(TEXT("%2.0f")), PopupBugsBallast, 5, 38}, 

	  // 5
	  {TEXT("L/D cruise"), TEXT("L/D Cru"), new InfoBoxFormatter(TEXT("%2.0f")), PopupBugsBallast, 19, 4}, 

	  // 6
	  {TEXT("Speed ground"), TEXT("V Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), SpeedProcessing, 23, 3}, 
	 
	  // 7
	  {TEXT("Last Thermal Average"), TEXT("TL Avg"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2}, 

	  // 8
	  {TEXT("Last Thermal Gain"), TEXT("TL Gain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 9, 7}, 

	  // 9
	  {TEXT("Last Thermal Time"), TEXT("TL Time"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 21, 8}, 

	  // 10
	  {TEXT("McCready Setting"), TEXT("McCready"), new InfoBoxFormatter(TEXT("%2.1f")), McCreadyProcessing, 34, 43}, 

	  // 11
	  {TEXT("Next Distance"), TEXT("WP Dist"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 12, 31}, 

	  // 12
	  {TEXT("Next Altitude Difference"), TEXT("WP AltD"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 13, 11}, 

	  // 13
	  {TEXT("Next Altitude Required"), TEXT("WP AltR"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 15, 12}, 

	  // 14
	  {TEXT("Next Waypoint"), TEXT("Next"), new FormatterWaypoint(TEXT("\0")), NextUpDown, 36, 42}, 
	 
	  // 15
	  {TEXT("Final Altitude Difference"), TEXT("Fin AltD"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 16, 13}, 

	  // 16
	  {TEXT("Final Altitude Required"), TEXT("Fin AltR"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 17, 15}, 

	  // 17
	  {TEXT("Speed Task Average"), TEXT("V Task"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16}, 

	  // 18
	  {TEXT("Final Distance"), TEXT("Fin Dis"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 27, 17}, 
	  
	  // 19
	  {TEXT("Final L/D"), TEXT("Fin L/D"), new InfoBoxFormatter(TEXT("%1.0f")), NoProcessing, 38, 5}, 

	  // 20
	  {TEXT("Terrain Elevation"), TEXT("H Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 33, 1}, 
	  
	  // 21
	  {TEXT("Thermal Average"), TEXT("TC Avg"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 22, 9}, 

	  // 22
	  {TEXT("Thermal Gain"), TEXT("TC Gain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 24, 21}, 
	 
	  // 23
	  {TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f°T")), DirectionProcessing, 32, 6}, 

	  // 24
	  {TEXT("Vario"), TEXT("Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 44, 22}, 

	  // 25
	  {TEXT("Wind Speed"), TEXT("Wind V"), new InfoBoxFormatter(TEXT("%2.0f")), WindSpeedProcessing, 26, 26}, 

	  // 26
	  {TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f°T")), WindDirectionProcessing, 25, 25}, 

	  // 27
	  {TEXT("AA Time"), TEXT("AA Time"), new FormatterTime(TEXT("%2.0f")), NoProcessing, 28, 18}, 

	  // 28
	  {TEXT("AA Distance Max"), TEXT("AA Dmax"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 29, 27}, 

	  // 29
	  {TEXT("AA Distance Min"), TEXT("AA Dmin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 30, 28}, 

	  // 30
	  {TEXT("AA Speed Max"), TEXT("AA Vmax"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 31, 29}, 

	  // 31
	  {TEXT("AA Speed Min"), TEXT("AA Vmin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 11, 30}, 

	  // 32
	  {TEXT("Airspeed IAS"), TEXT("V IAS"), new InfoBoxFormatter(TEXT("%2.0f")), AirspeedProcessing, 37, 23}, 

	  // 33
	  {TEXT("Pressure Altitude"), TEXT("H Baro"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 0, 20},

	  // 34
	  {TEXT("Speed MacReady"), TEXT("V Mc"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 35, 10}, 

	  // 35
	  {TEXT("Percentage climb"), TEXT("%% Climb"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 43, 34},

	  // 36
	  {TEXT("Time of flight"), TEXT("Time flt"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 39, 14},

	  // 37
	  {TEXT("G load"), TEXT("G"), new InfoBoxFormatter(TEXT("%2.2f")), AccelerometerProcessing, 3, 32},

	  // 38
	  {TEXT("Next L/D"), TEXT("WP L/D"), new InfoBoxFormatter(TEXT("%2.2f")), NoProcessing, 4, 19},

	  // 39
	  {TEXT("Time local"), TEXT("Time loc"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 40, 36},

	  // 40
	  {TEXT("Time UTC"), TEXT("Time UTC"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 41, 39},

	  // 41
	  {TEXT("Task Time To Go"), TEXT("Fin ETA"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 42, 40},

	  // 42
	  {TEXT("Next Time To Go"), TEXT("WP ETA"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 14, 41},

	  // 43
	  {TEXT("Speed Dolphin"), TEXT("V Opt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 10, 35}, 

	  // 44
	  {TEXT("Netto Vario"), TEXT("Netto"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 2, 24}, 

	};

int NUMSELECTSTRINGS = 45;

int ControlWidth, ControlHeight;
        
CRITICAL_SECTION  CritSec_FlightData;
CRITICAL_SECTION  CritSec_TerrainDataGraphics;
CRITICAL_SECTION  CritSec_TerrainDataCalculations;
CRITICAL_SECTION  CritSec_NavBox;


// Forward declarations of functions included in this code module:
ATOM                                                    MyRegisterClass (HINSTANCE, LPTSTR);
BOOL                                                    InitInstance    (HINSTANCE, int);
LRESULT CALLBACK        WndProc                 (HWND, UINT, WPARAM, LPARAM);
LRESULT                                         MainMenu(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void                                                    AssignValues(void);
void                                                    DisplayText(void);
void                                                    ReadAssetNumber(void);

void CommonProcessTimer    (void);
void ProcessTimer    (void);
void SIMProcessTimer(void);

void                                                    PopUpSelect(int i);
void                                                    SwitchToMapWindow(void);
HWND                                                    CreateRpCommandBar(HWND hwnd);

#ifdef DEBUG
void                                            DebugStore(char *Str);
#endif


extern BOOL GpsUpdated;
extern BOOL VarioUpdated;

void HideMenu() {
  // ignore this if the display isn't locked -- must keep menu visible
  if (DisplayLocked) {
    ShowWindow(hWndMenuButton, SW_HIDE);
    MenuTimeOut = MENUTIMEOUTMAX;
    DisplayTimeOut = 0;
  }
}

void ShowMenu() {
  MenuTimeOut = 0;
  ShowWindow(hWndMenuButton, SW_SHOW);
  DisplayTimeOut = 0;
}


//extern bool MapWindow::RequestMapDirty; // GUI asks for map window refresh
//extern bool MapWindow::MapDirty; // the actual map refresh trigger


#if (EXPERIMENTAL > 0)
#include "BlueSMS.h"
BlueDialupSMS bsms;
#endif


void ShowStatus() {
  TCHAR statusmessage[1000];
  TCHAR Temp[1000];
  int iwaypoint= -1;
  double sunsettime;
  int sunsethours;
  int sunsetmins;
  double bearing;
  double distance;
  TCHAR sLongditude[16];
  TCHAR sLattitude[16];
  int   TabStops[] = {60,80,0};

  statusmessage[0]=0;

  Units::LongditudeToString(GPS_INFO.Longditude, sLongditude, sizeof(sLongditude)-1);
  Units::LattitudeToString(GPS_INFO.Lattitude, sLattitude, sizeof(sLattitude)-1);

  sunsettime = DoSunEphemeris(GPS_INFO.Longditude,
                              GPS_INFO.Lattitude);
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime-sunsethours)*60);

  _stprintf(Temp,TEXT("%s\t%s\r\n%s\t%s\r\n%s\t%.0f %s\r\n%s\t%02d:%02d\r\n\r\n"),
		   gettext(TEXT("Longitude")),
           sLongditude,
		   gettext(TEXT("Latitude")),
           sLattitude,
		   gettext(TEXT("Altitude")),
           GPS_INFO.Altitude*ALTITUDEMODIFY,
            Units::GetAltitudeName(),
		   gettext(TEXT("Sunset")),
           sunsethours,
           sunsetmins
           );
  _tcscat(statusmessage, Temp);

  iwaypoint = FindNearestWayPoint(GPS_INFO.Longditude,
                                  GPS_INFO.Lattitude,
                                  100000.0); // big range limit
  if (iwaypoint>=0) {

    bearing = Bearing(GPS_INFO.Lattitude,
                      GPS_INFO.Longditude,
                      WayPointList[iwaypoint].Lattitude,
                      WayPointList[iwaypoint].Longditude);

    distance = Distance(GPS_INFO.Lattitude,
                        GPS_INFO.Longditude,
                        WayPointList[iwaypoint].Lattitude,
                        WayPointList[iwaypoint].Longditude)*DISTANCEMODIFY;

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
      wcscat(statusmessage, gettext(TEXT("GPS 2D fix")));      
    } else {
      wcscat(statusmessage, gettext(TEXT("GPS 3D fix")));      
    }
    wcscat(statusmessage, TEXT("\r\n"));

    wsprintf(Temp,TEXT("%s\t%d\r\n"),
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

  ShowStatusMessage(statusmessage, 60000, 15, false, TabStops);
  // i think one minute is enough...

}


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
  MapWindow::RequestFastRefresh = true;
  InfoBoxesDirty = true;
}




void RestartCommPorts() {
  if(Port1Available)
    {
      Port1Available = FALSE; Port1Close (hPort1);
    }
  PortIndex1 = 0; SpeedIndex1 = 2; ReadPort1Settings(&PortIndex1,&SpeedIndex1);
  Port1Available = Port1Initialize (COMMPort[PortIndex1],dwSpeed[SpeedIndex1]);
  
  if(Port2Available)
    {
      Port2Available = FALSE; Port2Close (hPort2);
    }
  PortIndex2 = 0; SpeedIndex2 = 2; ReadPort2Settings(&PortIndex2,&SpeedIndex2);
  if (PortIndex1 != PortIndex2) {
    Port2Available = Port2Initialize (COMMPort[PortIndex2],dwSpeed[SpeedIndex2]);
  } else {
    Port2Available = FALSE;
  }

  GpsUpdated = TRUE;
}


void FocusOnWindow(int i, bool selected) {
    //hWndTitleWindow
  HWND wind = hWndInfoWindow[i];

  if (selected) {
    SetWindowLong(wind,GWL_STYLE,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY|WS_BORDER);
  } else {
    SetWindowLong(wind,GWL_STYLE,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY);
  }

  wind = hWndTitleWindow[i];
  if (selected) {
    SetWindowLong(wind, GWL_USERDATA, 1);	  
  } else {
    SetWindowLong(wind, GWL_USERDATA, 0);
  }

}


DWORD CalculationThread (LPVOID lpvoid) {
  bool infoarrived;
  bool theinfoboxesaredirty;

  NMEA_INFO     tmp_GPS_INFO;
  DERIVED_INFO  tmp_CALCULATED_INFO;

  while (!MapWindow::CLOSETHREAD) {
    infoarrived = false;
    theinfoboxesaredirty = false;

    if (GpsUpdated) {
      infoarrived = true;
    }
    if (GPS_INFO.VarioAvailable && VarioUpdated) {
      infoarrived = true;
    }
    if (infoarrived) {
      
      // make local copy before editing...
      LockFlightData();
      memcpy(&tmp_GPS_INFO,&GPS_INFO,sizeof(NMEA_INFO));
      memcpy(&tmp_CALCULATED_INFO,&CALCULATED_INFO,sizeof(DERIVED_INFO));
      UnlockFlightData();

      // Do vario first to reduce audio latency
      if (GPS_INFO.VarioAvailable && VarioUpdated) {
        VarioUpdated = false;
        if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {
        }
        // assume new vario data has arrived, so infoboxes
        // need to be redrawn
        theinfoboxesaredirty = true;
      }

      if (GpsUpdated) {
        GpsUpdated = false;
        if(DoCalculations(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) 
          {
            theinfoboxesaredirty = true;
            MapWindow::RequestMapDirty = true;
          }        
      }

      // run the function anyway, because this gives audio functions
      // if no vario connected
      if (!GPS_INFO.VarioAvailable) {
        if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {
            theinfoboxesaredirty = true;
        }
      }

      
      if (theinfoboxesaredirty) {
        InfoBoxesDirty = true;
      }

      // values changed, so copy them back now: ONLY CALCULATED INFO
      // should be changed in DoCalculations, so we only need to write
      // that one back (otherwise we may write over new data)
      LockFlightData();
      memcpy(&CALCULATED_INFO,&tmp_CALCULATED_INFO,sizeof(DERIVED_INFO));
      UnlockFlightData();
      
    } else {
      Sleep(100); // sleep a while
    }

  }
  return 0;
}


void CreateCalculationThread() {
  HANDLE hCalculationThread;
  DWORD dwThreadID;

  // Create a read thread for performing calculations
  if (hCalculationThread = 
      CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE )CalculationThread, 0, 0, &dwThreadID))
  {
    SetThreadPriority(hCalculationThread, THREAD_PRIORITY_NORMAL); //THREAD_PRIORITY_ABOVE_NORMAL
    CloseHandle (hCalculationThread);
  }
}


int WINAPI WinMain(     HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPTSTR    lpCmdLine,
                        int       nCmdShow)
{
  MSG msg;
  HACCEL hAccelTable;
  INITCOMMONCONTROLSEX icc;

  // Registery (early)
  ReadRegistrySettings();

  // Interace (before interface)
  ReadLanguageFile();
  ReadStatusFile();
  ReadInputFile();

  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = ICC_UPDOWN_CLASS;
  InitCommonControls();
       
  
  // Perform application initialization:
  if (!InitInstance (hInstance, nCmdShow)) 
    {
      return FALSE;
    }

  CreateProgressDialog(gettext(TEXT("Initialising")));

  hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_XCSOAR);

  InitSineTable();

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
  InitializeCriticalSection(&CritSec_TerrainDataGraphics);
  InitializeCriticalSection(&CritSec_TerrainDataCalculations);

  memset( &(GPS_INFO), 0, sizeof(GPS_INFO));
  memset( &(CALCULATED_INFO), 0,sizeof(CALCULATED_INFO));
  memset( &SnailTrail[0],0,TRAILSIZE*sizeof(SNAIL_POINT));


#ifdef DEBUG
  DebugStore("# Start\r\n");
#endif

  // display start up screen
  //  StartupScreen();
  // not working very well at all
  
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

  OpenTopology();
  ReadTopology();

  VarioSound_Init();
  VarioSound_EnableSound(EnableSoundVario);
  VarioSound_SetVdead(SoundDeadband);
  VarioSound_SetV(0);

  VarioSound_SetSoundVolume(SoundVolume);

#ifndef _SIM_
  RestartCommPorts();
#endif

  cai302Register();
  ewRegister();
  // ... register all supported devices


  devInit(lpCmdLine);


  CreateCalculationThread();

  MapWindow::CreateDrawingThread();

#if (EXPERIMENTAL > 0)
  CreateProgressDialog(gettext(TEXT("Bluetooth dialup SMS")));
  bsms.Initialise();
#endif

  CreateProgressDialog(gettext(TEXT("Initialising display")));

  // just about done....

  DoSunEphemeris(147.0,-36.0);

#ifdef _SIM_
  DoStatusMessage(TEXT("Simulation\r\nNothing is real!"));
#else
  DoStatusMessage(TEXT("Maintain effective\r\nLOOKOUT at all times"));
#endif

  SwitchToMapWindow();
  MapWindow::MapDirty = true;

//  CloseProgressDialog();

 
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
  wc.cbWndExtra                 = dc.cbWndExtra ;
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
  wc.cbWndExtra = dc.cbWndExtra ;
  wc.hInstance = hInstance;
  wc.hIcon = (HICON)NULL;
  wc.hCursor = NULL;
  wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = TEXT("MapWindowClass");

  MapWindow::RequestMapDirty = true;

  return RegisterClass(&wc);

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
  LOGFONT logfont;
  int i;
  int FontHeight, FontWidth;
  int TitleHeight;

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

  MyRegisterClass(hInst, szWindowClass);

  RECT WindowSize;

  WindowSize.left = 0;
  WindowSize.top = 0;
  WindowSize.right = GetSystemMetrics(SM_CXSCREEN);
  WindowSize.bottom = GetSystemMetrics(SM_CYSCREEN);

  #ifdef SCREENWIDTH
    WindowSize.right = SCREENWIDTH;
    WindowSize.left = (GetSystemMetrics(SM_CXSCREEN) - SCREENWIDTH) / 2;
  #endif
  #ifdef SCREENHEIGHT
    WindowSize.bottom = SCREENHEIGHT;
    WindowSize.top = (GetSystemMetrics(SM_CYSCREEN) - SCREENHEIGHT) / 2;
  #endif


  hWndMainWindow = CreateWindow(szWindowClass, szTitle, WS_VISIBLE | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                WindowSize.left, WindowSize.top, WindowSize.right, WindowSize.bottom,
                                NULL, NULL, hInstance, NULL);

  SendMessage(hWndMainWindow, WM_SETICON,
	      (WPARAM)ICON_BIG, (LPARAM)IDI_XCSOARSWIFT);
  SendMessage(hWndMainWindow, WM_SETICON,
	      (WPARAM)ICON_SMALL, (LPARAM)IDI_XCSOARSWIFT);

  if (!hWndMainWindow)
    {   
      return FALSE;
    }

  hBrushSelected = (HBRUSH)CreateSolidBrush(ColorSelected);
  hBrushUnselected = (HBRUSH)CreateSolidBrush(ColorUnselected);

  GetClientRect(hWndMainWindow, &rc);

  FontHeight = (rc.bottom - rc.top ) / FONTHEIGHTRATIO;

  if ((rc.right - rc.left ) < (rc.bottom - rc.top ))
    FontWidth = (rc.right - rc.left ) / FONTWIDTHRATIO;

  else 

    FontWidth = 0;  // todo sgi

        
  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));

  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = FontHeight;
  logfont.lfWidth =  FontWidth;
  logfont.lfWeight = FW_BOLD;
  logfont.lfCharSet = ANSI_CHARSET;
#ifndef NOCLEARTYPE
  logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY; // JMW
#endif

  InfoWindowFont = CreateFontIndirect (&logfont);

  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight/TITLEFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth/TITLEFONTWIDTHRATIO);
  logfont.lfWeight = FW_BOLD;

#ifndef NOCLEARTYPE
  logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY; // JMW
#endif

  TitleWindowFont = CreateFontIndirect (&logfont);

  memset ((char *)&logfont, 0, sizeof (logfont));

  // new font for CDI Scale
        
  logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*CDIFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*CDIFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;

#ifndef NOCLEARTYPE
  logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY; // JMW
#endif

  CDIWindowFont = CreateFontIndirect (&logfont);




  // new font for map labels
  memset ((char *)&logfont, 0, sizeof (logfont));

        
  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*MAPFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*MAPFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;

#ifndef NOCLEARTYPE
  logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY; // JMW
#endif

  MapLabelFont = CreateFontIndirect (&logfont);


  // Font for map other text
  memset ((char *)&logfont, 0, sizeof (logfont));

  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*STATISTICSFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*STATISTICSFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;

#ifndef NOCLEARTYPE
  logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY; // JMW
#endif

  StatisticsFont = CreateFontIndirect (&logfont);

  // new font for map labels
        
  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*MAPFONTHEIGHTRATIO*1.3);
  logfont.lfWidth =  (int)(FontWidth*MAPFONTWIDTHRATIO*1.3);
  logfont.lfWeight = FW_MEDIUM;

#ifndef NOCLEARTYPE
  logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY; // JMW
#endif

  MapWindowFont = CreateFontIndirect (&logfont);

  SendMessage(hWndMapWindow,WM_SETFONT,
              (WPARAM)MapWindowFont,MAKELPARAM(TRUE,0));

  // Font for map bold text

  logfont.lfWeight = FW_BOLD;
  logfont.lfWidth =  (int)(FontWidth*MAPFONTWIDTHRATIO*1.3) +2;

  MapWindowBoldFont = CreateFontIndirect (&logfont);





  ////////

  GetClientRect(hWndMainWindow, &rc);

  ControlWidth = 2*(rc.right - rc.left) / NUMINFOWINDOWS;
  ControlHeight = (int)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
  TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO); 

#ifdef _MAP_
  ControlHeight = 0;
#endif


#ifndef _MAP_

  for(i=0;i<NUMINFOWINDOWS/2;i++)
    {
      if((i==0)&&0) // JMW why is this a special case?
        {
          hWndInfoWindow[i] = CreateWindow(TEXT("STATIC"),TEXT(""),WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY|WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                           i*ControlWidth, rc.top+TitleHeight,ControlWidth,ControlHeight,
                                           hWndMainWindow,NULL,hInstance,NULL);
        }
      else
        {
          hWndInfoWindow[i] = CreateWindow(TEXT("STATIC"),TEXT("\0"),
                                           WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                           i*ControlWidth, rc.top+TitleHeight,
                                           ControlWidth,ControlHeight-TitleHeight,
                                           hWndMainWindow,NULL,hInstance,NULL);
        }



      hWndTitleWindow[i] = CreateWindow(TEXT("STATIC"),
                                        // Data_Options[InfoType[i]& 0xff].Title
                                        TEXT("\0")
                                        ,
                                        WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                        i*ControlWidth, rc.top, ControlWidth, TitleHeight,
                                        hWndMainWindow,NULL,hInstance,NULL);


      hWndInfoWindow[i+(NUMINFOWINDOWS/2)] = CreateWindow(TEXT("STATIC"),TEXT("\0"),
                                                          WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                                          i*ControlWidth, (rc.bottom - ControlHeight+TitleHeight),
                                                          ControlWidth,ControlHeight-TitleHeight,
                                                          hWndMainWindow,NULL,hInstance,NULL);

      hWndTitleWindow[i+(NUMINFOWINDOWS/2)] =
        CreateWindow(TEXT("STATIC"),
                     // Data_Options[InfoType[i+(NUMINFOWINDOWS/2)]& 0xff].Title,
                     TEXT("\0"),
                     WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                     i*ControlWidth, (rc.bottom - ControlHeight),
                     ControlWidth, TitleHeight,
                     hWndMainWindow,NULL,hInstance,NULL);
    }

  for(i=0;i<NUMINFOWINDOWS;i++)
    {
      SendMessage(hWndInfoWindow[i],WM_SETFONT,(WPARAM)InfoWindowFont,MAKELPARAM(TRUE,0));
      SendMessage(hWndTitleWindow[i],WM_SETFONT,(WPARAM)TitleWindowFont,MAKELPARAM(TRUE,0));
    }
#endif


  //// create map window

  MapWindow::MapRect.top = rc.top+ControlHeight;
  MapWindow::MapRect.left = rc.left;
  MapWindow::MapRect.bottom = rc.bottom-ControlHeight;
  MapWindow::MapRect.right = rc.right;

#ifdef _MAP_

  hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                               0, 0, (rc.right - rc.left), 
			       (rc.bottom-rc.top) ,
                               hWndMainWindow,NULL,hInstance,NULL);
#else

#ifdef OLDWINDOW
  hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                               0, rc.top + ControlHeight, (rc.right - rc.left), ((rc.bottom-rc.top) - (2*ControlHeight)),
                               hWndMainWindow,NULL,hInstance,NULL);
#else

  hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                               0, 0, (rc.right - rc.left), 
			       (rc.bottom-rc.top) ,
                               hWndMainWindow, NULL ,hInstance,NULL);

#endif
        
#endif


  hWndMenuButton = CreateWindow(TEXT("BUTTON"),gettext(TEXT("Menu")),WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                0, 0,0,0,hWndMainWindow/*hWndMainWindow*/,NULL,hInst,NULL);


  SendMessage(hWndMenuButton,WM_SETFONT,(WPARAM)TitleWindowFont,MAKELPARAM(TRUE,0));

  // JMW moved menu button to center, to make room for thermal indicator
  SetWindowPos(hWndMenuButton,HWND_TOP,(int)(rc.right-rc.left-ControlWidth*MENUBUTTONWIDTHRATIO)/2,
               (int)(ControlHeight+10),
               (int)(ControlWidth*MENUBUTTONWIDTHRATIO),
               (int)((rc.bottom - rc.top)/10),SWP_SHOWWINDOW);

  // start of new code for displaying CDI window

  // JMW changed layout a bit, deleted Waiting for GPS info text as it is misleading here

  hWndCDIWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),WS_VISIBLE|WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                               0,0,0,0,hWndMainWindow,NULL,hInst,NULL);
  SendMessage(hWndCDIWindow,WM_SETFONT,
              (WPARAM)CDIWindowFont,MAKELPARAM(TRUE,0));

  SetWindowPos(hWndCDIWindow,hWndMenuButton,
               (int)(ControlWidth*0.6),(int)(ControlHeight+1),
               (int)(ControlWidth*2.8),(int)(TitleHeight*1.4),SWP_SHOWWINDOW);
  // JMW also made it so it doesn't obscure airspace warnings

  // end of new code for drawing CDI window (see below for destruction of objects)

  /////////////

    ShowWindow(hWndCDIWindow, SW_HIDE);
    ShowWindow(hWndMenuButton, SW_HIDE);

    ShowWindow(hWndMainWindow, nCmdShow);
    UpdateWindow(hWndMainWindow);

    ShowInfoBoxes();

    for(i=0;i<NUMINFOWINDOWS;i++)
      {
        UpdateWindow(hWndInfoWindow[i]);
        UpdateWindow(hWndTitleWindow[i]);
      }
    
    return TRUE;
}


int getInfoType(int i) {
  if (CALCULATED_INFO.Circling == TRUE)
    return InfoType[i] & 0xff;
  else if (CALCULATED_INFO.FinalGlide == TRUE) {
    return (InfoType[i] >> 16) & 0xff;
  } else {
    return (InfoType[i] >> 8) & 0xff;
  }
}


void setInfoType(int i, char j) {
  if (CALCULATED_INFO.Circling == TRUE) {
    InfoType[i] &= 0xffff00;
    InfoType[i] += (j);
  } else if (CALCULATED_INFO.FinalGlide == TRUE) {
    InfoType[i] &= 0x00ffff;
    InfoType[i] += (j<<16);
  } else {
    InfoType[i] &= 0xff00ff;
    InfoType[i] += (j<<8);
  }
}


void DoInfoKey(int keycode) {
  int i;

  HideMenu();

  LockNavBox();
  i = getInfoType(InfoFocus);

  LockFlightData();
  Data_Options[i].Process(keycode);
  UnlockFlightData();

  UnlockNavBox();
  
  /*
  LockFlightData();
  AssignValues();
  DisplayText();
  UnlockFlightData();
  */
  InfoBoxesDirty = true;

  GpsUpdated = TRUE; // emulate update to trigger calculations

  FocusTimeOut = 0;
  DisplayTimeOut = 0;

}


// XXX Debounce modified to ignore input
bool Debounce() {
  static DWORD fpsTimeLast= -1;
  DWORD fpsTimeThis = ::GetTickCount();
  DWORD dT = fpsTimeThis-fpsTimeLast;
  
  DisplayTimeOut = 0;

  if (ScreenBlanked) {
    // prevent key presses working if screen is blanked,
    // so a key press just triggers turning the display on again
    return false; 
  }

  if (dT>500) {
    fpsTimeLast = fpsTimeThis;
    return true;
  } else {
    return false;
  }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int i;
  static bool lastpress = false;
  long wdata;

  switch (message) 
    {

    case WM_USER:
      DoStatusMessage(TEXT("Closest Airfield\r\nChanged!"));
	  break;


    case WM_ERASEBKGND:
      return TRUE; // JMW trying to reduce screen flicker

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
      break;

      break;
    case WM_CREATE:
      memset (&s_sai, 0, sizeof (s_sai));
      s_sai.cbSize = sizeof (s_sai);

#ifdef _SIM_
      iTimerID = SetTimer(hWnd,1000,250,NULL);
#else
      iTimerID = SetTimer(hWnd,1000,250,NULL);
#endif
                        
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
      if(InfoWindowActive) {
	
        if(DisplayLocked) {

	  FocusOnWindow(InfoFocus,true);
	  //	  ShowMenu();
        }
        else {
	  
	  FocusOnWindow(InfoFocus,true);
	  //	  ShowMenu();
        }
      } else {
	HideMenu();
        SetFocus(hWndMapWindow);
      }
      break;

    case WM_KEYUP:

	// XXX Temporary button code...
	// Move these "functions" to a separate sub, and effectively just remap.
	// int i;
	// for (i = 0; i < InputCache_Size; i++) {
	// }


	// XXX Temporary version that uses new remote function
      switch (wParam)
        {
        case VK_APP1:	// Toggle full screen view
		InputExecute(TEXT("fullscreen"), TEXT("toggle"));
		break;

        case VK_APP2:	// Toggle snail trail
		InputExecute(TEXT("snailtrail"), TEXT("toggle"));
		break;

        case VK_APP3:	// Vario sound on/off
		InputExecute(TEXT("variosound"), TEXT("toggle"));
		break;

        case VK_APP4:	// Add a marker to current location
		InputExecute(TEXT("marker"), TEXT(""));
		break;

        case VK_APP6:	// Show Menu
		InputExecute(TEXT("menubutton"), TEXT(""));
		break;

        case VK_UP :  // SCROLL UP (infobox mode)
          DoInfoKey(1);
          break;

        case VK_DOWN: // SCROLL DOWN (infobox mode)
         DoInfoKey(-1);
          break;

        case VK_RETURN:
        if (!Debounce()) break;

          DoInfoKey(0);
          break;

        case VK_LEFT: // SCROLL DOWN
          DoInfoKey(-2);
          break;

        case VK_RIGHT: // SCROLL DOWN
          DoInfoKey(2);
          break;
        }
      break;

    case WM_TIMER:
      FrameRate = (double)FrameCount/4;
#ifdef _SIM_
      SIMProcessTimer();
#else
      ProcessTimer();
#endif
      FrameCount = 0;

      break;

    case WM_INITMENUPOPUP:
      if(DisplayLocked)
        CheckMenuItem((HMENU) wParam,IDM_FILE_LOCK,MF_CHECKED|MF_BYCOMMAND);
      else
        CheckMenuItem((HMENU) wParam,IDM_FILE_LOCK,MF_UNCHECKED|MF_BYCOMMAND);

      if(LoggerActive)
        CheckMenuItem((HMENU) wParam,IDM_FILE_LOGGER,MF_CHECKED|MF_BYCOMMAND);
      else
        CheckMenuItem((HMENU) wParam,IDM_FILE_LOGGER,MF_UNCHECKED|MF_BYCOMMAND);
      break;

    case WM_CLOSE:
      if(iTimerID)
        KillTimer(hWnd,iTimerID);

      SaveSoundSettings();

      VarioSound_EnableSound(false);

      VarioSound_Close();


      devCloseAll();

#if (EXPERIMENTAL > 0)
      bsms.Shutdown();
#endif

      MapWindow::CloseDrawingThread();

      NumberOfWayPoints = 0; Task[0].Index = -1;  ActiveWayPoint = -1; AATEnabled = FALSE;
      NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0; NumberOfAirspaceCircles = 0;
      CloseTerrain();
      CloseTopology();

      if(Port1Available)
        Port1Close(hPort1);
      if (Port2Available)
        Port2Close(hPort2);

      DestroyWindow(hWndMapWindow);
      DestroyWindow(hWndMenuButton);
      DestroyWindow(hWndCDIWindow);
                        
      for(i=0;i<NUMINFOWINDOWS;i++)
        {
          DestroyWindow(hWndInfoWindow[i]);
          DestroyWindow(hWndTitleWindow[i]);
        }
      CommandBar_Destroy(hWndCB);
      for (i=0; i<NUMSELECTSTRINGS; i++) {
        delete Data_Options[i].Formatter;
      }

      DeleteObject(InfoWindowFont);
      DeleteObject(TitleWindowFont);
      DeleteObject(CDIWindowFont);
      DeleteObject(MapLabelFont);
      DeleteObject(MapWindowFont);
      DeleteObject(MapWindowBoldFont);
      DeleteObject(StatisticsFont);


      if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
      if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
      if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);

      CloseWayPoints();

      DestroyWindow(hWndMainWindow);

      DeleteCriticalSection(&CritSec_FlightData);
      DeleteCriticalSection(&CritSec_NavBox);
      DeleteCriticalSection(&CritSec_TerrainDataCalculations); 
      DeleteCriticalSection(&CritSec_TerrainDataGraphics); 

      CloseProgressDialog();

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

// Execute an Input - this will execute an input based on the TEXT input
void InputExecute(TCHAR* function, TCHAR* misc) {
	int i, j;

	// Debounce all buttons !
	if (!Debounce()) return;

	if (wcscmp(function, TEXT("fullscreen"))) {
		// XXX Check on/off/toggle
		MapWindow::RequestToggleFullScreen();
		return;

	} else if (wcscmp(function, TEXT("snailtrail"))) {

		// XXX on/off/long/toggle !!! (if on/off/long - only if change)

          if (!InfoWindowActive) {
            TrailActive ++;
            if (TrailActive>2) {
              TrailActive=0;
            }

            if (TrailActive==0)
              DoStatusMessage(TEXT("SnailTrail OFF"));
            if (TrailActive==1) 
              DoStatusMessage(TEXT("SnailTrail ON Long"));
            if (TrailActive==2) 
              DoStatusMessage(TEXT("SnailTrail ON Short"));
            return;
          }

          i = getInfoType(InfoFocus);

          j = Data_Options[i].next_screen;
          setInfoType(InfoFocus,j);

          AssignValues();
          DisplayText();

          FocusTimeOut = 0;

          return;

	} else if (wcscmp(function, TEXT("variosound"))) {

		// XXX on/off/toggle
          if (!InfoWindowActive) {
            EnableSoundVario = !EnableSoundVario;
            VarioSound_EnableSound((BOOL)EnableSoundVario);

            // ARH Let the user know what's happened
            if (EnableSoundVario)
              DoStatusMessage(TEXT("Vario Sounds ON"));
            else
              DoStatusMessage(TEXT("Vario Sounds OFF"));

            return;
          }

          i = getInfoType(InfoFocus);

          j = Data_Options[i].prev_screen;
          setInfoType(InfoFocus,j);

          AssignValues();
          DisplayText();

          FocusTimeOut = 0;

          return;

	} else if (wcscmp(function, TEXT("marker"))) {
          if (InfoWindowActive)
            return;

          LockFlightData();

          MarkLocation(GPS_INFO.Longditude, GPS_INFO.Lattitude);

          UnlockFlightData();

          // ARH Let the user know what's happened
          DoStatusMessage(TEXT("Dropped marker"));

          return;
	
	} else if (wcscmp(function, TEXT("menubutton"))) {
		ShowMenu();
		return;

	} else {
		// XXX Debugging
		DoStatusMessage(TEXT("Invalid Input"), function);
		return;

	}

	return;
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
      if(wmControl == hWndMenuButton)
        {
          wID = DialogBox(hInst, (LPCTSTR)IDD_MENU, hWnd, (DLGPROC)Menu);
                
          switch (wID)
            {   
            case IDD_EXIT:
              if(
                #ifdef _SIM_
                (true)
                #else
                MessageBox(hWnd,gettext(TEXT("Do you wish to exit?")),gettext(TEXT("Exit?")),MB_YESNO|MB_ICONQUESTION) == IDYES
                #endif
              ) {
                  SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
                  SendMessage (hWnd, WM_CLOSE, 0, 0);
                } else {
              }
              MapWindow::MapDirty = true;
	             HideMenu();
              FullScreen();
              return 0;

	    case IDD_BACK:
	      HideMenu();
	      FullScreen();
	      return 0;

            case IDD_STATUS:
              ShowStatus();
              MenuActive = false;
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
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

              return 0;

            case IDC_ABORTTASK:

              LockFlightData();
              ResumeAbortTask();
              UnlockFlightData();

              ShowWindow(hWndCB,SW_HIDE);                               
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
              return 0;

            case IDC_ANALYSIS:

              ShowWindow(hWndCB,SW_HIDE);                               
	      FullScreen();
              PopupAnalysis();
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
              return 0;

            case IDD_SETTINGS:

              COMPORTCHANGED = FALSE;
              AIRSPACEFILECHANGED = FALSE;
              WAYPOINTFILECHANGED = FALSE;
              TERRAINFILECHANGED = FALSE;
              TOPOLOGYFILECHANGED = FALSE;
              POLARFILECHANGED = FALSE;

              MenuActive = true;
			  MapWindow::SuspendDrawingThread();

              ShowWindow(hWndCB,SW_SHOW);
              SetWindowPos(hWndMainWindow,HWND_TOP,
                           0, 0, 0, 0,
                           SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);

              SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR);
              DialogBox(hInst, (LPCTSTR)IDD_SETTINGS, hWndMainWindow, (DLGPROC)Settings);
              ShowWindow(hWndCB,SW_HIDE);                               
              SwitchToMapWindow();
  
              LockFlightData();
              LockNavBox();

              MenuActive = false;

              if(COMPORTCHANGED)
                {

#ifndef _SIM_
                  // JMW disabled com opening in sim mode
                  devClose(devA());

                  devClose(devA());



                  RestartCommPorts();

                  

                  devInit(TEXT(""));

#endif

                }
                                        
              if((WAYPOINTFILECHANGED) || (TERRAINFILECHANGED))
                {
                  CloseTerrain();
                  Task[0].Index = -1;  ActiveWayPoint = -1;

                  OpenTerrain();
                  ReadWayPoints();
		  ReadAirfieldFile();

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

                }

              if (AIRFIELDFILECHANGED)
                {
		  ReadAirfieldFile();
                }

              if (POLARFILECHANGED) {
                CalculateNewPolarCoef();
                GlidePolar::SetBallast();
              }
              
              if (AIRFIELDFILECHANGED 
                  || AIRSPACEFILECHANGED
                  || WAYPOINTFILECHANGED
                  || TERRAINFILECHANGED
                  )
                CloseProgressDialog();



              UnlockFlightData();
              UnlockNavBox();
			  MapWindow::ResumeDrawingThread();

              SwitchToMapWindow();
	      FullScreen();
              ShowWindow(hWndCB,SW_HIDE);                               
	      HideMenu();
              return 0;

            case IDD_LOGGER:
              TCHAR TaskMessage[1024];
              MenuActive = true;

              if(LoggerActive)
                {
                  if(MessageBox(hWndMapWindow,gettext(TEXT("Stop Logger")),gettext(TEXT("Stop Logger")),MB_YESNO|MB_ICONQUESTION) == IDYES)
                    LoggerActive = FALSE;
                }
              else
                {
                  _tcscpy(TaskMessage,TEXT("Start Logger With Declaration\r\n"));
                  for(i=0;i<MAXTASKPOINTS;i++)
                    {
                      if(Task[i].Index == -1)
                        {
                          if(i==0)
                            _tcscat(TaskMessage,TEXT("None"));
                          break;
                        }
                      _tcscat(TaskMessage,WayPointList[ Task[i].Index ].Name);
                      _tcscat(TaskMessage,TEXT("\r\n"));
                    }

                  if(MessageBox(hWndMapWindow,TaskMessage,TEXT("Start Logger"),MB_YESNO|MB_ICONQUESTION) == IDYES)
                    {
                      LoggerActive = TRUE;
                      StartLogger(strAssetNumber);
		      LoggerHeader();
                      StartDeclaration();
                      for(i=0;i<MAXTASKPOINTS;i++)
                        {
                          if(Task[i].Index == -1) break;
                          AddDeclaration(WayPointList[Task[i].Index].Lattitude , WayPointList[Task[i].Index].Longditude  , WayPointList[Task[i].Index].Name );
                        }
                      EndDeclaration();
                    }
                }
              MenuActive = false;
	      FullScreen();
              SwitchToMapWindow();
	      HideMenu();

              return 0;
            }
        }

      FullScreen();

      FocusTimeOut = 0;
      if (!InfoWindowActive) {
	ShowMenu();
      }
      for(i=0;i<NUMINFOWINDOWS;i++)
        {       
          if(wmControl == hWndInfoWindow[i])
            {
	      InfoWindowActive = TRUE;
	      SetFocus(hWnd);

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

		}
	      else
		{
		  PopUpSelect(i);
		  DisplayText();
		}
	      return 0;
	    }
	}

    }
  return DefWindowProc(hWnd, message, wParam, lParam);          
}


void ProcessChar1 (char c)
{
  static TCHAR BuildingString[100]; 
  static int i = 0;
        
  if (i<90)
    {
      if(c=='\n')
        {
          BuildingString[i] = '\0';

          LockFlightData();
          devParseNMEA(devGetDeviceOnPort(0), BuildingString, &GPS_INFO);
          UnlockFlightData();
          
        }
      else
        {
          BuildingString[i++] = c;
          return;
        }
    }

  i = 0;
}

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
                  bool dodisplay = false;

                  if(ParseNMEAString(BuildingString,&GPS_INFO))
                    {
                      VARIOCONNECT  = TRUE;
                    } 
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
  static TCHAR Caption[NUMINFOWINDOWS][100];

  int DisplayType;
  
#ifdef _MAP_
  return;
#endif

  LockNavBox();
  
  // JMW note: this is updated every GPS time step
  
  for(i=0;i<NUMINFOWINDOWS;i++)
    {
      Caption[i][0]= 0;

      if (CALCULATED_INFO.Circling == TRUE)
        DisplayType = InfoType[i] & 0xff;
      else if (CALCULATED_INFO.FinalGlide == TRUE) {
        DisplayType = (InfoType[i] >> 16) & 0xff;
      } else {
        DisplayType = (InfoType[i] >> 8) & 0xff;
      }

      Data_Options[DisplayType].Formatter->AssignValue(DisplayType);
      Data_Options[DisplayType].Formatter->Render(hWndInfoWindow[i]);

      _stprintf(Caption[i],gettext(Data_Options[DisplayType].Title) );

      SetWindowText(hWndTitleWindow[i],Caption[i]);

    }

  UnlockNavBox();

}


void CommonProcessTimer()
{
  SystemIdleTimerReset();

  if(InfoWindowActive)
    {
      if(FocusTimeOut == FOCUSTIMEOUTMAX)
        {
          SwitchToMapWindow();
        } 
      FocusTimeOut ++;
     
    } 

  if (DisplayLocked) {
    if(MenuTimeOut==MENUTIMEOUTMAX) {
      ShowWindow(hWndMenuButton, SW_HIDE);
    } 
    MenuTimeOut++;
  }

  if (DisplayTimeOut >= DISPLAYTIMEOUTMAX) {
    BlankDisplay(true);
  } else {
    BlankDisplay(false);
  }
  DisplayTimeOut++;
  
  if (!ScreenBlanked) {
    // No need to redraw map or infoboxes if screen is blanked.
    // This should save lots of battery power due to CPU usage
    // of drawing the screen

    if (MapWindow::RequestMapDirty) {
      MapWindow::MapDirty = true;
      MapWindow::RequestMapDirty = false;
    }
    
    if (InfoBoxesDirty) {
      InfoBoxesDirty = false;
      //JMWTEST    LockFlightData();
      AssignValues();
      DisplayText();
      //JMWTEST    UnlockFlightData();
    }
  }

#if (EXPERIMENTAL > 0)

  if (bsms.Poll()) {
    // turn screen on if blanked and receive a new message
    DisplayTimeOut = 0;
  }

#endif

}

void ProcessTimer(void)
{
  CommonProcessTimer();

  // processing moved to its own thread

#ifndef _SIM_

    // now check GPS status

    static int itimeout = 0;
    itimeout++;

    if (itimeout % 20 != 0) {
      // timeout if no new data in 5 seconds
      return;
    }

    static BOOL LastGPSCONNECT = FALSE;
    static BOOL LastVARIOCONNECT = FALSE;
    static BOOL CONNECTWAIT = FALSE;
    static BOOL LOCKWAIT = FALSE;
    
    //
    // replace bool with BOOL to correct warnings and match variable declarations RB
    //
    BOOL gpsconnect = GPSCONNECT;

    if (GPSCONNECT) {
      extGPSCONNECT = TRUE;
    }

    GPSCONNECT = FALSE;
    BOOL varioconnect = VARIOCONNECT;
    BOOL navwarning = (BOOL)(GPS_INFO.NAVWarning);
    
    if((gpsconnect == FALSE) && (LastGPSCONNECT == FALSE))
      {
        
        devLinkTimeout(devA());
        devLinkTimeout(devB());
        
        if(LOCKWAIT == TRUE)
          {
            // gps was waiting for fix, now waiting for connection
            MapWindow::MapDirty = true;
            SwitchToMapWindow();
            FullScreen();
            LOCKWAIT = FALSE;
          }
        if(!CONNECTWAIT)
          {
            // gps is waiting for connection first time

            MapWindow::MapDirty = true;
            extGPSCONNECT = FALSE;

            DoStatusMessage(TEXT("Waiting for GPS Connection"));

            //            LoadString(hInst, IDS_CONNECTWAIT, szLoadText, MAX_LOADSTRING);
            //            ShowStatusMessage(szLoadText, 5000);

 //            SetDlgItemText(hGPSStatus,IDC_GPSMESSAGE,szLoadText);

            CONNECTWAIT = TRUE;
            MessageBeep(MB_ICONEXCLAMATION);
            FullScreen();
            
          } else {
          
          if (itimeout % 120 == 0) {
            // we've been waiting for connection a long time

            // no activity for 30 seconds, so assume PDA has been
            // switched off and on again
            //
            MapWindow::MapDirty = true;

            extGPSCONNECT = FALSE;

            DoStatusMessage(TEXT("Restarting Comm Ports"));
            MessageBeep(MB_ICONEXCLAMATION);
            RestartCommPorts();

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
            MapWindow::MapDirty = true;

            SwitchToMapWindow();
            FullScreen();
            CONNECTWAIT = FALSE;
          }
      }
    
    if((gpsconnect == TRUE) && (LastGPSCONNECT == TRUE))
      {
        if((navwarning == TRUE) && (LOCKWAIT == FALSE))
          {
            DoStatusMessage(TEXT("Waiting for GPS Fix"));
            MapWindow::MapDirty = true;

            LOCKWAIT = TRUE;
            MessageBeep(MB_ICONEXCLAMATION);
            FullScreen();
            
          }
        else if((navwarning == FALSE) && (LOCKWAIT == TRUE))
          {
            MapWindow::MapDirty = true;
            SwitchToMapWindow();
            FullScreen();
            LOCKWAIT = FALSE;
          }
      }
    
    if((varioconnect == TRUE) && (LastVARIOCONNECT == FALSE)) {
      // vario is connected now
    }

    LastGPSCONNECT = gpsconnect;

#endif // end processing of non-simulation mode

  
}


void SIMProcessTimer(void)
{
  CommonProcessTimer();

  static int ktimer=0;
  ktimer++;
  if (ktimer % 4 != 0) {
    return; // only update every 4 clicks
  }

  LockFlightData();

  GPSCONNECT = TRUE;
  extGPSCONNECT = TRUE;

  GPS_INFO.NAVWarning = FALSE;
  GPS_INFO.SatellitesUsed = 6;

  GPS_INFO.Lattitude = FindLattitude(GPS_INFO.Lattitude, GPS_INFO.Longditude, GPS_INFO.TrackBearing, GPS_INFO.Speed*1.0 );
  GPS_INFO.Longditude = FindLongditude(GPS_INFO.Lattitude, GPS_INFO.Longditude, GPS_INFO.TrackBearing, GPS_INFO.Speed*1.0);
  GPS_INFO.Time+= 1.0;

  GpsUpdated = TRUE;

  UnlockFlightData();

}


void SwitchToMapWindow(void)
{
  if (InfoWindowActive) {
    FocusOnWindow(InfoFocus,false);
  }
  InfoWindowActive = FALSE;
  SetFocus(hWndMapWindow);
  if (  MenuTimeOut< MENUTIMEOUTMAX) {
    MenuTimeOut = MENUTIMEOUTMAX;
  }
  if (  FocusTimeOut< FOCUSTIMEOUTMAX) {
    FocusTimeOut = FOCUSTIMEOUTMAX;
  }

  // JMW reactivate menu button
  // ShowWindow(hWndMenuButton, SW_SHOW);

}

void PopupAnalysis()
{
  DialogBox(hInst, (LPCTSTR)IDD_ANALYSIS, hWndInfoWindow[0], 
            (DLGPROC)AnalysisProc);
}


void PopupWaypointDetails()
{
  DialogBox(hInst, (LPCTSTR)IDD_WAYPOINTDETAILS, hWndInfoWindow[0], (DLGPROC)WaypointDetails);
  /*
  ShowWindow(hWndCB,SW_HIDE);                           
  FullScreen();
  SwitchToMapWindow();
  */
}


void PopupBugsBallast(int UpDown)
{
  DialogBox(hInst, (LPCTSTR)IDD_BUGSBALLAST, hWndInfoWindow[0], (DLGPROC)SetBugsBallast);
  ShowWindow(hWndCB,SW_HIDE);                           
  FullScreen();
  SwitchToMapWindow();
}


void PopUpSelect(int Index)
{
  CurrentInfoType = InfoType[Index];
  InfoType[Index] = DialogBox(hInst, (LPCTSTR)IDD_SELECT, hWndInfoWindow[Index], (DLGPROC)Select);
  StoreType(Index, InfoType[Index]);
  ShowWindow(hWndCB,SW_HIDE);                           
  FullScreen();
  SwitchToMapWindow();
}

#include <stdio.h>

void DebugStore(char *Str)
{
  FILE *stream;
  static TCHAR szFileName[] = TEXT("\\TEMP.TXT");

  stream = _wfopen(szFileName,TEXT("a+t"));

  fwrite(Str,strlen(Str),1,stream);

  fclose(stream);
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
  for (i=0; i<NUMINFOWINDOWS; i++) {
    ShowWindow(hWndInfoWindow[i], SW_HIDE);
    ShowWindow(hWndTitleWindow[i], SW_HIDE);
  }
}


void ShowInfoBoxes() {
  int i;
  InfoBoxesHidden = false;
  for (i=0; i<NUMINFOWINDOWS; i++) {
    ShowWindow(hWndInfoWindow[i], SW_SHOW);
    ShowWindow(hWndTitleWindow[i], SW_SHOW);
  }
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

    if (doblank) {
      BATTERYINFO BatteryInfo;
      GetBatteryInfo(&BatteryInfo);

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

