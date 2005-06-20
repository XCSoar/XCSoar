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
*/
#include "stdafx.h"

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

#include <commctrl.h>
#include <aygshell.h>
#include <sipapi.h>

HINSTANCE			hInst;					// The current instance
HWND					hWndCB;					// The command bar handle
HWND					hWndMainWindow; // Main Windows
HWND					hWndMapWindow;	// MapWindow
HWND					hProgress = NULL;	// Progress Dialog Box
HWND          hWndMenuButton = NULL;


HWND					hWndInfoWindow[NUMINFOWINDOWS];
HWND					hWndTitleWindow[NUMINFOWINDOWS];

int						InfoType[NUMINFOWINDOWS] = {3084,1542,257,2827,771,1293,4622,4369};

BOOL					DisplayLocked = TRUE;
BOOL					InfoWindowActive = TRUE;
int						FocusTimeOut = 0;

// Serial Port Globals
HANDLE				hReadThread = NULL;              // Handle to the read thread
HANDLE				hPort = INVALID_HANDLE_VALUE;    // Handle to the serial port
BOOL					PortAvailable = NULL;

// Display Gobals
HFONT					InfoWindowFont;
HFONT					TitleWindowFont;
HFONT					MapWindowFont;
int						CurrentInfoType;
int						InfoFocus = 0;
int						DisplayOrientation = TRACKUP;
int						DisplayTextType = DISPLAYNONE;

int						AltitudeMode = ALLON;
int						ClipAltitude = 1000;
int						AltWarningMargin = 100;
double				QNH = (double)1013.2;

int						iAirspaceBrush[12];

//SI to Local Units
double        SPEEDMODIFY = TOKNOTS;
double				LIFTMODIFY  = TOKNOTS;
double				DISTANCEMODIFY = TONAUTICALMILES;
double        ALTITUDEMODIFY = TOFEET;

//Flight Data Globals
double        MACREADY = 0;
NMEA_INFO			GPS_INFO;
DERIVED_INFO	CALCULATED_INFO;

//Local Static data
static int iTimerID;
static BOOL GPSCONNECT = FALSE;
static BOOL GPSPROCESS = TRUE;

// Final Glide Data
double SAFTEYALTITUDE = 0;
double BUGS = 1;
double BALLAST = 0;
int		 POLARID = 0;
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

// Registration Data
TCHAR strAssetNumber[MAX_LOADSTRING] = TEXT(""); //4G17DW31L0HY");
TCHAR strRegKey[MAX_LOADSTRING] = TEXT("");

//Snail Trial
SNAIL_POINT SnailTrail[TRAILSIZE];
int SnailNext = 0;
int TrailActive = TRUE;
int TrailLock = FALSE;

//IGC Logger
BOOL LoggerActive = FALSE;

// Others
double pi;
double FrameRate = 0;
int FrameCount = 0;
BOOL TopWindow = TRUE;

BOOL COMPORTCHANGED = FALSE;
BOOL AIRSPACEFILECHANGED = FALSE;
BOOL WAYPOINTFILECHANGED = FALSE;
BOOL TERRAINFILECHANGED = FALSE;


//Task Information
TASK_POINT Task[MAXTASKPOINTS +1 ] = {{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0},{-1,0,0,0,0,0,0,0,0}};
int ActiveWayPoint = -1;

// Assigned Area Task
double AATTaskLength = 120;
BOOL AATEnabled = FALSE;

#if UNDER_CE >= 300
	static SHACTIVATEINFO s_sai;
#endif

static	TCHAR *COMMPort[] = {TEXT("COM1:"),TEXT("COM2:"),TEXT("COM3:"),TEXT("COM4:"),TEXT("COM5:"),TEXT("COM6:"),TEXT("COM7:"),TEXT("COM8:"),TEXT("COM9:"),TEXT("COM10:")};
static	DWORD	dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};
static	DWORD PortIndex = 0;
static	DWORD SpeedIndex = 2;

