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

  $Id: XCSoar.cpp,v 1.36 2005/07/07 09:02:43 jwharington Exp $
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

HINSTANCE                       hInst;                                  // The current instance
HWND                                    hWndCB;                                 // The command bar handle
HWND                                    hWndMainWindow; // Main Windows
HWND                                    hWndMapWindow;  // MapWindow
HWND                                    hProgress = NULL;       // Progress Dialog Box
HWND          hWndMenuButton = NULL;

HWND                                    hWndCDIWindow = NULL; //CDI Window

HWND                                    hWndInfoWindow[NUMINFOWINDOWS];
HWND                                    hWndTitleWindow[NUMINFOWINDOWS];

int                                     InfoType[NUMINFOWINDOWS] = {921102,725525,262144,74518,657930,2236963,394758,1644825};

BOOL                                    DisplayLocked = TRUE;
BOOL                                    InfoWindowActive = TRUE;
int                                             FocusTimeOut = 0;
int                                             MenuTimeOut = 0;



HBRUSH hBrushSelected;
HBRUSH hBrushUnselected;
COLORREF ColorSelected = RGB(0xC0,0xC0,0xC0);
COLORREF ColorUnselected = RGB(0xFF,0xFF,0xFF);
COLORREF ColorWarning = RGB(0xFF,0x00,0x00);

// Serial Port Globals

HANDLE                          hPort1 = INVALID_HANDLE_VALUE;    // Handle to the serial port
HANDLE                          hPort2 = INVALID_HANDLE_VALUE;    // Handle to the serial port
BOOL                                    Port1Available = NULL;
BOOL                                    Port2Available = NULL;

// Display Gobals
HFONT                                   InfoWindowFont;
HFONT                                   TitleWindowFont;
HFONT                                   MapWindowFont;
HFONT                                   CDIWindowFont; // New
HFONT                                   MapLabelFont;

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
double        MACREADY = 0;
bool          AutoMacReady = false;

int          NettoSpeed = 1000;

NMEA_INFO                       GPS_INFO;
DERIVED_INFO  CALCULATED_INFO;
BOOL GPSCONNECT = FALSE;
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

//Snail Trial
SNAIL_POINT SnailTrail[TRAILSIZE];
int SnailNext = 0;
int TrailActive = TRUE;
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

//IGC Logger
BOOL LoggerActive = FALSE;

// Others
double pi;
double FrameRate = 0;
int FrameCount = 0;
BOOL TopWindow = TRUE;

BOOL COMPORTCHANGED = FALSE;
BOOL AIRSPACEFILECHANGED = FALSE;
BOOL AIRFIELDFILECHANGED = FALSE;
BOOL WAYPOINTFILECHANGED = FALSE;
BOOL TERRAINFILECHANGED = FALSE;
BOOL TOPOLOGYFILECHANGED = FALSE;

//Task Information
Task_t Task = {{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0}};
int ActiveWayPoint = -1;

// Assigned Area Task
double AATTaskLength = 120;
BOOL AATEnabled = FALSE;

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


#define GLOBALFONT "Tahoma"
//#define GLOBALFONT "HelmetCondensed"

void PopupBugsBallast(int updown);

// Groups: 
//   Altitude 0,1,20,33
//   Aircraft info 3,6,23,32,37
//   LD 4,5,19
//   Vario 2,7,8,9,21,22,24
//   Wind 25,26
//   Mcready 10,34,35
//   Nav 11,12,13,15,16,17,18,27,28,29,30,31
//   Waypoint 14,36

