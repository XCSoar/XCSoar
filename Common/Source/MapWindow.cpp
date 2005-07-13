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

#include "Mapwindow.h"
#include "Utils.h"
#include "McReady.h"
#include "Airspace.h"
#include "Waypointparser.h"
#include "Dialogs.h"

#include "externs.h"
#include "VarioSound.h"

#include <windows.h>
#include <math.h>

#include <tchar.h>

#include "Terrain.h"

static DWORD DrawThread (LPVOID);

static void CalculateScreenPositions(POINT Orig, RECT rc, POINT *Orig_Aircraft);
static void CalculateScreenPositionsAirspace(POINT Orig, RECT rc, POINT *Orig_Aircraft);
static void CalculateWaypointReachable(void);


bool		PointVisible(POINT *P, RECT *rc);
bool            PointVisible(double lon, double lat);
rectObj screenbounds_latlon;


static void DrawAircraft(HDC hdc, POINT Orig);
static void DrawBestCruiseTrack(HDC hdc, POINT Orig);
static void DrawCompass(HDC hdc, RECT rc);
static void DrawWind(HDC hdc, POINT Orig, RECT rc);
static void DrawWindAtAircraft(HDC hdc, POINT Orig, RECT rc);
static void DrawAirSpace(HDC hdc, RECT rc);
static void DrawWaypoints(HDC hdc, RECT rc);
static void DrawFlightMode(HDC hdc, RECT rc);
static void DrawTrail(HDC hdc, POINT Orig, RECT rc);
static void DrawTask(HDC hdc, RECT rc);
static void DrawAbortedTask(HDC hdc, RECT rc, POINT Orig);
static void DrawBearing(HDC hdc, POINT Orig);
static void DrawMapScale(HDC hDC,RECT rc);
static void DrawMapScale2(HDC hDC,RECT rc, POINT Orig_Aircraft);
static void DrawFinalGlide(HDC hDC,RECT rc);
static void DrawThermalBand(HDC hDC,RECT rc);
static void DrawGlideThroughTerrain(HDC hDC, RECT rc);
static void DrawCDI();

extern HWND					hWndCDIWindow;

static void DrawSolidLine(HDC , POINT , POINT );
static void DrawDashLine(HDC , INT ,POINT , POINT , COLORREF );



static HBITMAP hDrawBitMap = NULL;
static HBITMAP hDrawBitMapBg = NULL;
static HBITMAP hDrawBitMapTmp = NULL;
static HDC hdcDrawWindow = NULL;
static HDC hdcDrawWindowBg = NULL;
static HDC hdcScreen = NULL;
static HDC hDCTemp = NULL;

double PanX=0.0;
double PanY=0.0;
double PanXr=0.0;
double PanYr=0.0;

bool EnablePan = FALSE;

extern int iround(double i);
extern void ShowMenu();

BOOL CLOSETHREAD = FALSE;
BOOL THREADRUNNING = TRUE;
//static BOOL	THREADRUNNING = FALSE;

DWORD  dwDrawThreadID;
HANDLE hDrawThread;

double RequestMapScale = 5;
double MapScale = 5;
double DisplayAngle = 0.0;
double DisplayAircraftAngle = 0.0;
static double DrawScale;

static bool AutoZoom = false;

static NMEA_INFO DrawInfo;
static DERIVED_INFO DerivedDrawInfo;

static int dTDisplay=0;

static HBITMAP hLandable, hReachable, hTurnPoint, hSmall, hCruise, hClimb,
hFinalGlide, hAutoMcReady, hTerrainWarning;

// 12 is number of airspace types
int	iAirspaceBrush[AIRSPACECLASSCOUNT];
int	iAirspaceColour[AIRSPACECLASSCOUNT];
BOOL bAirspaceBlackOutline = FALSE;

static HBRUSH   hBackgroundBrush;

// ARH: removed static
HBRUSH hAirspaceBrushes[NUMAIRSPACEBRUSHES];
HBITMAP hAirspaceBitmap[NUMAIRSPACEBRUSHES];

// ARH: removed static, so that Colours can be read by
// airspace colour dialog
COLORREF Colours[NUMAIRSPACECOLORS] =
{RGB(0xFF,0x00,0x00), RGB(0x00,0xFF,0x00), RGB(0x00,0x00,0xFF), RGB(0xFF,0xFF,0x00),
 RGB(0xFF,0x00,0xFF), RGB(0x00,0xFF,0xFF), RGB(0x7F,0x00,0x00), RGB(0x00,0x7F,0x00),
 RGB(0x00,0x00,0x7F), RGB(0x7F,0x7F,0x00), RGB(0x7F,0x00,0x7F), RGB(0x00,0x7F,0x7F),
 RGB(0xFF,0xFF,0xFF), RGB(0xC0,0xC0,0xC0), RGB(0x7F,0x7F,0x7F), RGB(0x00,0x00,0x00)};

static COLORREF BackgroundColor = RGB(0xF5,0xF5,0xF5);

static      HPEN hpAircraft, hpAircraftBorder;
static      HPEN hpWind;
static      HPEN hpWindThick;
static      HPEN hpBearing;
static      HPEN hpBestCruiseTrack;
static      HPEN hpCompass;
static      HPEN hpThermalBand, hpThermalBandGlider;
static      HPEN hpFinalGlideAbove, hpFinalGlideBelow;
static      HPEN hpMapScale;
static      HPEN hpTerrainLine;

static      HBRUSH hbCompass;
static      HBRUSH hbThermalBand;
static      HBRUSH hbBestCruiseTrack;
static      HBRUSH hbFinalGlideBelow, hbFinalGlideAbove;


int SelectedWaypoint;


void TextInBox(HDC hDC, TCHAR* Value, int x, int y, int size) {
  SIZE tsize;
  RECT brect;
  if (size==0) {
    size = _tcslen(Value);
  }

  HBRUSH hbOld;
  hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));

  GetTextExtentPoint(hDC, Value, size, &tsize);
  brect.left = x-1;
  brect.right = brect.left+tsize.cx+2;
  brect.top = y-1;
  brect.bottom = brect.top+tsize.cy+2;

  ExtTextOut(hDC,
    x, y,
    ETO_OPAQUE, &brect, Value, size, NULL);

  SelectObject(hDC, hbOld);

}


void FlyDirectTo(int index) {
  ActiveWayPoint = -1; AATEnabled = FALSE;
  for(int j=0;j<MAXTASKPOINTS;j++)
  {
    Task[j].Index = -1;
  }
  Task[0].Index = index;
  ActiveWayPoint = 0;
}


// Inserts a waypoint into the task, in the
// position of the ActiveWaypoint
void InsertWaypoint(int index) {
  int i;

  if (ActiveWayPoint<0) {
    ActiveWayPoint = 0;
    Task[ActiveWayPoint].Index = index;
    return;
  }

  if (Task[MAXTASKPOINTS-1].Index != -1) {
    // No room for any more task points!
    MessageBox(hWndMapWindow,
      TEXT("Too many waypoints in task!"),
      TEXT("Insert Waypoint"),
      MB_OK|MB_ICONEXCLAMATION);

    return;
  }

  // Shuffle ActiveWaypoint and all later task points
  // to the right by one position
  for (i=MAXTASKPOINTS-1; i>ActiveWayPoint; i--) {
    Task[i].Index = Task[i-1].Index;

  }

  // Insert new point and update task details
  Task[ActiveWayPoint].Index = index;
  RefreshTaskWaypoint(ActiveWayPoint+1);
  RefreshTaskWaypoint(ActiveWayPoint);

  CalculateTaskSectors();
  CalculateAATTaskSectors();
}


// RemoveTaskpoint removes a single waypoint
// from the current task.  index specifies an entry
// in the Task[] array - NOT a waypoint index.
//
// If you call this function, you MUST deal with
// correctly setting ActiveWayPoint yourself!
void RemoveTaskPoint(int index) {

  int i;

  if (index < 0 || index >= MAXTASKPOINTS) {
    return; // index out of bounds

  }

  if (Task[index].Index == -1) {
    return; // There's no WP at this location
  }

  // Shuffle all later taskpoints to the left to
  // fill the gap
  for (i=index; i<MAXTASKPOINTS-1; ++i) {
    Task[i].Index = Task[i+1].Index;
  }
  Task[MAXTASKPOINTS-1].Index = -1;

  // Only need to refresh info where the removal happened
  // as the order of other taskpoints hasn't changed
  if (Task[index].Index != -1) {
    RefreshTaskWaypoint(index);
  }

  CalculateTaskSectors();
  CalculateAATTaskSectors();

}


// Index specifies a waypoint in the WP list
// It won't necessarily be a waypoint that's
// in the task
void RemoveWaypoint(int index) {
  int i;

  if (ActiveWayPoint<0) {
    return; // No waypoint to remove
  }

  // Check to see whether selected WP is actually
  // in the task list.
  // If not, we'll ask the user if they want to remove
  // the currently active task point.
  // If the WP is in the task multiple times then we'll
  // remove the first instance after (or including) the
  // active WP.
  // If they're all before the active WP then just remove
  // the nearest to the active WP

  // Search forward first
  i = ActiveWayPoint;
  while (i < MAXTASKPOINTS && Task[i].Index != index) {
    ++i;
  }

  if (i < MAXTASKPOINTS) {
    // Found WP, so remove it
    RemoveTaskPoint(i);

    if (Task[ActiveWayPoint].Index == -1) {
      // We've just removed the last task point and it was
      // active at the time
      ActiveWayPoint--;
    }

  } else {
    // Didn't find WP, so search backwards

    i = ActiveWayPoint;
    do {
      --i;
    } while (i >= 0 && Task[i].Index != index);

    if (i >= 0) {
      // Found WP, so remove it
      RemoveTaskPoint(i);
      ActiveWayPoint--;

    } else {
      // WP not found, so ask user if they want to
      // remove the active WP
      int ret = MessageBox(hWndMapWindow,
        TEXT("Chosen Waypoint not in current task.\nRemove active WayPoint?"),
        TEXT("Remove Waypoint"),
        MB_YESNO|MB_ICONQUESTION);

      if (ret == IDYES) {
        RemoveTaskPoint(ActiveWayPoint);
        if (Task[ActiveWayPoint].Index == -1) {
          // Active WayPoint was last in the list so is currently
          // invalid.
          ActiveWayPoint--;
        }
      }
    }
  }
}


void ReplaceWaypoint(int index) {

  // ARH 26/06/05 Fixed array out-of-bounds bug
  if (ActiveWayPoint>=0) {

    Task[ActiveWayPoint].Index = index;
    RefreshTaskWaypoint(ActiveWayPoint);

    if (ActiveWayPoint>0) {
      RefreshTaskWaypoint(ActiveWayPoint-1);
    }

    if (ActiveWayPoint+1<MAXTASKPOINTS) {
      if (Task[ActiveWayPoint+1].Index != -1) {
        RefreshTaskWaypoint(ActiveWayPoint+1);
      }
    }

    CalculateTaskSectors();
    CalculateAATTaskSectors();

  } else {

    // Insert a new waypoint since there's
    // nothing to replace
    ActiveWayPoint=0;
    Task[ActiveWayPoint].Index = index;

  }
}