SCREEN_INFO Data_Options[] = {
																{TEXT("Altitude"),					TEXT("GPS Alt"),		TEXT("%2.0f"), 0, AltitudeProcessing},
																{TEXT("Altitude AGL"),	    TEXT("A.G.L."),			TEXT("%2.0f"), 0, NoProcessing},

																{TEXT("Average"),						TEXT("Average"),		TEXT("%2.1f"), 0, NoProcessing},
																{TEXT("Bearing"),						TEXT("Bearing"),		TEXT("%2.0f°T"), 0, NoProcessing},
																{TEXT("Current L/D"),				TEXT("L/D"),				TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Cruise L/D"),				TEXT("Cr L/D"),			TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Ground Speed"),			TEXT("Speed"),			TEXT("%2.0f"), 0, SpeedProcessing},

																{TEXT("Last Thermal Avg"),	TEXT("L A"),				TEXT("%2.1f"), 0, NoProcessing},
																{TEXT("Last Thermal Gain"),	TEXT("L G"),				TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Last Thermal Time"),	TEXT("L T"),				TEXT("%2.0f"), 0, NoProcessing},

																{TEXT("MacReady Setting"),		TEXT("MacReady"),		TEXT("%2.1f"), 0, McReadyProcessing},

																{TEXT("Next Distance"),			TEXT("Dist"),	  		TEXT("%2.1f"), 0, NoProcessing},
																{TEXT("Next Alt Difference"),TEXT("Alt Dif"),		TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Next Alt Required"),	TEXT("Alt Req"),		TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Next Waypoint"),			TEXT("Next"),				TEXT(""),			 0, NextUpDown},

																{TEXT("Task Alt Difference"),TEXT("Dif Fin"),		TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Task Alt Required"), TEXT("Req Fin"),		TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Task Average Speed"),TEXT("Av Speed"),		TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Task Distance"),			TEXT("To Go"),			TEXT("%2.0f"), 0, NoProcessing},
																{TEXT("Task LD Finish"),		TEXT("LD Fin"),			TEXT("%1.0f"), 0, NoProcessing},


																{TEXT("Terrain Height"),		TEXT("Terrain"),		TEXT("%2.0f"), 0, NoProcessing},

																{TEXT("Thermal Average"),		TEXT("T. A"),				TEXT("%2.1f"), 0, NoProcessing},
																{TEXT("Thermal Gain"),			TEXT("Gain"),				TEXT("%2.0f"), 0, NoProcessing},

																{TEXT("Track"),							TEXT("Track"),			TEXT("%2.0f°T"), 0, DirectionProcessing},
																{TEXT("Vario"),							TEXT("Vario"),			TEXT("%2.1f"), 0, NoProcessing},
																{TEXT("Wind Speed"),				TEXT("Wind S"),			TEXT("%2.0f"), 0, WindSpeedProcessing},
																{TEXT("Wind Bearing"),			TEXT("Wind B"),			TEXT("%2.0f°T"), 0, WindDirectionProcessing},
																{TEXT("AA Time"),						TEXT("AA Time"),		TEXT("%2.0f"),	0, NoProcessing},
																{TEXT("AA Max Dist"),				TEXT("Max D"),			TEXT("%2.0f"),	0, NoProcessing},
																{TEXT("AA Min Dist"),				TEXT("Min D"),			TEXT("%2.0f"),	0, NoProcessing},
																{TEXT("AA Max Speed"),			TEXT("Max S"),			TEXT("%2.0f"),	0, NoProcessing},
																{TEXT("AA Min Speed"),			TEXT("Min S"),			TEXT("%2.0f"),	0, NoProcessing},
                                {TEXT("Airspeed"),			    TEXT("Airspeed"),		TEXT("%2.0f"),	0, NoProcessing},
                                {TEXT("Baro Alt"),			    TEXT("Altitude"),		TEXT("%2.0f"),	0, NoProcessing}
															};

int NUMSELECTSTRINGS = 34;

int ControlWidth, ControlHeight;