SCREEN_INFO Data_Options[] = { 
  // 0  
  {TEXT("Altitude"), TEXT("GPS Alt"), new InfoBoxFormatter(TEXT("%2.0f")), AltitudeProcessing, 1, 33}, 

  // 1
  {TEXT("Altitude AGL"), TEXT("A.G.L."), new FormatterLowWarning(TEXT("%2.0f"),0.0), NoProcessing, 20, 0}, 

  // 2
  {TEXT("Average"), TEXT("Average"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 7, 24}, 

  // 3
  {TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f°T")), NoProcessing, 6, 37}, 

  // 4
  {TEXT("Current L/D"), TEXT("L/D"), new InfoBoxFormatter(TEXT("%2.0f")), PopupBugsBallast, 5, 19}, 

  // 5
  {TEXT("Cruise L/D"), TEXT("Cr L/D"), new InfoBoxFormatter(TEXT("%2.0f")), PopupBugsBallast, 19, 4}, 

  // 6
  {TEXT("Ground Speed"), TEXT("Speed"), new InfoBoxFormatter(TEXT("%2.0f")), SpeedProcessing, 23, 3}, 
 
  // 7
  {TEXT("Last Thermal Avg"), TEXT("L A"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2}, 

  // 8
  {TEXT("Last Thermal Gain"), TEXT("L G"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 9, 7}, 

  // 9
  {TEXT("Last Thermal Time"), TEXT("L T"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 21, 8}, 

  // 10
  {TEXT("MacReady Setting"), TEXT("MacReady"), new InfoBoxFormatter(TEXT("%2.1f")), McReadyProcessing, 34, 35}, 

  // 11
  {TEXT("Next Distance"), TEXT("Dist"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 12, 31}, 

  // 12
  {TEXT("Next Alt Difference"), TEXT("Alt Dif"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 13, 11}, 

  // 13
  {TEXT("Next Alt Required"), TEXT("Alt Req"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 15, 12}, 

  // 14
  {TEXT("Next Waypoint"), TEXT("Next"), new FormatterWaypoint(TEXT("\0")), NextUpDown, 36, 36}, 
 
  // 15
  {TEXT("Task Alt Difference"), TEXT("Dif Fin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 16, 13}, 

  // 16
  {TEXT("Task Alt Required"), TEXT("Req Fin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 17, 15}, 

  // 17
  {TEXT("Task Average Speed"), TEXT("Av Speed"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16}, 

  // 18
  {TEXT("Task Distance"), TEXT("To Go"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 27, 17}, 
  
  // 19
  {TEXT("Task LD Finish"), TEXT("LD Fin"), new InfoBoxFormatter(TEXT("%1.0f")), NoProcessing, 4, 5}, 

  // 20
  {TEXT("Terrain Height"), TEXT("Terrain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 33, 1}, 
  
  // 21
  {TEXT("Thermal Average"), TEXT("Av Climb"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 22, 9}, 

  // 22
  {TEXT("Thermal Gain"), TEXT("Gain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 24, 21}, 
 
  // 23
  {TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f°T")), DirectionProcessing, 32, 6}, 

  // 24
  {TEXT("Vario"), TEXT("Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 2, 22}, 

  // 25
  {TEXT("Wind Speed"), TEXT("Wind S"), new InfoBoxFormatter(TEXT("%2.0f")), WindSpeedProcessing, 26, 26}, 

  // 26
  {TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f°T")), WindDirectionProcessing, 25, 25}, 

  // 27
  {TEXT("AA Time"), TEXT("AA Time"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 28, 18}, 

  // 28
  {TEXT("AA Max Dist"), TEXT("Max D"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 29, 27}, 

  // 29
  {TEXT("AA Min Dist"), TEXT("Min D"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 30, 28}, 

  // 30
  {TEXT("AA Max Speed"), TEXT("Max S"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 31, 29}, 

  // 31
  {TEXT("AA Min Speed"), TEXT("Min S"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 11, 30}, 

  // 32
  {TEXT("Airspeed"), TEXT("Airspeed"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 37, 23}, 

  // 33
  {TEXT("Baro Alt"), TEXT("Altitude"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 0, 20},

  // 34
  {TEXT("MacReady speed"), TEXT("V Mc"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 35, 10}, 

  // 35
  {TEXT("Percentage climb"), TEXT("%% Climb"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 10, 34},

  // 36
  {TEXT("Time of day"), TEXT("Time"), new InfoBoxFormatter(TEXT("%04.0f")), NoProcessing, 14, 14},

  // 37
  {TEXT("G load"), TEXT("G"), new InfoBoxFormatter(TEXT("%2.2f")), NoProcessing, 3, 32},

};

int NUMSELECTSTRINGS = 38;

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
void                                                    ProcessTimer    (void);
void                                                    SIMProcessTimer(void);
void                                                    PopUpSelect(int i);
void                                                    SwitchToMapWindow(void);
HWND                                                    CreateRpCommandBar(HWND hwnd);

#ifdef DEBUG
void                                            DebugStore(TCHAR *Str);
#endif


extern RECT MapRect;
extern BOOL GpsUpdated;

void HideMenu() {
  // ignore this if the display isn't locked -- must keep menu visible
  if (DisplayLocked) {
    ShowWindow(hWndMenuButton, SW_HIDE);
    MenuTimeOut = MENUTIMEOUTMAX;
  }
}

void ShowMenu() {
  MenuTimeOut = 0;
  ShowWindow(hWndMenuButton, SW_SHOW);
}


extern bool RequestMapDirty; // GUI asks for map window refresh
extern bool MapDirty; // the actual map refresh trigger
extern bool RequestFastRefresh;

void FullScreen() {
  SHFullScreen(hWndMainWindow,
               SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
  SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),
  	       GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
  RequestFastRefresh = true;
  InfoBoxesDirty = true; 

  //  RequestMapDirty = true;
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

extern BOOL CLOSETHREAD;

DWORD CalculationThread (LPVOID lpvoid) {
  bool infoarrived = false;

  NMEA_INFO     tmp_GPS_INFO;
  DERIVED_INFO  tmp_CALCULATED_INFO;

  while (!CLOSETHREAD) {
    
    if (GpsUpdated) {
      GpsUpdated = FALSE;
      
      //    CheckRegistration();
      
      // JMW moved logging and snail to Calculations

      // make local copy before editing...
      LockFlightData();
      memcpy(&tmp_GPS_INFO,&GPS_INFO,sizeof(NMEA_INFO));
      memcpy(&tmp_CALCULATED_INFO,&CALCULATED_INFO,sizeof(DERIVED_INFO));
      UnlockFlightData();

      if(DoCalculations(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) 
        {
          InfoBoxesDirty = true;
          RequestMapDirty = true;
        }
      
      if (!GPS_INFO.VarioAvailable) {
        // run the function anyway, because this gives audio functions
        DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO);
      }

      // values changed, so copy them back now: ONLY CALCULATED INFO
      // should be changed in DoCalculations, so we only need to write
      // that one back (otherwise we may write over new data)
      LockFlightData();
      memcpy(&CALCULATED_INFO,&tmp_CALCULATED_INFO,sizeof(DERIVED_INFO));
      UnlockFlightData();
      
    } else {
      Sleep(100); // sleep 250 ms
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

  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = ICC_UPDOWN_CLASS;
  InitCommonControls();
        
  // Perform application initialization:
  if (!InitInstance (hInstance, nCmdShow)) 
    {
      return FALSE;
    }

  FullScreen();

  hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_XCSOAR);

  pi = (double)atan(1) * 4;
  InitSineTable();

  SHSetAppKeyWndAssoc(VK_APP1, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP2, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP3, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP4, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP5, hWndMainWindow);
  SHSetAppKeyWndAssoc(VK_APP6, hWndMainWindow);

  InitializeCriticalSection(&CritSec_FlightData);
  InitializeCriticalSection(&CritSec_NavBox);
  InitializeCriticalSection(&CritSec_TerrainDataGraphics);
  InitializeCriticalSection(&CritSec_TerrainDataCalculations);

  memset( &(GPS_INFO), 0, sizeof(GPS_INFO));
  memset( &(CALCULATED_INFO), 0,sizeof(CALCULATED_INFO));
  memset( &SnailTrail[0],0,TRAILSIZE*sizeof(SNAIL_POINT));

  ReadRegistrySettings();

  // display start up screen
  //  StartupScreen();
  // not working very well at all
  
  LoadWindFromRegistry();
  CalculateNewPolarCoef();
  SetBallast();
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

  AssignValues();
  DisplayText();

  CreateDrawingThread();

  CreateCalculationThread();

#ifdef _SIM_
  /*
  MessageBox(   hWndMainWindow, TEXT("Simulator mode\r\nNothing is Real!!"), TEXT("Caution"), MB_OK|MB_ICONWARNING);
  SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
  */
  ShowStatusMessage(TEXT("Simulation\r\nNothing is real!"), 3000);

#else
  /*
  MessageBox(   hWndMainWindow, TEXT("Maintain effective\r\nLOOKOUT at all times"), TEXT("Caution"), MB_OK|MB_ICONWARNING);
  SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);

  */
  ShowStatusMessage(TEXT("Maintain effective\r\nLOOKOUT at all times"), 3000);

  RestartCommPorts();

#endif

  cai302Register();
  ewRegister();
	// ... register all supported devices

  devInit();

  FullScreen();
  SwitchToMapWindow();

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
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOAR));
  wc.hCursor                    = 0;
  wc.hbrBackground              = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName               = 0;
  wc.lpszClassName              = szWindowClass;

  if (!RegisterClass (&wc))
    return FALSE;

  wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = (WNDPROC)MapWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = dc.cbWndExtra ;
  wc.hInstance = hInstance;
  wc.hIcon = (HICON)NULL;
  wc.hCursor = NULL;
  wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = TEXT("MapWindowClass");

  RequestMapDirty = true;

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

  hWndMainWindow = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
                                0, 0, GetSystemMetrics(SM_CXSCREEN), 
				GetSystemMetrics(SM_CYSCREEN), 
                                NULL, NULL, hInstance, NULL);

  SendMessage(hWndMainWindow, WM_SETICON,
	      (WPARAM)ICON_BIG, (LPARAM)IDI_XCSOAR);
  SendMessage(hWndMainWindow, WM_SETICON,
	      (WPARAM)ICON_SMALL, (LPARAM)IDI_XCSOARS);

  if (!hWndMainWindow)
    {   
      return FALSE;
    }
  FullScreen();

  hBrushSelected = (HBRUSH)CreateSolidBrush(ColorSelected);
  hBrushUnselected = (HBRUSH)CreateSolidBrush(ColorUnselected);

  GetClientRect(hWndMainWindow, &rc);

  FontHeight = (rc.bottom - rc.top ) / FONTHEIGHTRATIO;
  FontWidth = (rc.right - rc.left ) / FONTWIDTHRATIO;
        
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
        
  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = (int)(FontHeight*MAPFONTHEIGHTRATIO);
  logfont.lfWidth =  (int)(FontWidth*MAPFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;

#ifndef NOCLEARTYPE
  logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY; // JMW
#endif

  MapLabelFont = CreateFontIndirect (&logfont);

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
          hWndInfoWindow[i] = CreateWindow(TEXT("STATIC"),TEXT(""),WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY|WS_BORDER,
                                           i*ControlWidth, rc.top+TitleHeight,ControlWidth,ControlHeight,
                                           hWndMainWindow,NULL,hInstance,NULL);
        }
      else
        {
          hWndInfoWindow[i] = CreateWindow(TEXT("STATIC"),TEXT("\0"),
                                           WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY,
                                           i*ControlWidth, rc.top+TitleHeight,
                                           ControlWidth,ControlHeight-TitleHeight,
                                           hWndMainWindow,NULL,hInstance,NULL);
        }
                        
      hWndTitleWindow[i] = CreateWindow(TEXT("STATIC"),
                                        Data_Options[InfoType[i]& 0xff].Title,
                                        WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY,
                                        i*ControlWidth, rc.top, ControlWidth,TitleHeight,
                                        hWndMainWindow,NULL,hInstance,NULL);

      hWndInfoWindow[i+(NUMINFOWINDOWS/2)] = CreateWindow(TEXT("STATIC"),TEXT("\0"),
                                                          WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY,
                                                          i*ControlWidth, (rc.bottom - ControlHeight+TitleHeight),
                                                          ControlWidth,ControlHeight-TitleHeight,
                                                          hWndMainWindow,NULL,hInstance,NULL);
                         
      hWndTitleWindow[i+(NUMINFOWINDOWS/2)] = CreateWindow(TEXT("STATIC"),Data_Options[InfoType[i+(NUMINFOWINDOWS/2)]& 0xff].Title,
                                                           WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER,
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

  MapRect.top = rc.top+ControlHeight;
  MapRect.left = rc.left;
  MapRect.bottom = rc.bottom-ControlHeight;
  MapRect.right = rc.right;

#ifdef _MAP_

  hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                               0, 0, (rc.right - rc.left), 
			       (rc.bottom-rc.top) ,
                               hWndMainWindow,NULL,hInstance,NULL);
#else

#ifdef OLDWINDOW
  hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                               0, rc.top + ControlHeight, (rc.right - rc.left), ((rc.bottom-rc.top) - (2*ControlHeight)),
                               hWndMainWindow,NULL,hInstance,NULL);
#else

  hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,WS_VISIBLE|WS_CHILD|WS_TABSTOP,
                               0, 0, (rc.right - rc.left), 
			       (rc.bottom-rc.top) ,
                               hWndMainWindow,NULL,hInstance,NULL);

#endif
        
#endif


  hWndMenuButton = CreateWindow(TEXT("BUTTON"),TEXT("Menu"),WS_VISIBLE|WS_CHILD,
                                0, 0,0,0,hWndMainWindow,NULL,hInst,NULL);

  SendMessage(hWndMenuButton,WM_SETFONT,(WPARAM)TitleWindowFont,MAKELPARAM(TRUE,0));

  // JMW moved menu button to center, to make room for thermal indicator
  SetWindowPos(hWndMenuButton,HWND_TOP,(int)(rc.right-rc.left-ControlWidth*MENUBUTTONWIDTHRATIO)/2,
               (int)(ControlHeight+10),
               (int)(ControlWidth*MENUBUTTONWIDTHRATIO),
               (int)((rc.bottom - rc.top)/10),SWP_SHOWWINDOW);

  // start of new code for displaying CDI window

  // JMW changed layout a bit, deleted Waiting for GPS info text as it is misleading here

  hWndCDIWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),WS_VISIBLE|WS_CHILD, 
                               0,0,0,0,hWndMainWindow,NULL,hInst,NULL);
  SendMessage(hWndCDIWindow,WM_SETFONT,(WPARAM)CDIWindowFont,MAKELPARAM(TRUE,0));

  SetWindowPos(hWndCDIWindow,hWndMenuButton,
	  (int)(ControlWidth*0.6),(int)(ControlHeight+1),(int)(ControlWidth*2.8),(int)(TitleHeight*1.4),SWP_SHOWWINDOW);
  // JMW also made it so it doesn't obscure airspace warnings

  // end of new code for drawing CDI window (see below for destruction of objects)

  /////////////

    ShowWindow(hWndCDIWindow, SW_HIDE);
    ShowWindow(hWndMenuButton, SW_HIDE);

    FullScreen();
        
    ShowWindow(hWndMainWindow, nCmdShow);
    UpdateWindow(hWndMainWindow);

    SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);


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
}


bool Debounce(WPARAM wParam) {
  static DWORD fpsTimeLast= -1;
  static WPARAM wlast = 0;
  DWORD fpsTimeThis = ::GetTickCount();
  DWORD dT = fpsTimeThis-fpsTimeLast;
  fpsTimeLast = fpsTimeThis;

  if (wParam != wlast) {
    wlast = wParam;
    return true; // button changed, so OK
  }

  wlast = wParam;

  if (dT>250) {
    return true;
  } else {
    return false;
  }
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int i, j;
  static bool lastpress = false;
  long wdata;

  switch (message) 
    {
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
          SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
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
      switch (wParam)
        {
        case VK_APP1:

          if (!Debounce(wParam)) break;

	  RequestToggleFullScreen();

          break;

        case VK_APP2:

          if (!Debounce(wParam)) break;

          if (!InfoWindowActive) {
	    TrailActive = !TrailActive;


	    if (EnableSoundModes) {
	      if (TrailActive) {
		PlayResource(TEXT("IDR_INSERT")); 
	      } else {
		PlayResource(TEXT("IDR_REMOVE")); 
	      }
	    }

      // ARH Let the user know what's happened
            if (TrailActive)
              ShowStatusMessage(TEXT("SnailTrail ON"), 2000);
            else
              ShowStatusMessage(TEXT("SnailTrail OFF"), 2000);

            break;
          }

          i = getInfoType(InfoFocus);

          j = Data_Options[i].next_screen;
          setInfoType(InfoFocus,j);

          AssignValues();
          DisplayText();

          FocusTimeOut = 0;

          break;

        case VK_APP3:

          if (!Debounce(wParam)) break;

          if (!InfoWindowActive) {
            EnableSoundVario = !EnableSoundVario;
            VarioSound_EnableSound((BOOL)EnableSoundVario);

	    if (EnableSoundModes) {
	      if (EnableSoundVario) {
		PlayResource(TEXT("IDR_INSERT")); 
	      } else {
		PlayResource(TEXT("IDR_REMOVE")); 
	      }
	    }

            // ARH Let the user know what's happened
            if (EnableSoundVario)
              ShowStatusMessage(TEXT("Vario Sounds ON"), 2000);
            else
              ShowStatusMessage(TEXT("Vario Sounds OFF"), 2000);

            break;
          }

          i = getInfoType(InfoFocus);

          j = Data_Options[i].prev_screen;
          setInfoType(InfoFocus,j);

          AssignValues();
          DisplayText();

          FocusTimeOut = 0;

          break;

        case VK_APP4:

          if (!Debounce(wParam)) break;

          if (InfoWindowActive)
            break;

          LockFlightData();

          MarkLocation(GPS_INFO.Longditude, GPS_INFO.Lattitude);

          UnlockFlightData();

          // ARH Let the user know what's happened
          ShowStatusMessage(TEXT("Dropped marker"), 1500);


          break;

        case VK_UP :  // SCROLL UP
          DoInfoKey(1);
          break;

        case VK_DOWN: // SCROLL DOWN
          DoInfoKey(-1);
          break;

        case VK_RETURN:
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
      VarioSound_Close();  // added sgi

      devCloseAll();

      CloseDrawingThread();

      NumberOfWayPoints = 0; Task[0].Index = -1;  ActiveWayPoint = -1; AATEnabled = FALSE;
      NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0; NumberOfAirspaceCircles = 0;
      CloseTerrain();
      CloseTopology();

      if(hProgress)
        DestroyWindow(hProgress);

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

      if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
      if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
      if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);
      if(WayPointList != NULL) LocalFree((HLOCAL)WayPointList);

      DestroyWindow(hWndMainWindow);

      DeleteCriticalSection(&CritSec_FlightData);
      DeleteCriticalSection(&CritSec_NavBox);
      DeleteCriticalSection(&CritSec_TerrainDataCalculations); 
      DeleteCriticalSection(&CritSec_TerrainDataGraphics); 

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
      if(wmControl == hWndMenuButton)
        {
          wID = DialogBox(hInst, (LPCTSTR)IDD_MENU, hWnd, (DLGPROC)Menu);
                
          switch (wID)
            {   
            case IDD_EXIT:
              if(MessageBox(hWnd,TEXT("Do you wish to exit?"),TEXT("Exit?"),MB_YESNO|MB_ICONQUESTION) == IDYES)
                {
                  SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
                  SendMessage (hWnd, WM_CLOSE, 0, 0);
                }       
	      HideMenu();
              FullScreen();
              return 0;

	    case IDD_BACK:
	      HideMenu();
	      FullScreen();
	      return 0;

            case IDD_BUGS:
              DWORD dwError;

              ShowWindow(hWndCB,SW_SHOW);
              SHFullScreen(hWndMainWindow,SHFS_SHOWSIPBUTTON|SHFS_SHOWTASKBAR);
              DialogBox(hInst, (LPCTSTR)IDD_BUGSBALLAST, hWnd, (DLGPROC)SetBugsBallast);
              dwError = GetLastError();
              ShowWindow(hWndCB,SW_HIDE);                               
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
              return 0;
                
            case IDD_PRESSURE:
              ShowWindow(hWndCB,SW_SHOW);
              SHFullScreen(hWndMainWindow,SHFS_SHOWSIPBUTTON|SHFS_SHOWTASKBAR);
              DialogBox(hInst, (LPCTSTR)IDD_AIRSPACEPRESS, hWnd, (DLGPROC)AirspacePress);
              ConvertFlightLevels();
              ShowWindow(hWndCB,SW_HIDE);                               
              SwitchToMapWindow();
	      HideMenu();
	      FullScreen();
              return 0; 
        
            case IDD_TASK:
              SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR);
              DialogBox(hInst, (LPCTSTR)IDD_TASK, hWnd, (DLGPROC)SetTask);
              ShowWindow(hWndCB,SW_HIDE);                               
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


            case IDD_SETTINGS:
              COMPORTCHANGED = FALSE;
              AIRSPACEFILECHANGED = FALSE;
              WAYPOINTFILECHANGED = FALSE;
              TERRAINFILECHANGED = FALSE;
              TOPOLOGYFILECHANGED = FALSE;


              SuspendDrawingThread();

              ShowWindow(hWndCB,SW_SHOW);
              SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);

              SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR);
              DialogBox(hInst, (LPCTSTR)IDD_SETTINGS, hWndMainWindow, (DLGPROC)Settings);
              ShowWindow(hWndCB,SW_HIDE);                               
              SwitchToMapWindow();
  
              LockFlightData();
              LockNavBox();

              if(COMPORTCHANGED)
                {

#ifndef _SIM_
                  // JMW disabled com opening in sim mode
                  devClose(devA());
                  devClose(devA());

                  RestartCommPorts();

                  
                  devInit();
#endif

                }
                                        
              if((WAYPOINTFILECHANGED) || (TERRAINFILECHANGED))
                {
                  CloseTerrain();
                  NumberOfWayPoints = 0; Task[0].Index = -1;  ActiveWayPoint = -1;
                  if(WayPointList != NULL) LocalFree((HLOCAL)WayPointList);
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

                  NumberOfAirspacePoints = 0; 
		  NumberOfAirspaceAreas = 0; 
		  NumberOfAirspaceCircles = 0;
                  if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
                  if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
                  if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);
                  ReadAirspace();

                }

              if (AIRFIELDFILECHANGED)
                {
		  ReadAirfieldFile();
                }

              UnlockFlightData();
              UnlockNavBox();
              ResumeDrawingThread();

              SwitchToMapWindow();
	      FullScreen();
              ShowWindow(hWndCB,SW_HIDE);                               
	      HideMenu();
              return 0;

            case IDD_LOGGER:
              TCHAR TaskMessage[1024];
              if(LoggerActive)
                {
                  if(MessageBox(hWndMapWindow,TEXT("Stop Logger"),TEXT("Stop Logger"),MB_YESNO|MB_ICONQUESTION) == IDYES)
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
		      
                      if(DoCalculationsVario(&GPS_INFO,&CALCULATED_INFO))
                        {
			  //    AssignValues();
                          // JMW don't display here, as it is too often
                        }
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


int SecsToDisplayTime(int d) {
  int mins;
  int hours;
  hours = (d/3600);
  mins = (d/60-hours*60);
  return (hours*100+mins);
}


int DetectStartTime() {
  static int starttime = -1;
  if (starttime == -1) {
    if (GPS_INFO.Speed > 5) {
      starttime = (int)GPS_INFO.Time;
    } else {
      return 0;
    }
  }
  return SecsToDisplayTime((int)GPS_INFO.Time-starttime);
}


void    AssignValues(void)
{
  if (InfoBoxesHidden) {
    // no need to assign values
    return;
  }

  LockNavBox();

  Data_Options[0].Formatter->Value = ALTITUDEMODIFY*GPS_INFO.Altitude;

  ((FormatterLowWarning*)Data_Options[1].Formatter)->minimum = 
    ALTITUDEMODIFY*SAFETYALTITUDETERRAIN;
  Data_Options[1].Formatter->Value = ALTITUDEMODIFY*CALCULATED_INFO.AltitudeAGL  ;
        
  Data_Options[2].Formatter->Value = LIFTMODIFY*CALCULATED_INFO.Average30s;
  Data_Options[3].Formatter->Value = CALCULATED_INFO.WaypointBearing;

  if (CALCULATED_INFO.LD== 999) {
    Data_Options[4].Formatter->Valid = false;
  } else {
    Data_Options[4].Formatter->Valid = true;
    Data_Options[4].Formatter->Value = CALCULATED_INFO.LD; 
  }

  if (CALCULATED_INFO.CruiseLD== 999) {
    Data_Options[5].Formatter->Valid = false;
  } else {
    Data_Options[5].Formatter->Valid = true;
    Data_Options[5].Formatter->Value = CALCULATED_INFO.CruiseLD; 
  }

  Data_Options[6].Formatter->Value = SPEEDMODIFY*GPS_INFO.Speed;
        
  Data_Options[7].Formatter->Value = LIFTMODIFY*CALCULATED_INFO.LastThermalAverage;
  Data_Options[8].Formatter->Value = ALTITUDEMODIFY*CALCULATED_INFO.LastThermalGain;
  Data_Options[9].Formatter->Value = CALCULATED_INFO.LastThermalTime;
        
  Data_Options[10].Formatter->Value = MACREADY;
        
  Data_Options[11].Formatter->Value = DISTANCEMODIFY*CALCULATED_INFO.WaypointDistance;
  Data_Options[12].Formatter->Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeDifference;
  Data_Options[13].Formatter->Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeRequired; 
  Data_Options[14].Formatter->Value = 0; // Next Waypoint Text
        
  Data_Options[15].Formatter->Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeDifference;
  Data_Options[16].Formatter->Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeRequired;
  Data_Options[17].Formatter->Value = SPEEDMODIFY*CALCULATED_INFO.TaskSpeed;
  Data_Options[18].Formatter->Value = DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo; 

  if (CALCULATED_INFO.LDFinish== 999) {
    Data_Options[19].Formatter->Valid = false;
  } else {
    Data_Options[19].Formatter->Valid = true;
    Data_Options[19].Formatter->Value = CALCULATED_INFO.LDFinish; 
  }

  Data_Options[20].Formatter->Value = ALTITUDEMODIFY*CALCULATED_INFO.TerrainAlt ;
        
  Data_Options[21].Formatter->Value = LIFTMODIFY*CALCULATED_INFO.AverageThermal;
  Data_Options[22].Formatter->Value = ALTITUDEMODIFY*CALCULATED_INFO.ThermalGain;
        
  Data_Options[23].Formatter->Value = GPS_INFO.TrackBearing;

  if (GPS_INFO.VarioAvailable) {
    Data_Options[24].Formatter->Value = LIFTMODIFY*GPS_INFO.Vario;
  } else {
    Data_Options[24].Formatter->Value = LIFTMODIFY*CALCULATED_INFO.Vario;
  }

  Data_Options[25].Formatter->Value = SPEEDMODIFY*CALCULATED_INFO.WindSpeed;
  Data_Options[26].Formatter->Value = CALCULATED_INFO.WindBearing;
  Data_Options[27].Formatter->Value = CALCULATED_INFO.AATTimeToGo / 60;
  Data_Options[28].Formatter->Value = DISTANCEMODIFY*CALCULATED_INFO.AATMaxDistance ; 
  Data_Options[29].Formatter->Value = DISTANCEMODIFY*CALCULATED_INFO.AATMinDistance ; 
  Data_Options[30].Formatter->Value = SPEEDMODIFY*CALCULATED_INFO.AATMaxSpeed;
  Data_Options[31].Formatter->Value = SPEEDMODIFY*CALCULATED_INFO.AATMinSpeed;

  if (GPS_INFO.AirspeedAvailable) {
    Data_Options[32].Formatter->Value = SPEEDMODIFY*GPS_INFO.Airspeed;
    Data_Options[32].Formatter->Valid = true;
  } else {
    Data_Options[32].Formatter->Valid = false;
  }

  if (GPS_INFO.BaroAltitudeAvailable) {
    Data_Options[33].Formatter->Value = ALTITUDEMODIFY*GPS_INFO.BaroAltitude;
    Data_Options[33].Formatter->Valid = true;
  } else {
    Data_Options[33].Formatter->Valid = false;
  }

  Data_Options[34].Formatter->Value = SPEEDMODIFY*CALCULATED_INFO.VMcReady; 

  Data_Options[35].Formatter->Value = CALCULATED_INFO.PercentCircling;

  Data_Options[36].Formatter->Value = DetectStartTime();

  if (GPS_INFO.AccelerationAvailable) {
    Data_Options[37].Formatter->Value = GPS_INFO.Gload;
    Data_Options[37].Formatter->Valid = true;
  } else {
    Data_Options[37].Formatter->Valid = false;
  }

  UnlockNavBox();

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

      Data_Options[DisplayType].Formatter->Render(hWndInfoWindow[i]);

      _stprintf(Caption[i],Data_Options[DisplayType].Title );

      SetWindowText(hWndTitleWindow[i],Caption[i]);

    }

  UnlockNavBox();

}



void ProcessTimer(void)
{

  SystemIdleTimerReset();

  if(InfoWindowActive)
    {
      if(FocusTimeOut==FOCUSTIMEOUTMAX)
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

  if (RequestMapDirty) {
    MapDirty = true;
    RequestMapDirty = false;
  }
  if (InfoBoxesDirty) {
    InfoBoxesDirty = false;
    //JMWTEST    LockFlightData();
    AssignValues();
    DisplayText();
    //JMWTEST    UnlockFlightData();
  }

  //    ReadAssetNumber();

  if(!Port1Available)
    return;

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
    TCHAR szLoadText[MAX_LOADSTRING];
    
    //
    // replace bool with BOOL to correct warnings and match variable declarations RB
    //
    BOOL gpsconnect = GPSCONNECT;
    GPSCONNECT = FALSE;
    BOOL varioconnect = VARIOCONNECT;
    BOOL navwarning = (BOOL)(GPS_INFO.NAVWarning);
    
    if((gpsconnect == FALSE) && (LastGPSCONNECT == FALSE))
      {
        
        devLinkTimeout(devA());
        devLinkTimeout(devB());
        
        if(LOCKWAIT == TRUE)
          {
            DestroyWindow(hProgress);
            SwitchToMapWindow();
            hProgress = NULL;
            LOCKWAIT = FALSE;
          }
        if(!CONNECTWAIT)
          {
            hProgress=CreateDialog(hInst,(LPCTSTR)IDD_PROGRESS,hWndMainWindow,(DLGPROC)Progress);
            LoadString(hInst, IDS_CONNECTWAIT, szLoadText, MAX_LOADSTRING);
            SetDlgItemText(hProgress,IDC_MESSAGE,szLoadText);
            CONNECTWAIT = TRUE;
            MessageBeep(MB_ICONEXCLAMATION);
            SetWindowPos(hProgress,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
            FullScreen();
            
          } else {
          
          if (itimeout % 240 == 0) {
            // no activity for one minute, so assume device has been
            // switched off
#ifndef _SIM_
            RestartCommPorts();
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
            DestroyWindow(hProgress);
            SwitchToMapWindow();
            hProgress = NULL;
            CONNECTWAIT = FALSE;
          }
      }
    
    if((gpsconnect == TRUE) && (LastGPSCONNECT == TRUE))
      {
        if((navwarning == TRUE) && (LOCKWAIT == FALSE))
          {
            hProgress=CreateDialog(hInst,(LPCTSTR)IDD_PROGRESS,hWndMainWindow,(DLGPROC)Progress);
            LoadString(hInst, IDS_LOCKWAIT, szLoadText, MAX_LOADSTRING);
            SetDlgItemText(hProgress,IDC_MESSAGE,szLoadText);
            LOCKWAIT = TRUE;
            MessageBeep(MB_ICONEXCLAMATION);
            SetWindowPos(hProgress,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
            FullScreen();
            
          }
        else if((navwarning == FALSE) && (LOCKWAIT == TRUE))
          {
            DestroyWindow(hProgress);
            SwitchToMapWindow();
            hProgress = NULL;
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
  SystemIdleTimerReset();

  //  ReadAssetNumber();

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

  if (RequestMapDirty) {
    MapDirty = true;
    RequestMapDirty = false;
  }

  if (InfoBoxesDirty) {
    InfoBoxesDirty = false;
    //JMWTEST    LockFlightData();
    AssignValues();
    DisplayText();
    //JMWTEST    UnlockFlightData();
  }

  static int ktimer=0;
  ktimer++;
  if (ktimer % 4 != 0) {
    return; // only update every 4 clicks
  }

  LockFlightData();

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

#ifdef DEBUG
#include <stdio.h>

void DebugStore(TCHAR *Str)
{
  FILE *stream;
  static TCHAR szFileName[] = TEXT("\\TEMP.TXT");

  stream = _wfopen(szFileName,TEXT("ab"));

  fwrite(Str,wcslen(Str),1,stream);

  fclose(stream);
}

#endif

static bool ref_navbox = false;
static bool ref_flightdata = false;


void LockNavBox() {
  // EnterCriticalSection(&CritSec_NavBox);
  //  while(ref_navbox) {}
  //ref_navbox = true;
}

void UnlockNavBox() {
  //ref_navbox = false;
  //  LeaveCriticalSection(&CritSec_NavBox);
}

void LockFlightData() {
  EnterCriticalSection(&CritSec_FlightData);
  //  while(ref_flightdata) {}
  // ref_flightdata = true;
}

void UnlockFlightData() {
  //  ref_flightdata = false;
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