RECT MapRectBig;
RECT MapRect;
RECT MapRectSmall;
static bool MapFullScreen= false;
bool RequestFullScreen = false;

bool MapDirty = false;
bool RequestMapDirty = false;
bool RequestFastRefresh = false;

static DWORD fpsTime0=0;

void RefreshMap() {
  fpsTime0 = 0;
  RequestMapDirty = true;
}


void ToggleFullScreenStart() {

  // ok, save the state.
  MapFullScreen = RequestFullScreen;

  // show infoboxes immediately

  if (MapFullScreen) {
    MapRect = MapRectBig;
    HideInfoBoxes();
  } else {
    MapRect = MapRectSmall;
    ShowInfoBoxes();
  }
}


void RequestToggleFullScreen() {
  RequestFullScreen = !RequestFullScreen;
  RefreshMap();
}


LRESULT CALLBACK MapWndProc (HWND hWnd, UINT uMsg, WPARAM wParam,
                             LPARAM lParam)
{
  int i;
  //  TCHAR szMessageBuffer[1024];
  double X,Y;
  static double Xstart, Ystart;
  static double XstartScreen, YstartScreen;
  double distance;
  char val;
  static bool first = true;

  static DWORD dwDownTime=0, dwUpTime=0;

  switch (uMsg)
  {
  case WM_ERASEBKGND:
    // JMW trying to reduce flickering
    if (first || MapDirty) {
      first = false;
      MapDirty = true;
      return (DefWindowProc (hWnd, uMsg, wParam, lParam));
    } else
      return TRUE;
  case WM_SIZE:
    hDrawBitMap = CreateCompatibleBitmap (hdcScreen, (int) LOWORD (lParam), (int) HIWORD (lParam));
    SelectObject(hdcDrawWindow, (HBITMAP)hDrawBitMap);
    hDrawBitMapBg = CreateCompatibleBitmap (hdcScreen, (int) LOWORD (lParam), (int) HIWORD (lParam));
    SelectObject(hdcDrawWindowBg, (HBITMAP)hDrawBitMapBg);
    hDrawBitMapTmp = CreateCompatibleBitmap (hdcScreen, (int) LOWORD (lParam), (int) HIWORD (lParam));
    SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);
    break;

  case WM_CREATE:
    hdcScreen = GetDC(hWnd);
    hdcDrawWindow = CreateCompatibleDC(hdcScreen);
    hdcDrawWindowBg = CreateCompatibleDC(hdcScreen);
    hDCTemp = CreateCompatibleDC(hdcDrawWindow);

    hBackgroundBrush = CreateSolidBrush(BackgroundColor);

    hTerrainWarning=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TERRAINWARNING));
    hLandable=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LANDABLE));
    hReachable=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REACHABLE));
    hTurnPoint=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TURNPOINT));
    hSmall=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SMALL));
    hCruise=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CRUISE));
    hClimb=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMB));
    hFinalGlide=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FINALGLIDE));
    hAutoMcReady=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AUTOMCREADY));

    // airspace brushes and colours

    hAirspaceBitmap[0]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE0));
    hAirspaceBitmap[1]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE1));
    hAirspaceBitmap[2]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE2));

    for (i=0; i<NUMAIRSPACEBRUSHES; i++) {
      hAirspaceBrushes[i] =
        CreatePatternBrush((HBITMAP)hAirspaceBitmap[i]);
    }

    /* JMW created all re-used pens here */

    hpAircraft = (HPEN)CreatePen(PS_SOLID, 3, RGB(0xa0,0xa0,0xa0));
    hpAircraftBorder = (HPEN)CreatePen(PS_SOLID, 3, RGB(0x00,0x00,0x00));
    hpWind = (HPEN)CreatePen(PS_SOLID, 2, RGB(255,0,0));
    hpWindThick = (HPEN)CreatePen(PS_SOLID, 4, RGB(255,220,220));
    hpBearing = (HPEN)CreatePen(PS_SOLID, 2, RGB(0,0,0));
    hpBestCruiseTrack = (HPEN)CreatePen(PS_SOLID, 1, RGB(0,0,255));
    #ifdef MONOCHROME_SCREEN
    hpCompass = (HPEN)CreatePen(PS_SOLID, 1, RGB(0x00,0x00,0x00));
    #else
    hpCompass = (HPEN)CreatePen(PS_SOLID, 1, RGB(0xcf,0xcf,0xFF));
    #endif
    hpThermalBand = (HPEN)CreatePen(PS_SOLID, 2, RGB(0x40,0x40,0xFF));
    hpThermalBandGlider = (HPEN)CreatePen(PS_SOLID, 2, RGB(0x00,0x00,0x30));
    hpFinalGlideBelow = (HPEN)CreatePen(PS_SOLID, 1, RGB(0xFF,0xA0,0xA0));
    hpFinalGlideAbove = (HPEN)CreatePen(PS_SOLID, 1, RGB(0xA0,0xFF,0xA0));
    hpMapScale = (HPEN)CreatePen(PS_SOLID, 1, RGB(0,0,0));
    hpTerrainLine = (HPEN)CreatePen(PS_DASH, 1, RGB(0x30,0x30,0x30));

    #ifdef MONOCHROME_SCREEN
    hbCompass=(HBRUSH)CreateSolidBrush(RGB(0xff,0xff,0xff));
    #else
    hbCompass=(HBRUSH)CreateSolidBrush(RGB(0x40,0x40,0xFF));
    #endif
    hbThermalBand=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0xFF));
    hbBestCruiseTrack=(HBRUSH)CreateSolidBrush(RGB(0x0,0x0,0xFF));
    hbFinalGlideBelow=(HBRUSH)CreateSolidBrush(RGB(0xFF,0x00,0x00));
    hbFinalGlideAbove=(HBRUSH)CreateSolidBrush(RGB(0x00,0xFF,0x00));

    //JMWFOO

    break;

  case WM_DESTROY:
    CloseDrawingThread();

    ReleaseDC(hWnd, hdcScreen);
    DeleteDC(hdcDrawWindow);
    DeleteDC(hdcDrawWindowBg);
    DeleteDC(hDCTemp);
    DeleteObject(hDrawBitMap);
    DeleteObject(hDrawBitMapBg);

    DeleteObject(hLandable);
    DeleteObject(hReachable);
    DeleteObject(hTurnPoint);
    DeleteObject(hSmall);
    DeleteObject(hCruise);
    DeleteObject(hClimb);
    DeleteObject(hFinalGlide);
    DeleteObject(hAutoMcReady);
    DeleteObject(hTerrainWarning);

    DeleteObject((HPEN)hpAircraft);
    DeleteObject((HPEN)hpAircraftBorder);
    DeleteObject((HPEN)hpWind);
    DeleteObject((HPEN)hpWindThick);
    DeleteObject((HPEN)hpBearing);
    DeleteObject((HPEN)hpBestCruiseTrack);
    DeleteObject((HPEN)hpCompass);
    DeleteObject((HPEN)hpThermalBand);
    DeleteObject((HPEN)hpThermalBandGlider);
    DeleteObject((HPEN)hpFinalGlideAbove);
    DeleteObject((HPEN)hpFinalGlideBelow);
    DeleteObject((HPEN)hpMapScale);
    DeleteObject((HPEN)hpTerrainLine);

    DeleteObject((HBRUSH)hbCompass);
    DeleteObject((HBRUSH)hbThermalBand);
    DeleteObject((HBRUSH)hbBestCruiseTrack);
    DeleteObject((HBRUSH)hbFinalGlideBelow);
    DeleteObject((HBRUSH)hbFinalGlideAbove);

    for(i=0;i<NUMAIRSPACEBRUSHES;i++)
    {
      DeleteObject(hAirspaceBrushes[i]);
      DeleteObject(hAirspaceBitmap[i]);
    }
    PostQuitMessage (0);
    break;

  case WM_LBUTTONDBLCLK:
    // Added by ARH to show menu button when mapwindow is
    // double clicked.
    ShowMenu();
    break;

  case WM_LBUTTONDOWN:
    dwDownTime = GetTickCount();
    Xstart = LOWORD(lParam); Ystart = HIWORD(lParam);
    XstartScreen = Xstart;
    YstartScreen = Ystart;
    GetLocationFromScreen(&Xstart, &Ystart);
    FullScreen();
    break;

  case WM_LBUTTONUP:
    X = LOWORD(lParam); Y = HIWORD(lParam);
    if(InfoWindowActive)
    {
      InfoWindowActive = FALSE;
      SetFocus(hWnd);
      FocusOnWindow(InfoFocus,false);
      break;
    }
    dwUpTime = GetTickCount(); dwDownTime = dwUpTime - dwDownTime;

    distance = isqrt4((long)((XstartScreen-X)*(XstartScreen-X)+
                      (YstartScreen-Y)*(YstartScreen-Y)));

    GetLocationFromScreen(&X, &Y);

    if (EnablePan && (distance>36)) {
      PanX += Xstart-X;
      PanY += Ystart-Y;
      RefreshMap();
      break; // disable picking when in pan mode
    } else {
#ifdef _SIM_
      if (distance>36) {
        double newbearing = Bearing(Ystart, Xstart, Y, X);
        GPS_INFO.TrackBearing = newbearing;
        GPS_INFO.Speed = min(100,distance/3);
      }
#endif
    }

    if(dwDownTime < 1000)
    {
      i=FindNearestWayPoint(Xstart, Ystart, MapScale * 500);
      if(i != -1)
      {

        SelectedWaypoint = i;
        PopupWaypointDetails();

        SetFocus(hWnd);
        SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);

        break;
      }
    }
    else
    {
      i= FindAirspaceCircle(Xstart,Ystart);
      if(i != -1)
      {
        DisplayAirspaceWarning(AirspaceCircle[i].Type , AirspaceCircle[i].Name , AirspaceCircle[i].Base, AirspaceCircle[i].Top );
        break;
      }
      i= FindAirspaceArea(Xstart,Ystart);
      if(i != -1)
      {
        DisplayAirspaceWarning(AirspaceArea[i].Type , AirspaceArea[i].Name , AirspaceArea[i].Base, AirspaceArea[i].Top );
        break;
      }
    }
    break;

  case WM_KEYUP:
    switch (wParam)
    {
    case VK_DOWN :  // SCROLL UP
      if (!Debounce(wParam)) break;
      RequestMapScale *= 1.414;
      if(RequestMapScale>160) RequestMapScale = 160;
      RefreshMap();
      break;

    case VK_UP: // SCROLL DOWN
      if (!Debounce(wParam)) break;
      if(RequestMapScale >= 0.01)
      {
        RequestMapScale /= 1.414;
      }
      RefreshMap();
      break;

    case VK_RIGHT: // Pan mode
      if (!Debounce(wParam)) break;
      EnablePan = !EnablePan;

      if (EnableSoundModes) {
        if (EnablePan) {
          PlayResource(TEXT("IDR_INSERT"));
        } else {
          PlayResource(TEXT("IDR_REMOVE"));
        }
      }

      // ARH Let the user know what's happening
      if (EnablePan)
        ShowStatusMessage(TEXT("Pan mode ON"), 1500);
      else
        ShowStatusMessage(TEXT("Pan mode OFF"), 1500);

      if (!EnablePan) {
        PanX = 0.0;
        PanY = 0.0;
        RefreshMap();
      }
      break;

    case VK_RETURN: // Pan mode, cycles through modes
      if (!Debounce(wParam)) break;

      if (ClearAirspaceWarnings()) {
        // airspace was active, enter was used to acknowledge
        break;
      }

      val = 0;
      val += (EnableTopology)*0x01;
      val += (EnableTerrain)*0x02;

      val++;
      if (val>3) val=0;

      if (EnableSoundModes) {
        if (val>0) {
          PlayResource(TEXT("IDR_INSERT"));
        } else {
          PlayResource(TEXT("IDR_REMOVE"));
        }
      }

      EnableTopology = (val & 0x01);
      EnableTerrain = (val & 0x02)>>1;

      // ARH Let user know what's happening
      TCHAR buf[128];

      if (EnableTopology)
        _stprintf(buf, TEXT("Topo: %s\r\n"), TEXT("ON"));
      else
        _stprintf(buf, TEXT("Topo: %s\r\n"), TEXT("OFF"));

      if (EnableTerrain)
        _stprintf(buf+_tcslen(buf), TEXT("Terrain: %s"), TEXT("ON"));
      else
        _stprintf(buf+_tcslen(buf), TEXT("Terrain: %s"), TEXT("OFF"));

      ShowStatusMessage(buf, 2500);
      //

      RefreshMap();
      break;

    case VK_LEFT:

      if (!Debounce(wParam)) break;
      AutoZoom = !AutoZoom;

      if (EnableSoundModes) {
        if (AutoZoom) {
          PlayResource(TEXT("IDR_INSERT"));
        } else {
          PlayResource(TEXT("IDR_REMOVE"));
        }
      }

      // ARH Let user know what's happening
      if (AutoZoom)
        ShowStatusMessage(TEXT("AutoZoom ON"), 1500);
      else
        ShowStatusMessage(TEXT("AutoZoom OFF"), 1500);


      if (AutoZoom) {
        EnablePan = false;
        PanX = 0.0;
        PanY = 0.0;
      }

      break;
      }
      break;
    }

    return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}