// Forward declarations of functions included in this code module:
ATOM							MyRegisterClass	(HINSTANCE, LPTSTR);
BOOL							InitInstance	(HINSTANCE, int);
LRESULT CALLBACK	WndProc			(HWND, UINT, WPARAM, LPARAM);
LRESULT						MainMenu(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void							AssignValues(void);
void							DisplayText(void);
void							ReadAssetNumber(void);
void							ProcessTimer	(void);
void							SIMProcessTimer(void);
void							PopUpSelect(int i);
void							SwitchToMapWindow(void);
HWND							CreateRpCommandBar(HWND hwnd);

#ifdef DEBUG
	void						DebugStore(TCHAR *Str);
#endif

int WINAPI WinMain(	HINSTANCE hInstance,
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

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_XCSOAR);

	pi = (double)atan(1) * 4;
	InitSineTable();
	memset( &(GPS_INFO), 0, sizeof(GPS_INFO));
	memset( &(CALCULATED_INFO), 0,sizeof(CALCULATED_INFO));
	memset( &SnailTrail[0],0,TRAILSIZE*sizeof(SNAIL_POINT));
	ReadRegistrySettings();
	CalculateNewPolarCoef();
  OpenTerrain();
	ReadWayPoints();
	if(NumberOfWayPoints)
	{
		SetHome();
	}

	ReadAirspace();

	CreateDrawingThread();


#ifdef _SIM_
	MessageBox(	hWndMainWindow, TEXT("XCSoar Simulator\r\nNothing is Real!!"), TEXT("Caution"), MB_OK|MB_ICONWARNING);
 	SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
#else
	PortIndex = 0;
	SpeedIndex = 2;
	ReadPortSettings(&PortIndex,&SpeedIndex);
	PortAvailable = PortInitialize (COMMPort[PortIndex],dwSpeed[SpeedIndex]);
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

  wc.style					= CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc		= (WNDPROC) WndProc;
  wc.cbClsExtra			= 0;
  wc.cbWndExtra			= dc.cbWndExtra ;
  wc.hInstance			= hInstance;
  wc.hIcon					= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOAR));
  wc.hCursor				= 0;
  wc.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName		= 0;
  wc.lpszClassName	= szWindowClass;

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
	TCHAR	szTitle[MAX_LOADSTRING];			// The title bar text
	TCHAR	szWindowClass[MAX_LOADSTRING];		// The window class name
	RECT rc;
	LOGFONT logfont;
	int i;
	int FontHeight, FontWidth;
	int TitleHeight;

	hInst = hInstance;		// Store instance handle in our global variable
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
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		NULL, NULL, hInstance, NULL);

	if (!hWndMainWindow)
	{
		return FALSE;
	}

	GetClientRect(hWndMainWindow, &rc);

	FontHeight = (rc.bottom - rc.top ) / 10;
	FontWidth = (rc.right - rc.left ) / 24;

	memset ((char *)&logfont, 0, sizeof (logfont));

  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = FontHeight;
  logfont.lfWidth =  FontWidth;
	logfont.lfWeight = FW_MEDIUM;
  logfont.lfCharSet = ANSI_CHARSET;

  InfoWindowFont = CreateFontIndirect (&logfont);

	memset ((char *)&logfont, 0, sizeof (logfont));

  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = FontHeight/3;
	logfont.lfWidth =  FontWidth/2;
	logfont.lfWeight = FW_MEDIUM;

	TitleWindowFont = CreateFontIndirect (&logfont);

	GetClientRect(hWndMainWindow, &rc);

	ControlWidth = 2*(rc.right - rc.left) / NUMINFOWINDOWS;
	ControlHeight = (rc.bottom - rc.top) / 7;
	TitleHeight = (int)(ControlHeight/4.5);

	#ifdef _MAP_
	ControlHeight = 0;
	#endif

	#ifndef _MAP_

	for(i=0;i<NUMINFOWINDOWS/2;i++)
	{
		if(i==0)
		{
			hWndInfoWindow[i] = CreateWindow(TEXT("STATIC"),TEXT(""),WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY|WS_BORDER,
														i*ControlWidth, rc.top+TitleHeight,ControlWidth,ControlHeight,
														hWndMainWindow,NULL,hInstance,NULL);
		}
		else
		{
			hWndInfoWindow[i] = CreateWindow(TEXT("STATIC"),TEXT("0"),WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY,
														i*ControlWidth, rc.top+TitleHeight,ControlWidth,ControlHeight-TitleHeight,
														hWndMainWindow,NULL,hInstance,NULL);
		}

		hWndTitleWindow[i] = CreateWindow(TEXT("STATIC"),Data_Options[InfoType[i]& 0xff].Title,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY,
															i*ControlWidth, rc.top,ControlWidth,10,
															hWndMainWindow,NULL,hInstance,NULL);

		hWndInfoWindow[i+(NUMINFOWINDOWS/2)] = CreateWindow(TEXT("STATIC"),TEXT("0"),WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY,
																									i*ControlWidth, (rc.bottom - ControlHeight+TitleHeight), ControlWidth,ControlHeight-TitleHeight,
																									hWndMainWindow,NULL,hInstance,NULL);

		hWndTitleWindow[i+(NUMINFOWINDOWS/2)] = CreateWindow(TEXT("STATIC"),Data_Options[InfoType[i+(NUMINFOWINDOWS/2)]& 0xff].Title,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER,
																									i*ControlWidth, (rc.bottom - ControlHeight), ControlWidth,TitleHeight,
																									hWndMainWindow,NULL,hInstance,NULL);
	}

	for(i=0;i<NUMINFOWINDOWS;i++)
	{
		SendMessage(hWndInfoWindow[i],WM_SETFONT,(WPARAM)InfoWindowFont,MAKELPARAM(TRUE,0));
		SendMessage(hWndTitleWindow[i],WM_SETFONT,(WPARAM)TitleWindowFont,MAKELPARAM(TRUE,0));
	}
	#endif

	#ifdef _MAP_
	hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,WS_VISIBLE|WS_CHILD|WS_TABSTOP,
															0, 0, (rc.right - rc.left), (rc.bottom-rc.top) ,
															hWndMainWindow,NULL,hInstance,NULL);
	#else
	hWndMapWindow = CreateWindow(TEXT("MapWindowClass"),NULL,WS_VISIBLE|WS_CHILD|WS_TABSTOP,
															0, rc.top + ControlHeight, (rc.right - rc.left), ((rc.bottom-rc.top) - (2*ControlHeight)),
															hWndMainWindow,NULL,hInstance,NULL);

	#endif

  hWndMenuButton = CreateWindow(TEXT("BUTTON"),TEXT("Menu"),WS_VISIBLE|WS_CHILD,
																0, 0,0,0,hWndMainWindow,NULL,hInst,NULL);

  SendMessage(hWndMenuButton,WM_SETFONT,(WPARAM)TitleWindowFont,MAKELPARAM(TRUE,0));

  SetWindowPos(hWndMenuButton,HWND_TOP,0,ControlHeight,ControlWidth,(rc.bottom - rc.top) / 10,SWP_SHOWWINDOW);

  SHFullScreen(hWndMainWindow,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);

  ShowWindow(hWndMainWindow, nCmdShow);
	UpdateWindow(hWndMainWindow);

	SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i;

	switch (message)
	{
		case WM_COMMAND:
			return MainMenu(hWnd, message, wParam, lParam);
		break;

		case WM_CREATE:
			memset (&s_sai, 0, sizeof (s_sai));
      s_sai.cbSize = sizeof (s_sai);

			#ifdef _SIM_
				iTimerID = SetTimer(hWnd,1000,1000,NULL);
			#else
				iTimerID = SetTimer(hWnd,1000,4000,NULL);
			#endif

			hWndCB = CreateRpCommandBar(hWnd);
			AssignValues();
			DisplayText();
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
			if(InfoWindowActive)
				if(DisplayLocked)
					SetWindowLong(hWndInfoWindow[InfoFocus],GWL_STYLE,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY|WS_BORDER);
				else
					SetWindowLong(hWndInfoWindow[InfoFocus],GWL_STYLE,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY);
			else
				SetFocus(hWndMapWindow);
		break;

		case WM_KEYUP:
			switch (wParam)
			{
				case VK_UP :  // SCROLL UP
					if (CALCULATED_INFO.Circling == TRUE)
						i = InfoType[InfoFocus] & 0xff;
					else
						i = InfoType[InfoFocus] >> 8;
					Data_Options[i].Process(TRUE);
					DoCalculations(&GPS_INFO,&CALCULATED_INFO);
					AssignValues();
					DisplayText();
					FocusTimeOut = 0;
				break;

				case VK_DOWN: // SCROLL DOWN
					if (CALCULATED_INFO.Circling == TRUE)
						i = InfoType[InfoFocus] & 0xff;
					else
						i = InfoType[InfoFocus] >> 8;
					Data_Options[i].Process(FALSE);
					DoCalculations(&GPS_INFO,&CALCULATED_INFO);
					AssignValues();
					DisplayText();
					FocusTimeOut = 0;
				break;
			}
		break;

		case WM_TIMER:
			FrameRate = (double)FrameCount;
			#ifdef _SIM_
				SIMProcessTimer();
				FrameRate = FrameRate;
			#else
				ProcessTimer();
				FrameRate = FrameRate/4;
			#endif
			FrameCount = 0;
			AssignValues();
			DisplayText();
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
			CloseDrawingThread();
			NumberOfWayPoints = 0; Task[0].Index = -1;  ActiveWayPoint = -1; AATEnabled = FALSE;
			NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0; NumberOfAirspaceCircles = 0;
			CloseTerrain();

       if(hProgress)
				DestroyWindow(hProgress);

			if(iTimerID)
				KillTimer(hWnd,iTimerID);

			if(PortAvailable)
				PortClose (hPort);

			DestroyWindow(hWndMapWindow);
			DestroyWindow(hWndMenuButton);

			for(i=0;i<NUMINFOWINDOWS;i++)
			{
				DestroyWindow(hWndInfoWindow[i]);
				DestroyWindow(hWndTitleWindow[i]);
			}
			CommandBar_Destroy(hWndCB);

			DeleteObject(InfoWindowFont);
			DeleteObject(TitleWindowFont);

			if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
			if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
			if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);
			if(WayPointList != NULL) LocalFree((HLOCAL)WayPointList);

			DestroyWindow(hWndMainWindow);
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
					if(MessageBox(hWnd,TEXT("Do You Wish To Exit?"),TEXT("Exit?"),MB_YESNO|MB_ICONQUESTION) == IDYES)
					{
						SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
						SendMessage (hWnd, WM_CLOSE, 0, 0);
					}
          SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
				return 0;

				case IDD_BUGS:
          DWORD dwError;

					ShowWindow(hWndCB,SW_SHOW);
					SHFullScreen(hWndMainWindow,SHFS_SHOWSIPBUTTON|SHFS_SHOWTASKBAR);
					DialogBox(hInst, (LPCTSTR)IDD_BUGSBALLAST, hWnd, (DLGPROC)SetBugsBallast);
         	dwError = GetLastError();
          SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
					SHFullScreen(hWndMainWindow,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
					ShowWindow(hWndCB,SW_HIDE);
					SwitchToMapWindow();
				return 0;

				case IDD_PRESSURE:
					ShowWindow(hWndCB,SW_SHOW);
					SHFullScreen(hWndMainWindow,SHFS_SHOWSIPBUTTON|SHFS_SHOWTASKBAR);
					DialogBox(hInst, (LPCTSTR)IDD_AIRSPACEPRESS, hWnd, (DLGPROC)AirspacePress);
					ConvertFlightLevels();
         	SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
					SHFullScreen(hWndMainWindow,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
					ShowWindow(hWndCB,SW_HIDE);
					SwitchToMapWindow();
				return 0;

				case IDD_TASK:
					SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR);
					DialogBox(hInst, (LPCTSTR)IDD_TASK, hWnd, (DLGPROC)SetTask);
         	SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
          SHFullScreen(hWndMainWindow,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
					ShowWindow(hWndCB,SW_HIDE);
					SwitchToMapWindow();
				return 0;

				case IDD_LOCK:
					DisplayLocked = ! DisplayLocked;
					SwitchToMapWindow();
         	SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
				return 0;

				case IDD_SETTINGS:
					COMPORTCHANGED = FALSE;
					AIRSPACEFILECHANGED = FALSE;
					WAYPOINTFILECHANGED = FALSE;
					TERRAINFILECHANGED = FALSE;

          CloseDrawingThread();
          ShowWindow(hWndCB,SW_SHOW);
         	SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
					SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR);
					DialogBox(hInst, (LPCTSTR)IDD_SETTINGS, hWndMainWindow, (DLGPROC)Settings);
					ShowWindow(hWndCB,SW_HIDE);
					SwitchToMapWindow();

					if(COMPORTCHANGED)
					{
						if(PortAvailable)
						{
							PortAvailable = FALSE; PortClose (hPort);
						}
						PortIndex = 0; SpeedIndex = 2; ReadPortSettings(&PortIndex,&SpeedIndex);
						PortAvailable = PortInitialize (COMMPort[PortIndex],dwSpeed[SpeedIndex]);
					}

					if((WAYPOINTFILECHANGED) || (TERRAINFILECHANGED))
					{
            CloseTerrain();
            NumberOfWayPoints = 0; Task[0].Index = -1;  ActiveWayPoint = -1;
						if(WayPointList != NULL) LocalFree((HLOCAL)WayPointList);
            OpenTerrain();
          	ReadWayPoints();
          	if(NumberOfWayPoints) SetHome();
          }

          if(AIRSPACEFILECHANGED)
          {
						NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0; NumberOfAirspaceCircles = 0;
						if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
						if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
						if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);
          	ReadAirspace();
					}

          CreateDrawingThread();
          SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
          SwitchToMapWindow();
					SHFullScreen(hWndMainWindow,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
					ShowWindow(hWndCB,SW_HIDE);
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
							StartDeclaration();
							for(i=0;i<MAXTASKPOINTS;i++)
							{
								if(Task[i].Index == -1) break;
								AddDeclaration(WayPointList[Task[i].Index].Lattitude , WayPointList[Task[i].Index].Longditude  , WayPointList[Task[i].Index].Name );
							}
							EndDeclaration();
						}
					}
				return 0;
			}
		}

    SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
		SwitchToMapWindow();
		FocusTimeOut = 0;
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
						SetWindowLong(hWndInfoWindow[i],GWL_STYLE,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY|WS_BORDER);
						SetWindowLong(hWndInfoWindow[InfoFocus],GWL_STYLE,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY);
						InfoFocus = i;
						InfoWindowActive = TRUE;
					}
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