extern int FrameCount;




static void UpdateMapScale()
{
  static double AutoMapScale= RequestMapScale;
  static int AutoMapScaleWaypointIndex = -1;
  static double StartingAutoMapScale=0.0;
  double AutoZoomFactor;

  bool useraskedforchange = false;

  // if there is user intervention in the scale
  if(MapScale != RequestMapScale)
  {
    MapScale = RequestMapScale;

    DrawScale = MapScale/DISTANCEMODIFY;
    DrawScale = DrawScale/111000;
    DrawScale = 30/DrawScale;

    useraskedforchange = true;

    //      fpsTime0 = 0; // trigger immediate screen update
  }

  if (AutoZoom) {
    if(DerivedDrawInfo.WaypointDistance > 0)
    {

      if(
        (DisplayOrientation == NORTHUP)
        ||
        ((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == TRUE) )
        )
      {
        AutoZoomFactor = 2.5;
      }
      else
      {
        AutoZoomFactor = 4;
      }

      if(
         (DerivedDrawInfo.WaypointDistance
          < ( AutoZoomFactor * RequestMapScale / DISTANCEMODIFY))
         ||
         (StartingAutoMapScale==0.0))
      {
        // waypoint is too close, so zoom in
        // OR just turned waypoint

        // this is the first time this waypoint has gotten close,
        // so save original map scale

        if (StartingAutoMapScale==0.0) {
          StartingAutoMapScale = RequestMapScale;
        }

        // set scale exactly so that waypoint distance is the zoom factor
        // across the screen
        MapScale = DerivedDrawInfo.WaypointDistance * DISTANCEMODIFY
          / AutoZoomFactor;

        // limit zoomed in so doesn't reach silly levels
        if (MapScale<0.2) {
          MapScale = 0.2;
        }
        RequestMapScale = MapScale;

        // calculate scale factors for display etc.
        DrawScale = DerivedDrawInfo.WaypointDistance / AutoZoomFactor;
        DrawScale = DrawScale/111000;
        DrawScale = 30/DrawScale;


      }	else {

        if (useraskedforchange) {

          // user asked for a zoom change and it was achieved, so
          // reset starting map scale


          ////?          StartingAutoMapScale = MapScale;
        }

      }
    }
    //    fpsTime0 = 0; // Trigger immediate screen update
  } else {

    // reset starting map scale for auto zoom if momentarily switch
    // off autozoom
    //    StartingAutoMapScale = RequestMapScale;
  }


  // if we aren't looking at a waypoint, see if we are now
  if (AutoMapScaleWaypointIndex == -1) {
    if (ActiveWayPoint>=0) {
      AutoMapScaleWaypointIndex = Task[ActiveWayPoint].Index;
    }
  }

  // if there is an active waypoint
  if (ActiveWayPoint>=0) {

    // if the current zoom focused waypoint has changed...
    if (AutoMapScaleWaypointIndex != Task[ActiveWayPoint].Index) {
      AutoMapScaleWaypointIndex = Task[ActiveWayPoint].Index;

      // zoom back out to where we were before
      if (StartingAutoMapScale> 0.0) {
        RequestMapScale = StartingAutoMapScale;
      }

      // reset search for new starting zoom level
      StartingAutoMapScale = 0.0;
    }

  }

}


static void CalculateOrigin(RECT rc, POINT *Orig)
{
  bool GliderCenter=false;

  if( (DisplayOrientation == NORTHUP)
    ||
    (
    ((DisplayOrientation == NORTHCIRCLE)
    ||(DisplayOrientation==TRACKCIRCLE))
    && (DerivedDrawInfo.Circling == TRUE) )
    )
  {
    GliderCenter = TRUE;

    if (DisplayOrientation == TRACKCIRCLE) {
      DisplayAngle = DerivedDrawInfo.WaypointBearing;
      DisplayAircraftAngle = DrawInfo.TrackBearing-DisplayAngle;
    } else {
      DisplayAngle = 0.0;
      DisplayAircraftAngle = DrawInfo.TrackBearing;
    }

  } else {
    // normal, glider forward
    GliderCenter = FALSE;
    DisplayAngle = DrawInfo.TrackBearing;
    DisplayAircraftAngle = 0.0;

  }

  if(GliderCenter || EnablePan) {
    Orig->x = (rc.left + rc.right ) /2;
    Orig->y = (rc.bottom - rc.top) /2+rc.top;
  }
  else
  {
    Orig->x = (rc.left + rc.right ) /2;
    Orig->y = (rc.bottom - rc.top) - ((rc.bottom - rc.top )/5)+rc.top;
  }
}



static void RenderMapWindow(  RECT rc)
{
  bool drawmap;
  DWORD	fpsTime = ::GetTickCount();

  if (fpsTime0==0) {
    fpsTime0 = fpsTime;
  }

  if ((fpsTime-fpsTime0>1000)||(fpsTime==fpsTime0)) {
    fpsTime0 += 1000;
    drawmap = true;
  } else {
    drawmap = false;
  }

  //  int	dtms = max(0,1000-dTDisplay);
  // we basically sleep here so we're only updating 1 fps, which is what
  // the gps data comes in at anyway.
  //  Sleep(dtms);

  POINT Orig, Orig_Aircraft;

  CalculateOrigin(rc, &Orig);

  CalculateScreenPositions(Orig, rc, &Orig_Aircraft);
  CalculateScreenPositionsAirspace(Orig, rc, &Orig_Aircraft);

  if (drawmap) {
    CalculateWaypointReachable();

    HGDIOBJ Temp;

    // display border and fill background..

    if(InfoWindowActive) {
      Temp = SelectObject(hdcDrawWindowBg, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindowBg, GetStockObject(BLACK_PEN));
    }
    else {
      // JMW added light grey background
      Temp = SelectObject(hdcDrawWindowBg, hBackgroundBrush);
      SelectObject(hdcDrawWindowBg, GetStockObject(WHITE_PEN));
    }

    Rectangle(hdcDrawWindowBg,rc.left,rc.top,rc.right,rc.bottom);

    Temp = SelectObject(hdcDrawWindowBg, GetStockObject(BLACK_BRUSH));
    Temp = SelectObject(hdcDrawWindowBg, GetStockObject(BLACK_PEN));

    // ground first...

    LockTerrainDataGraphics();
    terrain_dem_graphics.SetCacheTime();
    UnlockTerrainDataGraphics();

    if (EnableTerrain) {
      double sunelevation = 40.0;
      double sunazimuth = -DerivedDrawInfo.WindBearing;
      sunazimuth+= DisplayAngle;
      DrawTerrain(hdcDrawWindowBg, rc, sunazimuth, sunelevation);
    }
    if (EnableTopology) {
      DrawTopology(hdcDrawWindowBg, rc);
    }

    // then airspace..
    DrawAirSpace(hdcDrawWindowBg, rc);

    DrawWaypoints(hdcDrawWindowBg,rc);

    if(TrailActive)
      DrawTrail(hdcDrawWindowBg, Orig_Aircraft, rc);

    // draw wind vector at aircraft
    if (!DerivedDrawInfo.Circling && (!EnablePan)) {
      DrawWindAtAircraft(hdcDrawWindowBg, Orig, rc);
    }

    if (FinalGlideTerrain && DerivedDrawInfo.TerrainValid) {
      DrawGlideThroughTerrain(hdcDrawWindowBg, rc);
    }

    if (TaskAborted) {
      DrawAbortedTask(hdcDrawWindowBg, rc, Orig_Aircraft);
    } else {
      DrawTask(hdcDrawWindowBg, rc);
    }

    DrawBestCruiseTrack(hdcDrawWindowBg, Orig_Aircraft);

    DrawBearing(hdcDrawWindowBg, Orig_Aircraft);

    // finally, draw you!

    DrawAircraft(hdcDrawWindowBg, Orig_Aircraft);

    // marks on top...
    DrawMarks(hdcDrawWindowBg, rc);

  }

  BitBlt(hdcDrawWindow, 0, 0, MapRectBig.right, MapRectBig.bottom,
    hdcDrawWindowBg, 0, 0, SRCCOPY);

  // overlays

  DrawCDI();

  DrawMapScale(hdcDrawWindow,rc);
  DrawMapScale2(hdcDrawWindow,rc, Orig_Aircraft);

  DrawCompass(hdcDrawWindow, rc);

  if (DerivedDrawInfo.Circling || EnablePan) {
    DrawWind(hdcDrawWindow, Orig, rc); // JMW shouldn't need Orig here
  }

  DrawFlightMode(hdcDrawWindow, rc);

  DrawFinalGlide(hdcDrawWindow,rc);

  DrawThermalBand(hdcDrawWindow, rc);

}



DWORD DrawThread (LPVOID lpvoid)
{

  MapScale = RequestMapScale;
  DrawScale = MapScale/DISTANCEMODIFY;
  DrawScale = DrawScale/111000;
  DrawScale = 30/DrawScale;

  //  THREADRUNNING = FALSE;

  GetClientRect(hWndMapWindow, &MapRectBig);

  MapRectSmall = MapRect;

  MapRect = MapRectSmall;

  SetBkMode(hdcDrawWindow,TRANSPARENT);
  SetBkMode(hdcDrawWindowBg,TRANSPARENT);
  SetBkMode(hDCTemp,OPAQUE);

  // paint draw window white
  SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
  Rectangle(hdcDrawWindow,MapRectBig.left,MapRectBig.top,
            MapRectBig.right,MapRectBig.bottom);

  //  SelectObject(hdcDrawWindow, hAirspaceBitmap[0]);

  BitBlt(hdcScreen, 0, 0, MapRectBig.right-MapRectBig.left,
         MapRectBig.bottom-MapRectBig.top,
         hdcDrawWindow, 0, 0, SRCCOPY);

  //////

  ////// This is just here to give fully rendered start screen
  LockFlightData();
  memcpy(&DrawInfo,&GPS_INFO,sizeof(NMEA_INFO));
  memcpy(&DerivedDrawInfo,&CALCULATED_INFO,sizeof(DERIVED_INFO));
  UnlockFlightData();

  UpdateMapScale();
  RenderMapWindow(MapRect);
  SetTopologyBounds(MapRect);

  // display end of starting splash stuff
  CloseProgressDialog();

  FullScreen();

  //////

  while (!CLOSETHREAD)
  {
    if (!THREADRUNNING) {
      Sleep(10);
      continue;
    }
    if (!MapDirty && !RequestFastRefresh) {
      Sleep(10);
      continue;
    }

    // draw previous frame so screen is immediately refreshed
    BitBlt(hdcScreen, 0, 0, MapRectBig.right-MapRectBig.left,
           MapRectBig.bottom-MapRectBig.top,
       hdcDrawWindow, 0, 0, SRCCOPY);

    if (RequestFullScreen != MapFullScreen) {
      ToggleFullScreenStart();
    }

    if (MapDirty) {
      MapDirty = false;
    } else {
      if (RequestFastRefresh) {
        RequestFastRefresh = false;
        continue;
      }
    }

    LockFlightData();
    memcpy(&DrawInfo,&GPS_INFO,sizeof(NMEA_INFO));
    memcpy(&DerivedDrawInfo,&CALCULATED_INFO,sizeof(DERIVED_INFO));
    UnlockFlightData();

    UpdateMapScale();

    RenderMapWindow(MapRect);

    BitBlt(hdcScreen, 0, 0, MapRectBig.right-MapRectBig.left,
	     MapRectBig.bottom-MapRectBig.top,
       hdcDrawWindow, 0, 0, SRCCOPY);

    FrameCount ++;

    // we do caching after screen update, to minimise perceived delay
    SetTopologyBounds(MapRect);

  }
  MessageBeep(0);
  return 0;
}


void DrawAircraft(HDC hdc, POINT Orig)
{
  POINT Aircraft[7] = { {-15,0}, {15,0}, {0,-6}, {0,12}, {-4,10}, {4,10}, {0,0} };
  double dX,dY;
  int i;
  HPEN hpOld;

  hpOld = (HPEN)SelectObject(hdc, hpAircraft);

  for(i=0;i<7;i++)
  {

// JMW now corrects displayed aircraft heading for wind

    dX = (double)Aircraft[i].x ;dY = (double)Aircraft[i].y;
    rotate(&dX, &dY, DisplayAircraftAngle+
           (DerivedDrawInfo.Heading-DrawInfo.TrackBearing)
           );

    Aircraft[i].x =iround(dX+Orig.x)+1;  Aircraft[i].y = iround(dY+Orig.y)+1;
  }

  DrawSolidLine(hdc,Aircraft[2],Aircraft[3]);
  DrawSolidLine(hdc,Aircraft[4],Aircraft[5]);
  DrawSolidLine(hdc,Aircraft[0],Aircraft[6]);
  DrawSolidLine(hdc,Aircraft[1],Aircraft[6]);

  SelectObject(hdc, hpOld);

  // draw it again so can get white border
  SelectObject(hdc, hpAircraftBorder);

  for(i=0;i<7;i++)
  {

    Aircraft[i].x -= 1;  Aircraft[i].y -= 1;
  }

  DrawSolidLine(hdc,Aircraft[2],Aircraft[3]);
  DrawSolidLine(hdc,Aircraft[4],Aircraft[5]);
  DrawSolidLine(hdc,Aircraft[0],Aircraft[6]);
  DrawSolidLine(hdc,Aircraft[1],Aircraft[6]);

  SelectObject(hdc, hpOld);
}


void DrawBitmapIn(HDC hdc, int x, int y, HBITMAP h) {
  SelectObject(hDCTemp, h);
  BitBlt(hdc,x-5,y-5,10,10,
    hDCTemp,0,0,SRCPAINT);
  BitBlt(hdc,x-5,y-5,10,10,
    hDCTemp,10,0,SRCAND);
}


void DrawFlightMode(HDC hdc, RECT rc)
{

  if (DerivedDrawInfo.Circling) {
    SelectObject(hDCTemp,hClimb);
  } else if (DerivedDrawInfo.FinalGlide) {
    SelectObject(hDCTemp,hFinalGlide);
  } else {
    SelectObject(hDCTemp,hCruise);
  }
  //		BitBlt(hdc,rc.right-35,5,24,20,
  //				 hDCTemp,20,0,SRCAND);
  BitBlt(hdc,rc.right-24-3,rc.bottom-20-3,24,20,
    hDCTemp,0,0,SRCAND);

  if (DerivedDrawInfo.AutoMcReady) {
    SelectObject(hDCTemp,hAutoMcReady);
    BitBlt(hdc,rc.right-48-3,rc.bottom-20-3,24,20,
      hDCTemp,0,0,SRCAND);
  };

}


bool WaypointInTask(int ind) {
  int i;
  if( (WayPointList[ind].Flags & HOME) == HOME) {
    return true;
  }
  for(i=0;i<MAXTASKPOINTS;i++)
  {
    if(Task[i].Index == ind) return true;
  }
  if (ind == HomeWaypoint) {
    return true;
  }
  return false;
}


void DrawWaypoints(HDC hdc, RECT rc)
{
  unsigned int i;
  TCHAR Buffer[10];

  for(i=0;i<NumberOfWayPoints;i++)
  {
    if(WayPointList[i].Visible )
    {
      if(MapScale > 20)
      {
        SelectObject(hDCTemp,hSmall);
      }
      else if( ((WayPointList[i].Flags & AIRPORT) == AIRPORT) || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) )
      {
        if(WayPointList[i].Reachable)
          SelectObject(hDCTemp,hReachable);
        else
          SelectObject(hDCTemp,hLandable);
      }
      else
      {
        SelectObject(hDCTemp,hTurnPoint);
      }

      if((WayPointList[i].Zoom >= MapScale*10) || (WayPointList[i].Zoom == 0))
      {
        BitBlt(hdc,WayPointList[i].Screen.x-10 , WayPointList[i].Screen.y-10,20,20,
          hDCTemp,0,0,SRCPAINT);
        BitBlt(hdc,WayPointList[i].Screen.x-10 , WayPointList[i].Screen.y-10,20,20,
          hDCTemp,20,0,SRCAND);

      }

      // JMW
      if (DisplayTextType == DISPLAYNAMEIFINTASK) {

        if (WaypointInTask(i)) {

          TextInBox(hdc, WayPointList[i].Name, WayPointList[i].Screen.x+5,
            WayPointList[i].Screen.y, 0);
        }

      } else

        if( ((WayPointList[i].Zoom >= MapScale*10) || (WayPointList[i].Zoom == 0)) && (MapScale <= 10))
        {
          switch(DisplayTextType)
          {
          case DISPLAYNAME:

            TextInBox(hdc, WayPointList[i].Name, WayPointList[i].Screen.x+5,
              WayPointList[i].Screen.y, 0);

            break;
          case DISPLAYNUMBER:
            wsprintf(Buffer, TEXT("%d"),WayPointList[i].Number);

            TextInBox(hdc, Buffer, WayPointList[i].Screen.x+5,
              WayPointList[i].Screen.y, 0);

            break;
          case DISPLAYFIRSTFIVE:

            TextInBox(hdc, WayPointList[i].Name, WayPointList[i].Screen.x+5,
              WayPointList[i].Screen.y, 5);

            break;
          case DISPLAYFIRSTTHREE:
            TextInBox(hdc, WayPointList[i].Name, WayPointList[i].Screen.x+5,
              WayPointList[i].Screen.y, 3);
            break;
          }
        }
    }
  }
}


void DrawAbortedTask(HDC hdc, RECT rc, POINT me)
{
  int i;

  for(i=0;i<MAXTASKPOINTS-1;i++)
  {
    if(Task[i].Index >=0)
    {
      DrawDashLine(hdc, 1,
        WayPointList[Task[i].Index].Screen,
        me,
        RGB(0,255,0));
    }
  }
}