#define WAIT 0
#define FILL 1
void ProcessChar (char c)
{
	static int i = 0;
	static int State = WAIT;
	static TCHAR BuildingString[100];

	if(!GPSPROCESS)
		return;

	if(State == WAIT)
	{
		if(c=='$')
		{
			BuildingString[0] = c;
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
				#ifdef DEBUG
					DebugStore(BuildingString);
				#endif
				if(ParseNMEAString(BuildingString,&GPS_INFO))
				{
					GPSCONNECT  = TRUE;
					if(GPS_INFO.NAVWarning == FALSE)
					{
						if(DoCalculations(&GPS_INFO,&CALCULATED_INFO))
						{
							AssignValues();
							DisplayText();
						}
					}
				}
			}
			else
			{
				BuildingString[i++] = c;
			}
		}
	}
}

void	AssignValues(void)
{
	Data_Options[0].Value = ALTITUDEMODIFY*GPS_INFO.Altitude;
	Data_Options[1].Value = ALTITUDEMODIFY*CALCULATED_INFO.AltitudeAGL  ;

	Data_Options[2].Value = LIFTMODIFY*CALCULATED_INFO.Average30s;
	Data_Options[3].Value = GPS_INFO.WaypointBearing;
	Data_Options[4].Value = CALCULATED_INFO.LD;
	Data_Options[5].Value = CALCULATED_INFO.CruiseLD;
	Data_Options[6].Value = SPEEDMODIFY*GPS_INFO.Speed;

	Data_Options[7].Value = LIFTMODIFY*CALCULATED_INFO.LastThermalAverage;
	Data_Options[8].Value = ALTITUDEMODIFY*CALCULATED_INFO.LastThermalGain;
	Data_Options[9].Value = CALCULATED_INFO.LastThermalTime;

	Data_Options[10].Value = MACREADY;

	Data_Options[11].Value = DISTANCEMODIFY*GPS_INFO.WaypointDistance;
	Data_Options[12].Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeDifference;
	Data_Options[13].Value = ALTITUDEMODIFY*CALCULATED_INFO.NextAltitudeRequired;
	Data_Options[14].Value = 0; // Next Waypoint Text

	Data_Options[15].Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeDifference;
	Data_Options[16].Value = ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeRequired;
	Data_Options[17].Value = SPEEDMODIFY*CALCULATED_INFO.TaskSpeed;
	Data_Options[18].Value = DISTANCEMODIFY*CALCULATED_INFO.TaskDistanceToGo;
	Data_Options[19].Value = CALCULATED_INFO.LDFinish;


	Data_Options[20].Value = ALTITUDEMODIFY*CALCULATED_INFO.TerrainAlt ;

	Data_Options[21].Value = LIFTMODIFY*CALCULATED_INFO.AverageThermal;
	Data_Options[22].Value = ALTITUDEMODIFY*CALCULATED_INFO.ThermalGain;

	Data_Options[23].Value = GPS_INFO.TrackBearing;
	Data_Options[24].Value = LIFTMODIFY*CALCULATED_INFO.Vario;
	Data_Options[25].Value = SPEEDMODIFY*CALCULATED_INFO.WindSpeed;
	Data_Options[26].Value = CALCULATED_INFO.WindBearing;
	Data_Options[27].Value = CALCULATED_INFO.AATTimeToGo / 60;
	Data_Options[28].Value = DISTANCEMODIFY*CALCULATED_INFO.AATMaxDistance ;
	Data_Options[29].Value = DISTANCEMODIFY*CALCULATED_INFO.AATMinDistance ;
	Data_Options[30].Value = SPEEDMODIFY*CALCULATED_INFO.AATMaxSpeed;
	Data_Options[31].Value = SPEEDMODIFY*CALCULATED_INFO.AATMinSpeed;
  Data_Options[32].Value = SPEEDMODIFY*GPS_INFO.Airspeed;
  Data_Options[33].Value = ALTITUDEMODIFY*GPS_INFO.BaroAltitude;

}