void DrawTask(HDC hdc, RECT rc)
{
  int i;
  double tmp;

  COLORREF whitecolor = RGB(0xff,0xff, 0xff);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  for(i=0;i<MAXTASKPOINTS-1;i++)
  {
    if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
    {
      DrawDashLine(hdc, 3,
        WayPointList[Task[i].Index].Screen,
        WayPointList[Task[i+1].Index].Screen,
        RGB(0,255,0));
    }
  }

  if((Task[0].Index >=0) &&  (Task[1].Index >=0))
  {
    if(StartLine)
    {
      DrawDashLine(hdc, 2, WayPointList[Task[0].Index].Screen, Task[0].End, RGB(127,127,127));
      DrawDashLine(hdc, 2, WayPointList[Task[0].Index].Screen, Task[0].Start , RGB(127,127,127));
    }
    tmp = StartRadius*DISTANCEMODIFY/MapScale; tmp = tmp * 30;
    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));SelectObject(hdc, GetStockObject(BLACK_PEN));
    Circle(hdc,WayPointList[Task[0].Index].Screen.x,WayPointList[Task[0].Index].Screen.y,(int)tmp, rc);
  }

  for(i=1;i<MAXTASKPOINTS-1;i++)
  {
    if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
    {
      if(AATEnabled == TRUE)
      {
        if(Task[i].AATType == CIRCLE)
        {
          tmp = Task[i].AATCircleRadius * DISTANCEMODIFY/MapScale;
          tmp = tmp * 30;

          // this color is used as the black bit
          SetTextColor(hDCTemp,
                   Colours[iAirspaceColour[AATASK]]);

          // this color is the transparent bit
          SetBkColor(hDCTemp,
                     whitecolor);

          SelectObject(hdc, hAirspaceBrushes[iAirspaceBrush[AATASK]]);
          SelectObject(hdc, GetStockObject(BLACK_PEN));

          Circle(hdc,WayPointList[Task[i].Index].Screen.x,WayPointList[Task[i].Index].Screen.y,(int)tmp, rc);
        }
        else
        {

          // this color is used as the black bit
          SetTextColor(hDCTemp,
                   Colours[iAirspaceColour[AATASK]]);

          // this color is the transparent bit
          SetBkColor(hDCTemp,
                     whitecolor);

          SelectObject(hdc, hAirspaceBrushes[iAirspaceBrush[AATASK]]);
          SelectObject(hdc, GetStockObject(BLACK_PEN));

          DrawSolidLine(hdc,WayPointList[Task[i].Index].Screen, Task[i].AATStart);
          DrawSolidLine(hdc,WayPointList[Task[i].Index].Screen, Task[i].AATFinish);
        }
      }
      else
      {
        DrawDashLine(hdc, 2, WayPointList[Task[i].Index].Screen, Task[i].Start, RGB(127,127,127));
        DrawDashLine(hdc, 2, WayPointList[Task[i].Index].Screen, Task[i].End, RGB(127,127,127));

        if(FAISector != TRUE)
        {
          tmp = SectorRadius*DISTANCEMODIFY/MapScale;
          tmp = tmp * 30;
          SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));SelectObject(hdc, GetStockObject(BLACK_PEN));
          Circle(hdc,WayPointList[Task[i].Index].Screen.x,WayPointList[Task[i].Index].Screen.y,(int)tmp, rc);
        }
      }
    }
  }

  // restore original color
  SetTextColor(hDCTemp, origcolor);


}



void DrawWindAtAircraft(HDC hdc, POINT Orig, RECT rc) {
  double dX,dY;
  int i, j;
  POINT Start;
  HPEN hpOld;
  int iwind;
  int koff;

  if (DerivedDrawInfo.WindSpeed<0.5) {
    return; // JMW don't bother drawing it if not significant
  }

  hpOld = (HPEN)SelectObject(hdc, hpWind);

  int wmag = iround(10.0*DerivedDrawInfo.WindSpeed);
  int numvecs;

  numvecs = (wmag/52)+1;
  for (j=0; j<numvecs; j++) {
    if (j== numvecs-1) {
      iwind = wmag % 52;
    } else {
      iwind = 52;
    }

    Start.y = Orig.y;
    Start.x = Orig.x;
    POINT Arrow[4] = { {0,7}, {0,0}, {-5,0}, {5,0} };

    koff = 5*(2*j-(numvecs-1));
    Arrow[0].y += iwind/2;
    Arrow[1].y -= 0;
    Arrow[2].y += iwind/2;
    Arrow[3].y += iwind/2;
    Arrow[0].x += koff;
    Arrow[1].x += koff;
    Arrow[2].x += koff;
    Arrow[3].x += koff;

    /* OLD WIND
    POINT Arrow[4] = { {0,-15}, {0,-35}, {-5,-22}, {5,-22} };
    Start.x = Orig.x;
    Start.y = Orig.y;
    Arrow[1].y =(long)( -15 - 5 * DerivedDrawInfo.WindSpeed );
    */

    // JMW TODO: if wind is stronger than 10 knots, draw two arrowheads

    for(i=0;i<4;i++)
    {
      dX = (double)Arrow[i].x ;dY = (double)(Arrow[i].y-iwind/2-25);
      rotate(&dX, &dY, -1*(DisplayAngle - DerivedDrawInfo.WindBearing));

      Arrow[i].x = iround(Start.x+dX);  Arrow[i].y = iround(dY+Start.y);

    }

    SelectObject(hdc, hpWindThick);

    DrawSolidLine(hdc,Arrow[0],Arrow[1]);
    DrawSolidLine(hdc,Arrow[0],Arrow[2]);
    DrawSolidLine(hdc,Arrow[0],Arrow[3]);

    SelectObject(hdc, hpWind);

    DrawSolidLine(hdc,Arrow[0],Arrow[1]);
    DrawSolidLine(hdc,Arrow[0],Arrow[2]);
    DrawSolidLine(hdc,Arrow[0],Arrow[3]);

  }

  SelectObject(hdc, hpOld);
}


void DrawWind(HDC hdc, POINT Orig, RECT rc)
{
  double dX,dY;
  int i, j;
  POINT Start;
  HPEN hpOld;
  int iwind;
  int koff;

  if (DerivedDrawInfo.WindSpeed<0.5) {
    return; // JMW don't bother drawing it if not significant
  }

  hpOld = (HPEN)SelectObject(hdc, hpWind);

  int wmag = iround(10.0*DerivedDrawInfo.WindSpeed);
  int numvecs;

  numvecs = (wmag/52)+1;
  for (j=0; j<numvecs; j++) {
    if (j== numvecs-1) {
      iwind = wmag % 52;
    } else {
      iwind = 52;
    }

    Start.y = 19+rc.top;
    Start.x = rc.right - 19;
    POINT Arrow[4] = { {0,7}, {0,0}, {-5,0}, {5,0} };

    koff = 5*(2*j-(numvecs-1));

    Arrow[0].y += iwind/2;
    Arrow[1].y -= 0;
    Arrow[2].y += iwind/2;
    Arrow[3].y += iwind/2;
    Arrow[0].x += koff;
    Arrow[1].x += koff;
    Arrow[2].x += koff;
    Arrow[3].x += koff;

    /* OLD WIND
    POINT Arrow[4] = { {0,-15}, {0,-35}, {-5,-22}, {5,-22} };
    Start.x = Orig.x;
    Start.y = Orig.y;
    Arrow[1].y =(long)( -15 - 5 * DerivedDrawInfo.WindSpeed );
    */

    // JMW TODO: if wind is stronger than 10 knots, draw two arrowheads

    for(i=0;i<4;i++)
    {
      dX = (double)Arrow[i].x ;dY = (double)Arrow[i].y;
      rotate(&dX, &dY, -1*(DisplayAngle - DerivedDrawInfo.WindBearing));

      Arrow[i].x = iround(Start.x+dX);  Arrow[i].y = iround(dY+Start.y);

    }

    SelectObject(hdc, hpWindThick);

    DrawSolidLine(hdc,Arrow[0],Arrow[1]);
    DrawSolidLine(hdc,Arrow[0],Arrow[2]);
    DrawSolidLine(hdc,Arrow[0],Arrow[3]);

    SelectObject(hdc, hpWind);

    DrawSolidLine(hdc,Arrow[0],Arrow[1]);
    DrawSolidLine(hdc,Arrow[0],Arrow[2]);
    DrawSolidLine(hdc,Arrow[0],Arrow[3]);

  }

  SelectObject(hdc, hpOld);
}


void DrawBearing(HDC hdc, POINT Orig)
{
  POINT Start, End;
  HPEN hpOld;

  hpOld = (HPEN)SelectObject(hdc, hpBearing);

  if(ActiveWayPoint >= 0)
  {
    Start.x = WayPointList[Task[ActiveWayPoint].Index].Screen.x;
    Start.y = WayPointList[Task[ActiveWayPoint].Index].Screen.y;
    End.x = Orig.x;
    End.y = Orig.y;
    DrawSolidLine(hdc, Start, End);
  }

  SelectObject(hdc, hpOld);
}


POINT Orig_Screen;


// JMW TODO: This should have orig saved so it's faster
// actually, orig gets calculated with displayorientation everywhere
// and it is inefficient.

// RETURNS Longitude, Latitude!

void GetLocationFromScreen(double *X, double *Y)
{

  *X = (*X-Orig_Screen.x)/DrawScale;
  *Y = (*Y-Orig_Screen.y)/DrawScale;

  rotate(X,Y,DisplayAngle);

  *Y = (PanYr)  - *Y;

  *X = *X / (double)ffastcosine((float)*Y);

  *X = (PanXr) + *X;
}


void DrawMapScale(HDC hDC, RECT rc)
{
  TCHAR Scale[20];
  POINT Start, End;
  HPEN hpOld;

  hpOld = (HPEN)SelectObject(hDC, hpMapScale);

  Start.x = rc.right-6; End.x = rc.right-6;
  Start.y = rc.bottom-30; End.y = Start.y - 30;
  DrawSolidLine(hDC,Start,End);

  Start.x = rc.right-11; End.x = rc.right-6;
  End.y = Start.y;
  DrawSolidLine(hDC,Start,End);

  Start.y = Start.y - 30; End.y = Start.y;
  DrawSolidLine(hDC,Start,End);

  SelectObject(hDC, hpOld);

  if(MapScale <0.1)
  {
    wsprintf(Scale,TEXT("%1.2f"),MapScale);
  }
  else if(MapScale <3)
  {
    wsprintf(Scale,TEXT("%1.1f"),MapScale);
  }
  else
  {
    wsprintf(Scale,TEXT("%1.0f"),MapScale);
  }
  if (AutoZoom) {
    wcscat(Scale,TEXT(" A"));
  }
  if (EnablePan) {
    wcscat(Scale,TEXT(" PAN"));
  }

  SIZE tsize;
  GetTextExtentPoint(hDC, Scale, _tcslen(Scale), &tsize);

  COLORREF whitecolor = RGB(0xd0,0xd0, 0xd0);
  COLORREF blackcolor = RGB(0x20,0x20, 0x20);
  COLORREF origcolor = SetTextColor(hDC, whitecolor);

  SetTextColor(hDC, blackcolor);

  ExtTextOut(hDC, rc.right-11-tsize.cx, End.y+8, 0, NULL, Scale, _tcslen(Scale), NULL);

  SetTextColor(hDC, whitecolor);
  ExtTextOut(hDC, rc.right-10-tsize.cx, End.y+7, 0, NULL, Scale, _tcslen(Scale), NULL);

  // restore original color
  SetTextColor(hDC, origcolor);

  // JMW for debugging
  /*
  wsprintf(Scale,TEXT("%d"), terraincacheefficiency);
  ExtTextOut(hDC, 20, End.y+20, 0, NULL, Scale, _tcslen(Scale), NULL);

  //////////////

  hpOld = (HPEN)SelectObject(hDC, hpBearing);

  Start.x = rc.right-1;
  Start.y = rc.bottom;
  End.x = rc.right-1;
  End.y = (rc.bottom-rc.top)*terrain_dem_graphics.
    terraincacheefficiency/100+rc.top;
  DrawSolidLine(hDC, Start, End);

  */

  SelectObject(hDC, hpOld);


}