void DisplayText(void)
{
	int i;
	static TCHAR Value[100] = TEXT("");
	static TCHAR Caption[100] = TEXT("");
	int DisplayType;

	#ifdef _MAP_
		return;
	#endif

	for(i=0;i<NUMINFOWINDOWS;i++)
	{
		if (CALCULATED_INFO.Circling == TRUE)
			DisplayType = InfoType[i] & 0xff;
		else
			DisplayType = InfoType[i] >> 8;

		if(DisplayType == 14) // Waypoint Name
		{
			if(ActiveWayPoint >=0)
			{

				_stprintf(Caption,Data_Options[DisplayType].Title );
				if ( DisplayTextType == DISPLAYFIRSTTHREE)
				{
					_tcsncpy(Value,WayPointList[ Task[ActiveWayPoint].Index ].Name,3);
					Value[3] = '\0';
				}
				else if( DisplayTextType == DISPLAYNUMBER)
				{
					_stprintf(Value,TEXT("%d"),WayPointList[ Task[ActiveWayPoint].Index ].Number );
				}
				else
				{
					_tcsncpy(Value,WayPointList[ Task[ActiveWayPoint].Index ].Name,5);
					Value[5] = '\0';
				}
			}
			else
			{
				_stprintf(Caption,Data_Options[DisplayType].Title );
				Value[0] = '\0';
			}
		}
		else
		{
			_stprintf(Caption,Data_Options[DisplayType].Title );
			_stprintf(Value,Data_Options[DisplayType].Format, Data_Options[DisplayType].Value );
		}

		SetWindowText(hWndInfoWindow[i],Value);
		SetWindowText(hWndTitleWindow[i],Caption);
	}
}

void ProcessTimer(void)
{
	static BOOL LastGPSCONNECT = FALSE;
	static BOOL CONNECTWAIT = FALSE;
	static BOOL LOCKWAIT = FALSE;
	TCHAR szLoadText[MAX_LOADSTRING];

	SystemIdleTimerReset();

	if(InfoWindowActive)
	{
		FocusTimeOut ++;
		if(FocusTimeOut > 10)
		{
			SwitchToMapWindow();
		}
	}

	ReadAssetNumber();

	if(!PortAvailable)
	  return;

	CheckRegistration();

	if(LoggerActive)
		LogPoint(GPS_INFO.Lattitude , GPS_INFO.Longditude , GPS_INFO.Altitude );

	AddSnailPoint();


	if((GPSCONNECT == FALSE) && (LastGPSCONNECT == FALSE))
	{
		PortWriteString(TEXT("NMEA\r\n"));

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
		}
	}

	if((GPSCONNECT == TRUE) && (LastGPSCONNECT == FALSE))
	{
		if(CONNECTWAIT)
		{
			DestroyWindow(hProgress);
			SwitchToMapWindow();
			hProgress = NULL;
			CONNECTWAIT = FALSE;
		}
	}

	if((GPSCONNECT == TRUE) && (LastGPSCONNECT == TRUE))
	{
		if((GPS_INFO.NAVWarning == TRUE) && (LOCKWAIT == FALSE))
		{
			hProgress=CreateDialog(hInst,(LPCTSTR)IDD_PROGRESS,hWndMainWindow,(DLGPROC)Progress);
			LoadString(hInst, IDS_LOCKWAIT, szLoadText, MAX_LOADSTRING);
			SetDlgItemText(hProgress,IDC_MESSAGE,szLoadText);
			LOCKWAIT = TRUE;
			MessageBeep(MB_ICONEXCLAMATION);
			SetWindowPos(hProgress,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
		}
		else if((GPS_INFO.NAVWarning == FALSE) && (LOCKWAIT == TRUE))
		{
			DestroyWindow(hProgress);
			SwitchToMapWindow();
			hProgress = NULL;
			LOCKWAIT = FALSE;
		}
	}
	LastGPSCONNECT = GPSCONNECT;
	GPSCONNECT  = FALSE;

}