void DrawGlideThroughTerrain(HDC hDC, RECT rc) {
  POINT Groundline[NUMTERRAINSWEEPS+1];
  double lat, lon;
  double distance;
  double bearing;
  int scx, scy;
  HPEN hpOld;

  hpOld = (HPEN)SelectObject(hDC, hpTerrainLine);

  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    bearing = -90+i*180.0/NUMTERRAINSWEEPS+DrawInfo.TrackBearing;
    distance = FinalGlideThroughTerrain(bearing, &DrawInfo,
      &DerivedDrawInfo, &lat, &lon);
    LatLon2Screen(lon, lat, &scx, &scy);
    Groundline[i].x = scx;
    Groundline[i].y = scy;
  }
  Polyline(hDC, Groundline, NUMTERRAINSWEEPS+1);

  if ((DerivedDrawInfo.TerrainWarningLattitude != 0.0)
    &&(DerivedDrawInfo.TerrainWarningLongditude != 0.0)) {

    LatLon2Screen(DerivedDrawInfo.TerrainWarningLongditude,
      DerivedDrawInfo.TerrainWarningLattitude, &scx, &scy);
    DrawBitmapIn(hDC, scx, scy, hTerrainWarning);

  }

  SelectObject(hDC, hpOld);

}

void DrawBestCruiseTrack(HDC hdc, POINT Orig)
{
  POINT Arrow[7] = { {-1,-40}, {1,-40}, {1,0}, {6,8}, {-6,8}, {-1,0}, {-1,-40}};
  double dX,dY;
  int i;
  HPEN hpOld;
  HBRUSH hbOld;

  if (ActiveWayPoint<0) {
    return; // nothing to draw..
  }

  hpOld = (HPEN)SelectObject(hdc, hpBestCruiseTrack);
  hbOld = (HBRUSH)SelectObject(hdc, hbBestCruiseTrack);

  int dy = (long)(70); //  DerivedDrawInfo.WindSpeed );

  Arrow[2].y -= dy;
  Arrow[3].y -= dy;
  Arrow[4].y -= dy;
  Arrow[5].y -= dy;

  for(i=0;i<7;i++)
  {
    dX = (double)Arrow[i].x ;dY = (double)Arrow[i].y;
    rotate(&dX, &dY, -1*(DisplayAngle - DerivedDrawInfo.BestCruiseTrack));

    Arrow[i].x = iround(dX+Orig.x);  Arrow[i].y = iround(dY+Orig.y);
  }


  Polygon(hdc,Arrow,7);

  SelectObject(hdc, hpOld);
  SelectObject(hdc, hbOld);
}


void DrawCompass(HDC hDC,RECT rc)
{
  //	TCHAR Scale[5];
  POINT Start;
  HPEN hpOld;
  HBRUSH hbOld;

  Start.y = 19+rc.top;
  Start.x = rc.right - 19;

  POINT Arrow[5] = { {0,-18}, {-6,10}, {0,0}, {6,10}, {0,-18}};
  double dX,dY;
  int i;

  hpOld = (HPEN)SelectObject(hDC, hpCompass);
  hbOld = (HBRUSH)SelectObject(hDC, hbCompass);

  for(i=0;i<5;i++)
  {
    dX = (double)Arrow[i].x ;dY = (double)Arrow[i].y;
    rotate(&dX, &dY, -1*DisplayAngle);
    Arrow[i].x = iround(dX+Start.x);  Arrow[i].y = iround(dY+Start.y);

  }

  Polygon(hDC,Arrow,5);

  /*
  wsprintf(Scale,TEXT("%1.2f"), DrawInfo.TrackBearing);

    SetBkMode(hDC,TRANSPARENT);
    ExtTextOut(hDC, 10, End.y+7, 0, NULL, Scale, _tcslen(Scale), NULL);
  */

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);

}


void DrawAirSpace(HDC hdc, RECT rc)
{
  unsigned i,j;
  POINT pt[501];
  COLORREF whitecolor = RGB(0xff,0xff, 0xff);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);

  if (bAirspaceBlackOutline) {
    SelectObject(hDCTemp, GetStockObject(BLACK_PEN));
  } else {
    SelectObject(hDCTemp, GetStockObject(WHITE_PEN));
  }

  SelectObject(hDCTemp, GetStockObject(WHITE_BRUSH));
  Rectangle(hDCTemp,rc.left,rc.top,rc.right,rc.bottom);

  for(i=0;i<NumberOfAirspaceCircles;i++)
  {
    if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude,
                             AirspaceCircle[i].Top.Altitude))
    {

      // this color is used as the black bit
      SetTextColor(hDCTemp,
                   Colours[iAirspaceColour[AirspaceCircle[i].Type]]);
      // this color is the transparent bit
      SetBkColor(hDCTemp,
                 whitecolor);

      // get brush, can be solid or a 1bpp bitmap
      SelectObject(hDCTemp,
                   hAirspaceBrushes[iAirspaceBrush[AirspaceCircle[i].Type]]);

      AirspaceCircle[i].Visible =
        Circle(hDCTemp,AirspaceCircle[i].ScreenX ,
        AirspaceCircle[i].ScreenY ,
        AirspaceCircle[i].ScreenR ,rc);

    } else {
      // JMW think this was missing before
      // NOPE, NOT A BUG
      AirspaceCircle[i].Visible = false;

    }
  }

  /////////

  for(i=0;i<NumberOfAirspaceAreas;i++)
  {
    if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, AirspaceArea[i].Top.Altitude))
    {
      for(j= AirspaceArea[i].FirstPoint; j < (AirspaceArea[i].NumPoints + AirspaceArea[i].FirstPoint); j++)
      {
        pt[j-AirspaceArea[i].FirstPoint].x = AirspacePoint[j].Screen.x ;
        pt[j-AirspaceArea[i].FirstPoint].y = AirspacePoint[j].Screen.y ;
      }



      AirspaceArea[i].Visible =
        msRectOverlap(&AirspaceArea[i].bounds, &screenbounds_latlon);
      // JMW now looks for overlap in lat/lon coordinates

      if(AirspaceArea[i].Visible) {

        // this color is used as the black bit
        SetTextColor(hDCTemp,
                     Colours[iAirspaceColour[AirspaceArea[i].Type]]);
        // this color is the transparent bit
        SetBkColor(hDCTemp,
                   whitecolor);

        SelectObject(hDCTemp,
                     hAirspaceBrushes[iAirspaceBrush[AirspaceArea[i].Type]]);

        Polygon(hDCTemp,pt,AirspaceArea[i].NumPoints);
      }
    } else {
      // JMW think this was missing before
      // NOPE, NOT A BUG
      AirspaceArea[i].Visible = false;
    }
  }

  SelectObject(hDCTemp,GetStockObject(HOLLOW_BRUSH));

  for(i=0;i<NumberOfAirspaceCircles;i++)
  {
    if(AirspaceCircle[i].Visible)
    {
      if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude,
        AirspaceCircle[i].Top.Altitude))
      {

      // this color is used as the black bit
      SetTextColor(hDCTemp,
                   Colours[iAirspaceColour[AirspaceCircle[i].Type]]);

      // this color is the transparent bit
      SetBkColor(hDCTemp,
                 whitecolor);

        Circle(hDCTemp,
          AirspaceCircle[i].ScreenX, AirspaceCircle[i].ScreenY ,
          AirspaceCircle[i].ScreenR, rc );
      }
    }
  }

  // restore original color
  SetTextColor(hDCTemp, origcolor);


  for(i=0;i<NumberOfAirspaceAreas;i++)
  {
    if(AirspaceArea[i].Visible)
    {
      if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, AirspaceArea[i].Top.Altitude))
      {
        for(j= (int)AirspaceArea[i].FirstPoint; j < (int)(AirspaceArea[i].NumPoints+AirspaceArea[i].FirstPoint) ;j++)
        {
          pt[j-AirspaceArea[i].FirstPoint].x = AirspacePoint[j].Screen.x ;
          pt[j-AirspaceArea[i].FirstPoint].y = AirspacePoint[j].Screen.y ;
        }
        Polygon(hDCTemp,pt,AirspaceArea[i].NumPoints);
      }
    }
  }

  BitBlt(hdcDrawWindowBg,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
	   hDCTemp,rc.left,rc.top,SRCAND);

}


void CreateDrawingThread(void)
{
  CLOSETHREAD = FALSE;
  hDrawThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE )DrawThread, 0, 0, &dwDrawThreadID);
  SetThreadPriority(hDrawThread,THREAD_PRIORITY_NORMAL);
}

void SuspendDrawingThread(void)
{
  LockTerrainDataGraphics();
  THREADRUNNING = FALSE;
  UnlockTerrainDataGraphics();
  //  SuspendThread(hDrawThread);
}

void ResumeDrawingThread(void)
{
  LockTerrainDataGraphics();
  THREADRUNNING = TRUE;
  UnlockTerrainDataGraphics();
  //  ResumeThread(hDrawThread);
}

void CloseDrawingThread(void)
{
  LockTerrainDataGraphics();
  CLOSETHREAD = TRUE;
  SuspendDrawingThread();
  SetThreadPriority(hDrawThread,THREAD_PRIORITY_ABOVE_NORMAL);
  UnlockTerrainDataGraphics();
}

void DrawThermalBand(HDC hDC,RECT rc)
{
  POINT ThermalProfile[NUMTHERMALBUCKETS+2];
  POINT GliderBand[4] = { {2,0},{23,0},{22,0},{24,0} };

  if ((DerivedDrawInfo.TaskAltitudeDifference>50)||(DerivedDrawInfo.FinalGlide)) {
    return;
  }

  // JMW TODO: gather proper statistics
  // note these should/may also be relative to ground
  int i;
  double mth = DerivedDrawInfo.MaxThermalHeight-DerivedDrawInfo.TerrainAlt;
  double maxh;
  double h;
  double Wt[NUMTHERMALBUCKETS];
  double Wtot=0.0;
  int TBSCALEY = ( (rc.bottom - rc.top )/2)-30;
#define TBSCALEX 20

  // calculate height above safety altitude
  h = DrawInfo.Altitude-SAFETYALTITUDEBREAKOFF-DerivedDrawInfo.TerrainAlt;
  // calculate top height
  if (h>mth) {
    maxh = h;
  } else {
    maxh = mth;
  }
  if (h<0) {
    // JMW TODO: below safety height, maybe give warning here
    h=0;
    return;
  }

  if (maxh>mth) {
    // above highest thermal
    return;
  }
  // no thermalling has been done above safety altitude
  if (maxh<0) {
    return;
  }

  // calculate averages
  int numtherm = 0;
  for (i=0; i<NUMTHERMALBUCKETS; i++) {
    if (DerivedDrawInfo.ThermalProfileN[i]>10) {
      // now requires 10 items in bucket before displaying,
      // to eliminate kinks
      Wt[i] = DerivedDrawInfo.ThermalProfileW[i]/DerivedDrawInfo.ThermalProfileN[i];
      if (Wt[i]<0) {
        Wt[i]= 0.0;
      }
    } else {
      Wt[i] = 0.0;
    }
    if (Wt[i]>0) {
      numtherm++;
      Wtot += Wt[i];
    }
  }
  if (numtherm) {
    Wtot/= numtherm;
  } else {
    Wtot = 1.0;
  }

  double mc = MACREADY/LIFTMODIFY;

  // scale to mcready
  if (mc>0.5) {
    Wtot = mc;
  } else {
    // use whatever scale thermal average gives
  }

  // position of thermal band
  for (i=0; i<NUMTHERMALBUCKETS; i++) {
    ThermalProfile[1+i].x = 7+iround((Wt[i]/Wtot)*TBSCALEX);
    ThermalProfile[1+i].y = 5+iround(TBSCALEY*(1.0-(mth/maxh)*(i)/NUMTHERMALBUCKETS))+rc.top;
  }
  ThermalProfile[0].x = 7;
  ThermalProfile[0].y = ThermalProfile[1].y;
  ThermalProfile[NUMTHERMALBUCKETS+1].x = 7;
  ThermalProfile[NUMTHERMALBUCKETS+1].y = ThermalProfile[NUMTHERMALBUCKETS].y;


  // position of thermal band
  GliderBand[0].y = 5+iround(TBSCALEY*(1.0-h/maxh))+rc.top;
  GliderBand[1].y = GliderBand[0].y;
  GliderBand[2].y = GliderBand[0].y-4;
  GliderBand[3].y = GliderBand[0].y+4;
  GliderBand[1].x = 7+iround((mc/Wtot)*TBSCALEX);
  GliderBand[2].x = GliderBand[1].x-4;
  GliderBand[3].x = GliderBand[1].x-4;

  // drawing info
  HPEN hpOld;
  HBRUSH hbOld;

  hpOld = (HPEN)SelectObject(hDC, hpThermalBand);
  hbOld = (HBRUSH)SelectObject(hDC, hbThermalBand);

  Polygon(hDC,ThermalProfile,NUMTHERMALBUCKETS+2);

  (HPEN)SelectObject(hDC, hpThermalBandGlider);

  DrawSolidLine(hDC,GliderBand[0],GliderBand[1]);
  DrawSolidLine(hDC,GliderBand[1],GliderBand[2]);
  DrawSolidLine(hDC,GliderBand[1],GliderBand[3]);

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);

}


void DrawFinalGlide(HDC hDC,RECT rc)
{

  POINT Scale[18] = {
    {5,-50 }, {14,-60 }, {23, -50},
    {5,-40 }, {14,-50 }, {23, -40},
    {5,-30 }, {14,-40 }, {23, -30},
    {5,-20 }, {14,-30 }, {23, -20},
    {5,-10 }, {14,-20 }, {23, -10},
    {5, 0  }, {14,-10 }, {23,   0},
  };

  POINT	GlideBar[5] =
  { {5,0},{14,-10},{23,0},{23,0},{5,0} };

  HPEN hpOld;
  HBRUSH hbOld;

  TCHAR Value[10];

  double Offset;
  int i;

  if (ActiveWayPoint == -1) {
    return;
    // JMW not going anywhere, so nothing to display
  }

  Offset = DerivedDrawInfo.TaskAltitudeDifference / 300;	// JMW TODO: should be an angle if in final glide mode
  Offset *= 20;

  if(Offset > 60) Offset = 60;
  if(Offset < -60) Offset = -60;

  if (Offset<0) {
    hpOld = (HPEN)SelectObject(hDC, hpFinalGlideBelow);
    hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));
  } else {
    hpOld = (HPEN)SelectObject(hDC, hpFinalGlideAbove);
    hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));
  }

  if(Offset<0)
  {
    GlideBar[1].y = 10;
    (HBRUSH)SelectObject(hDC, hbFinalGlideBelow);
  }
  else
  {
    (HBRUSH)SelectObject(hDC, hbFinalGlideAbove);
  }

  for(i=0;i<5;i++)
  {
    GlideBar[i].y += ( (rc.bottom - rc.top )/2)+rc.top;
  }
  GlideBar[0].y -= (int)Offset;
  GlideBar[1].y -= (int)Offset;
  GlideBar[2].y -= (int)Offset;

  Polygon(hDC,GlideBar,5);

  wsprintf(Value,TEXT("%1.0f "),ALTITUDEMODIFY*DerivedDrawInfo.TaskAltitudeDifference);

  if (Offset>=0) {
    Offset = (GlideBar[2].y+Offset)+5;
  } else {
    Offset = (GlideBar[2].y+Offset)-15;
  }

  TextInBox(hDC, Value, GlideBar[0].x, (int)Offset, 0);

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);

}


#define NUMSNAILRAMP 3

COLORRAMP snail_colors[] = {
  {-5,          0xff, 0x50, 0x50},
  {0,           0x8f, 0x8f, 0x8f},
  {5,           0x50, 0xff, 0x50}
};


static int iSnailNext=0;


void DrawTrail( HDC hdc, POINT Orig, RECT rc)
{
  int i;
  int P1,P2;
  HPEN	hpNew, hpOld, hpDelete;
  BYTE Red,Green,Blue;
  int scx, scy;
  bool p1Visible = false;
  bool p2Visible = false;
  bool havep1 = true;

  if(!TrailActive)
    return;

  hpDelete = (HPEN)CreatePen(PS_SOLID, 2, RGB(0xFF,0xFF,0xFF));
  hpOld = (HPEN)SelectObject(hdc, hpDelete);

  // JMW don't draw first bit from home airport

  havep1 = false;

  for(i=0;i<TRAILSIZE;i++)
  {
    if( i < TRAILSIZE-1)
    {
      P1 = i; P2 = i+1;
    }
    else
    {
      P1 = i; P2 = 0;
    }

    if (P1 == 0) {
      // set first point up

      p1Visible = PointVisible(SnailTrail[P1].Longditude ,
                               SnailTrail[P1].Lattitude);
    }

    p2Visible = PointVisible(SnailTrail[P2].Longditude ,
                             SnailTrail[P2].Lattitude);

    // the line is invalid
    if ((P1 == iSnailNext) || (P2 == iSnailNext) ||
        (!p2Visible) || (!p1Visible)) {

      p1Visible = p2Visible;

      // p2 wasn't computed in screen coordinates, better do it next
      // time if required
      havep1 = false;
      continue;
    }

    // now we know both points are visible, better get screen coords
    // if we don't already.

    if (!havep1) {
      LatLon2Screen(SnailTrail[P1].Longditude,
                    SnailTrail[P1].Lattitude, &scx, &scy);
      SnailTrail[P1].Screen.x = scx;
      SnailTrail[P1].Screen.y = scy;
    } else {
      havep1 = false;
    }

    LatLon2Screen(SnailTrail[P2].Longditude,
                  SnailTrail[P2].Lattitude, &scx, &scy);
    SnailTrail[P2].Screen.x = scx;
    SnailTrail[P2].Screen.y = scy;
    havep1 = true; // next time our p1 will be in screen coords

    // shuffle visibility along
    p1Visible = p2Visible;

    // ok, we got this far, so draw the line

    ColorRampLookup((short)(SnailTrail[P1].Vario/1.5),
                    &Red, &Green, &Blue,
                    snail_colors, NUMSNAILRAMP);

    int width = min(8,max(2,(int)SnailTrail[P1].Vario));

    hpNew = (HPEN)CreatePen(PS_SOLID, width,
                            RGB((BYTE)Red,(BYTE)Green,(BYTE)Blue));
    SelectObject(hdc,hpNew);
    DeleteObject((HPEN)hpDelete);
    hpDelete = hpNew;

    DrawSolidLine(hdc,SnailTrail[P1].Screen,SnailTrail[P2].Screen);
  }

  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDelete);
}


bool PointVisible(double lon, double lat) {
  if ((lon> screenbounds_latlon.minx)&&(lon< screenbounds_latlon.maxx)
      && (lat>screenbounds_latlon.miny)&&(lat< screenbounds_latlon.maxy)) {
    return 1;
  } else {
    return 0;
  }
}


bool PointVisible(POINT *P, RECT *rc)
{
  if(	( P->x > rc->left )
    &&
    ( P->x < rc->right )
    &&
    ( P->y > rc->top  )
    &&
    ( P->y < rc->bottom  )
    )
    return TRUE;
  else
    return FALSE;
}


void DisplayAirspaceWarning(int Type, TCHAR *Name , AIRSPACE_ALT Base, AIRSPACE_ALT Top )
{
  TCHAR szMessageBuffer[1024];
  TCHAR szTitleBuffer[1024];

  FormatWarningString(Type, Name , Base, Top, szMessageBuffer, szTitleBuffer );

  ShowStatusMessage(szMessageBuffer, 7000, 25);

//  MessageBox(hWndMapWindow,szMessageBuffer ,szTitleBuffer,MB_OK|MB_ICONWARNING);
//  SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
}



void LatLon2Screen(float lon, float lat, int *scX, int *scY) {
  float X, Y;
  X = (float)DrawScale*((float)PanXr - lon)*ffastcosine(lat);
  Y = (float)DrawScale*((float)PanYr  - lat);

  frotate(&X, &Y, (float)DisplayAngle );

  *scX = Orig_Screen.x - iround(X);
  *scY = Orig_Screen.y + iround(Y);
}


void LatLon2Screen(double lon, double lat, int *scX, int *scY) {
  double X, Y;
  X = DrawScale*(PanXr - lon)*fastcosine(lat);
  Y = DrawScale*(PanYr  - lat);

  rotate(&X, &Y, DisplayAngle );

  *scX = Orig_Screen.x - (int)X; // iround(X);
  *scY = Orig_Screen.y + (int)Y; // iround(Y);
}



void CalculateScreenPositionsAirspace(POINT Orig, RECT rc, POINT *Orig_Aircraft) {
  unsigned int i,j;
  double tmp;
  int scx, scy;

  for(i=0;i<NumberOfAirspaceCircles;i++)
  {
    if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude, AirspaceCircle[i].Top.Altitude))
    {

      LatLon2Screen(AirspaceCircle[i].Longditude, AirspaceCircle[i].Lattitude, &scx, &scy);

      AirspaceCircle[i].ScreenX = scx;
      AirspaceCircle[i].ScreenY = scy;

      tmp = AirspaceCircle[i].Radius * DISTANCEMODIFY/MapScale;
      tmp = tmp * 30;

      AirspaceCircle[i].ScreenR = (int)tmp;
    }
  }


  for(i=0;i<NumberOfAirspaceAreas;i++)
  {
    if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, AirspaceArea[i].Top.Altitude))
    {
      for(j=AirspaceArea[i].FirstPoint ; j<(AirspaceArea[i].FirstPoint + AirspaceArea[i].NumPoints) ; j++)
      {

        // bug fix by Samuel Gisiger
        LatLon2Screen(AirspacePoint[j].Longditude, AirspacePoint[j].Lattitude, &scx, &scy);

        AirspacePoint[j].Screen.x = scx;
        AirspacePoint[j].Screen.y = scy;
      }
    }
  }

}