void SIMProcessTimer(void)
{
	SystemIdleTimerReset();

	ReadAssetNumber();

	if(InfoWindowActive)
	{
		FocusTimeOut ++;
		if(FocusTimeOut > 10)
		{
			SwitchToMapWindow();
		}
	}

	GPS_INFO.Lattitude = FindLattitude(GPS_INFO.Lattitude, GPS_INFO.Longditude, GPS_INFO.TrackBearing, GPS_INFO.Speed );
	GPS_INFO.Longditude = FindLongditude(GPS_INFO.Lattitude, GPS_INFO.Longditude, GPS_INFO.TrackBearing, GPS_INFO.Speed);
	GPS_INFO.Time++;

	if(DoCalculations(&GPS_INFO,&CALCULATED_INFO))
	{
		AssignValues();
		DisplayText();
	}

	if(LoggerActive)
		LogPoint(GPS_INFO.Lattitude , GPS_INFO.Longditude , GPS_INFO.Altitude );

	AddSnailPoint();

}

void AddSnailPoint(void)
{
	if(TrailLock) return;

	SnailTrail[SnailNext].Lattitude = GPS_INFO.Lattitude;
	SnailTrail[SnailNext].Longditude = GPS_INFO.Longditude;
	SnailTrail[SnailNext].Vario = CALCULATED_INFO.Vario ;
	SnailNext ++;
	SnailNext %= TRAILSIZE;
}


void SwitchToMapWindow(void)
{
	InfoWindowActive = FALSE;
	SetWindowLong(hWndInfoWindow[InfoFocus],GWL_STYLE,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY);
	SetFocus(hWndMapWindow);
}

void PopUpSelect(int Index)
{
	CurrentInfoType = InfoType[Index];
	InfoType[Index] = DialogBox(hInst, (LPCTSTR)IDD_SELECT, hWndInfoWindow[Index], (DLGPROC)Select);
	StoreType(Index,InfoType[Index]);
	SwitchToMapWindow();
	SHFullScreen(hWndMainWindow,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
	ShowWindow(hWndCB,SW_HIDE);
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