void CalculateScreenPositions(POINT Orig, RECT rc, POINT *Orig_Aircraft)
{
  unsigned int i;
  int scx, scy;

  // compute lat lon extents of visible screen
  screenbounds_latlon = GetRectBounds(rc);

  Orig_Screen = Orig;

  PanXr = DrawInfo.Longditude + PanX;
  PanYr = DrawInfo.Lattitude + PanY;

  LatLon2Screen(DrawInfo.Longditude, DrawInfo.Lattitude, &scx,
    &scy);
  Orig_Aircraft->x = scx;
  Orig_Aircraft->y = scy;

  // get screen coordinates for all task waypoints

  for (i=0; i<MAXTASKPOINTS; i++) {
    if (Task[i].Index>=0) {

      LatLon2Screen(WayPointList[Task[i].Index].Longditude,
                    WayPointList[Task[i].Index].Lattitude,
                    &scx, &scy);

      WayPointList[Task[i].Index].Screen.x = scx;
      WayPointList[Task[i].Index].Screen.y = scy;
    }

  }

  // only calculate screen coordinates for waypoints that are visible

  for(i=0;i<NumberOfWayPoints;i++)
  {

    if(PointVisible(WayPointList[i].Longditude, WayPointList[i].Lattitude) )
    {
      WayPointList[i].Visible = TRUE;

      LatLon2Screen(WayPointList[i].Longditude, WayPointList[i].Lattitude, &scx, &scy);

      WayPointList[i].Screen.x = scx;
      WayPointList[i].Screen.y = scy;

    }
    else
    {
      WayPointList[i].Visible = FALSE;
    }
  }


  if(TrailActive)
  {

    iSnailNext = SnailNext;
    // set this so that new data doesn't arrive between calculating
    // this and the screen updates

    /* don't bother doing this now! we are checking visibility from
       the lat/long.... faster
    for(i=0;i<TRAILSIZE;i++)
    {
      LatLon2Screen(SnailTrail[i].Longditude,
        SnailTrail[i].Lattitude, &scx, &scy);

      SnailTrail[i].Screen.x = scx;
      SnailTrail[i].Screen.y = scy;
    }
    */

  }

  for(i=0;i<MAXTASKPOINTS-1;i++)
  {
    if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
    {
      LatLon2Screen(Task[i].SectorEndLon, Task[i].SectorEndLat, &scx, &scy);

      Task[i].End.x  = scx;
      Task[i].End.y = scy;

      LatLon2Screen(Task[i].SectorStartLon, Task[i].SectorStartLat, &scx, &scy);

      Task[i].Start.x  = scx;
      Task[i].Start.y = scy;

      if((AATEnabled) && (Task[i].AATType == SECTOR))
      {
        LatLon2Screen(Task[i].AATStartLon, Task[i].AATStartLat, &scx, &scy);

        Task[i].AATStart.x  = scx;
        Task[i].AATStart.y = scy;

        LatLon2Screen(Task[i].AATFinishLon, Task[i].AATFinishLat, &scx, &scy);

        Task[i].AATFinish.x  = scx;
        Task[i].AATFinish.y = scy;

      }
    }
  }
}


void CalculateWaypointReachable(void)
{
  unsigned int i;
  double WaypointDistance, WaypointBearing,AltitudeRequired;

  for(i=0;i<NumberOfWayPoints;i++)
  {
    if(WayPointList[i].Visible )
    {
      if(  ((WayPointList[i].Flags & AIRPORT) == AIRPORT) || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) )
      {
        WaypointDistance = Distance(DrawInfo.Lattitude, DrawInfo.Longditude, WayPointList[i].Lattitude, WayPointList[i].Longditude);
        WaypointBearing =  Bearing(DrawInfo.Lattitude, DrawInfo.Longditude, WayPointList[i].Lattitude, WayPointList[i].Longditude);
        AltitudeRequired = McReadyAltitude(0.0, // JMW was MACREADY/LIFTMODIFY
          WaypointDistance,WaypointBearing,
          DerivedDrawInfo.WindSpeed,
          DerivedDrawInfo.WindBearing,0,0,1);
        AltitudeRequired = AltitudeRequired * (1/BUGS);
        AltitudeRequired = AltitudeRequired + SAFETYALTITUDEARRIVAL + WayPointList[i].Altitude ;
        AltitudeRequired = DrawInfo.Altitude - AltitudeRequired;

        if(AltitudeRequired >=0)
          WayPointList[i].Reachable = TRUE;
        else
          WayPointList[i].Reachable = FALSE;
      }
    }
  }
}


#define NUMPOINTS 2
void DrawSolidLine(HDC hdc, POINT ptStart, POINT ptEnd)
{
  POINT pt[2];

  pt[0].x = ptStart.x;
  pt[0].y = ptStart.y;
  pt[1].x = ptEnd.x;
  pt[1].y = ptEnd.y;
  Polyline(hdc, pt, NUMPOINTS);
}


void DrawDashLine(HDC hdc, INT width, POINT ptStart, POINT ptEnd, COLORREF cr)
{
  int i;
  HPEN hpDash,hpOld;
  POINT pt[2];
  //Create a dot pen
  hpDash = (HPEN)CreatePen(PS_DASH, 1, cr);
  hpOld = (HPEN)SelectObject(hdc, hpDash);

  pt[0].x = ptStart.x;
  pt[0].y = ptStart.y;
  pt[1].x = ptEnd.x;
  pt[1].y = ptEnd.y;

  //increment on smallest variance
  if(abs(ptStart.x - ptEnd.x) < abs(ptStart.y - ptEnd.y)){
    for (i = 0; i < width; i++){
      pt[0].x += 1;
      pt[1].x += 1;
      Polyline(hdc, pt, NUMPOINTS);
    }
  } else {
    for (i = 0; i < width; i++){
      pt[0].y += 1;
      pt[1].y += 1;
      Polyline(hdc, pt, NUMPOINTS);
    }
  }

  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDash);

}


void DrawCDI() {
  if (DerivedDrawInfo.Circling) {
    ShowWindow(hWndCDIWindow, SW_SHOW);

    // JMW changed layout here to fit reorganised display
    // insert waypoint bearing ".<|>." into CDIScale string"

    TCHAR CDIScale[] = TEXT("330..340..350..000..010..020..030..040..050..060..070..080..090..100..110..120..130..140..150..160..170..180..190..200..210..220..230..240..250..260..270..280..290..300..310..320..330..340..350..000..010..020..030..040.");
    TCHAR CDIDisplay[25] = TEXT("");
    int j;
    int CDI_WP_Bearing = (int)DerivedDrawInfo.WaypointBearing/2;
    CDIScale[CDI_WP_Bearing + 9] = 46;
    CDIScale[CDI_WP_Bearing + 10] = 60;
    CDIScale[CDI_WP_Bearing + 11] = 124; // "|" character
    CDIScale[CDI_WP_Bearing + 12] = 62;
    CDIScale[CDI_WP_Bearing + 13] = 46;
    for (j=0;j<24;j++) CDIDisplay[j] = CDIScale[(j + (int)(DrawInfo.TrackBearing)/2)];
    CDIDisplay[24] = NULL;
    // JMW fix bug! This indicator doesn't always display correctly!

    // JMW added arrows at end of CDI to point to track if way off..
    int deltacdi = iround(DerivedDrawInfo.WaypointBearing-DrawInfo.TrackBearing);

    while (deltacdi>180) {
      deltacdi-= 360;
    }
    while (deltacdi<-180) {
      deltacdi+= 360;
    }
    if (deltacdi>20) {
      CDIDisplay[21]='>';
      CDIDisplay[22]='>';
      CDIDisplay[23]='>';
    }
    if (deltacdi<-20) {
      CDIDisplay[0]='<';
      CDIDisplay[1]='<';
      CDIDisplay[2]='<';
    }

    SetWindowText(hWndCDIWindow,CDIDisplay);
    // end of new code to display CDI scale
  } else {
    ShowWindow(hWndCDIWindow, SW_HIDE);
  }
}



double findMapScaleBarSize(RECT rc) {

  int range = rc.bottom-rc.top;
  int nbars = 0;
  int nscale = 1;
  double pixelsize = MapScale/30; // km/pixel

  // find largest bar size that will fit in display

  double displaysize = range*pixelsize/2; // km

  if (displaysize>100.0) {
    return 100.0/pixelsize;
  }
  if (displaysize>10.0) {
    return 10.0/pixelsize;
  }
  if (displaysize>1.0) {
    return 1.0/pixelsize;
  }
  if (displaysize>0.1) {
    return 0.1/pixelsize;
  }
  // this is as far as is reasonable
  return 0.1/pixelsize;
}


void DrawMapScale2(HDC hDC, RECT rc, POINT Orig_Aircraft)
{

  double barsize = findMapScaleBarSize(rc);

//  TCHAR Scale[20]; Unused variable remm'd out RB

  HPEN hpOld;

  hpOld = (HPEN)SelectObject(hDC, hpMapScale);
  HPEN hpWhite = (HPEN)CreatePen(PS_SOLID, 2, RGB(0xd0,0xd0,0xd0));
  HPEN hpBlack = (HPEN)CreatePen(PS_SOLID, 2, RGB(0x30,0x30,0x30));

  double y;
  bool color = false;
  POINT Start, End;
  bool first=true;

  for (y=(double)Orig_Aircraft.y; y<(double)rc.bottom+barsize; y+= barsize) {
    if (color) {
      SelectObject(hDC, hpWhite);
    } else {
      SelectObject(hDC, hpBlack);
    }
    Start.x = rc.right-1;
    Start.y = (int)y;
    if (!first) {
      DrawSolidLine(hDC,Start,End);
    } else {
      first=false;
    }
    End = Start;
    color = !color;
  }

  color = true;
  first = true;
  for (y=(double)Orig_Aircraft.y; y>(double)rc.top-barsize; y-= barsize) {
    if (color) {
      SelectObject(hDC, hpWhite);
    } else {
      SelectObject(hDC, hpBlack);
    }
    Start.x = rc.right-1;
    Start.y = (int)y;
    if (!first) {
      DrawSolidLine(hDC,Start,End);
    } else {
      first=false;
    }
    End = Start;
    color = !color;
  }


  // draw text as before

  SelectObject(hDC, hpOld);
  DeleteObject(hpWhite);
  DeleteObject(hpBlack);

}
