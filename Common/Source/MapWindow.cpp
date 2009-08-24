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

#include "MapWindow.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Utils.h"
#include "UtilsAirspace.hpp"
#include "UtilsSystem.hpp"
#include "OnLineContest.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Units.h"
#include "McReady.h"
#include "Airspace.h"
#include "Waypointparser.h"
#include "Dialogs.h"
#include "Blackboard.hpp"
#include "Settings.hpp"
#include "SettingsTask.hpp"
#include "Audio/VarioSound.h"
#include "InputEvents.h"
#include "Trigger.hpp"
#include "Math/Geometry.hpp"
#include "Math/Screen.hpp"
#include "Screen/Blank.hpp"
#include "Screen/Util.hpp"
#include "Screen/Ramp.hpp"
#include "Compatibility/gdi.h"

#include "SettingsUser.hpp"
#include "Interface.hpp"

#include "Calculations.h" // TODO danger! for InAATTurnSector

#include <assert.h>
#include <windows.h>
#include <math.h>

#include <tchar.h>

#include "TopologyStore.h"
#include "TerrainRenderer.h"
#include "Marks.h"
#include "Task.h"

#include "GaugeVarioAltA.h"
#include "GaugeCDI.h"
#include "GaugeFLARM.h"
#include "InfoBoxLayout.h"
#include "InfoBoxManager.h"
#include "RasterTerrain.h"
#include "Utils2.h"

#if (WINDOWSPC>0)
#include <wingdi.h>
#endif

int misc_tick_count=0;

#ifndef NDEBUG
#define DRAWLOAD
#define DEBUG_VIRTUALKEYS
#endif

int TrailActive = TRUE;
int VisualGlide = 0;

extern void DrawGlideCircle(HDC hdc, POINT Orig, RECT rc );


#define NUMSNAILRAMP 6

static const COLORREF taskcolor = RGB(0,120,0); // was 255

const COLORRAMP snail_colors[] = {
  {0,         0xff, 0x3e, 0x00},
  {50,        0xcd, 0x4f, 0x27},
  {100,       0x8f, 0x8f, 0x8f},
  {150,       0x27, 0xcd, 0x4f},
  {201,       0x00, 0xff, 0x3e},
  {501,       0x00, 0xff, 0x3e}
};

///////////////////////////////// Initialisation

DisplayMode_t UserForceDisplayMode = dmNone;
DisplayMode_t DisplayMode = dmCruise;

HBITMAP MapWindow::hBmpAirportReachable;
HBITMAP MapWindow::hBmpAirportUnReachable;
HBITMAP MapWindow::hBmpFieldReachable;
HBITMAP MapWindow::hBmpFieldUnReachable;
HBITMAP MapWindow::hBmpThermalSource;
HBITMAP MapWindow::hBmpTarget;
HBITMAP MapWindow::hBmpTeammatePosition;
HBITMAP MapWindow::hAboveTerrainBitmap;
HBRUSH  MapWindow::hAboveTerrainBrush;

HPEN    MapWindow::hpCompassBorder;
HBRUSH  MapWindow::hBrushFlyingModeAbort;
int MapWindow::SnailWidthScale = 16;

HBITMAP MapWindow::hBmpUnitKm;
HBITMAP MapWindow::hBmpUnitSm;
HBITMAP MapWindow::hBmpUnitNm;
HBITMAP MapWindow::hBmpUnitM;
HBITMAP MapWindow::hBmpUnitFt;
HBITMAP MapWindow::hBmpUnitMpS;

int MapWindow::ScaleListCount = 0;
double MapWindow::ScaleList[];
int MapWindow::ScaleCurrent;
HBITMAP MapWindow::hBmpCompassBg = NULL;
HBITMAP MapWindow::hBmpClimbeAbort = NULL;
HBITMAP MapWindow::hBmpMapScale=NULL;

POINT MapWindow::Orig_Screen;

RECT MapWindow::MapRect;
RECT MapWindow::MapRectBig;
RECT MapWindow::MapRectSmall;

HBITMAP MapWindow::hDrawBitMap = NULL;
HBITMAP MapWindow::hDrawBitMapTmp = NULL;
HBITMAP MapWindow::hMaskBitMap = NULL;
HDC MapWindow::hdcDrawWindow = NULL;
HDC MapWindow::hdcScreen = NULL;
HDC MapWindow::hDCTemp = NULL;
HDC MapWindow::hDCMask = NULL;

rectObj MapWindow::screenbounds_latlon;

double MapWindow::PanLatitude = 0.0;
double MapWindow::PanLongitude = 0.0;

int MapWindow::TargetDrag_State = 0;
double MapWindow::TargetDrag_Latitude = 0;
double MapWindow::TargetDrag_Longitude = 0;

bool MapWindow::EnablePan = false;
bool MapWindow::TargetPan = false;
int MapWindow::TargetPanIndex = 0;
double MapWindow::TargetZoomDistance = 500.0;
bool MapWindow::EnableTrailDrift=false;
int MapWindow::GliderScreenPosition = 20; // 20% from bottom
int MapWindow::WindArrowStyle = 0;

BOOL MapWindow::CLOSETHREAD = FALSE;
BOOL MapWindow::THREADRUNNING = TRUE;
BOOL MapWindow::THREADEXIT = FALSE;
BOOL MapWindow::Initialised = FALSE;

bool MapWindow::BigZoom = true;
unsigned char MapWindow::DeclutterLabels = 0;

DWORD  MapWindow::dwDrawThreadID;
HANDLE MapWindow::hDrawThread;

double MapWindow::RequestMapScale=5;
double MapWindow::MapScale=5;
double MapWindow::MapScaleOverDistanceModify=5/DISTANCEMODIFY;
double MapWindow::ResMapScaleOverDistanceModify = 0.0;
double MapWindow::DisplayAngle = 0.0;
double MapWindow::DisplayAircraftAngle = 0.0;
double MapWindow::DrawScale;
double MapWindow::InvDrawScale;

bool MapWindow::AutoZoom = false;
bool MapWindow::LandableReachable = false;

HBITMAP MapWindow::hTurnPoint;
HBITMAP MapWindow::hSmall;
HBITMAP MapWindow::hCruise;
HBITMAP MapWindow::hClimb;
HBITMAP MapWindow::hFinalGlide;
HBITMAP MapWindow::hAutoMacCready;
HBITMAP MapWindow::hTerrainWarning;
HBITMAP MapWindow::hFLARMTraffic;
HBITMAP MapWindow::hGPSStatus1;
HBITMAP MapWindow::hGPSStatus2;
HBITMAP MapWindow::hAbort;
HBITMAP MapWindow::hLogger;
HBITMAP MapWindow::hLoggerOff;

HPEN MapWindow::hSnailPens[NUMSNAILCOLORS];
COLORREF MapWindow::hSnailColours[NUMSNAILCOLORS];

POINT MapWindow::Groundline[NUMTERRAINSWEEPS+1];

// 12 is number of airspace types
int      MapWindow::iAirspaceBrush[AIRSPACECLASSCOUNT] =
  {2,0,0,0,3,3,3,3,0,3,2,3,3,3};
int      MapWindow::iAirspaceColour[AIRSPACECLASSCOUNT] =
  {5,0,0,10,0,0,10,2,0,10,9,3,7,7};
int      MapWindow::iAirspaceMode[AIRSPACECLASSCOUNT] =
  {0,0,0,0,0,0,0,0,0,0,0,1,1,0};

HPEN MapWindow::hAirspacePens[AIRSPACECLASSCOUNT];
bool MapWindow::bAirspaceBlackOutline = false;

HBRUSH  MapWindow::hBackgroundBrush;

HBRUSH  MapWindow::hAirspaceBrushes[NUMAIRSPACEBRUSHES];
HBITMAP MapWindow::hAirspaceBitmap[NUMAIRSPACEBRUSHES];

COLORREF MapWindow::Colours[NUMAIRSPACECOLORS] =
  {RGB(0xFF,0x00,0x00), RGB(0x00,0xFF,0x00),
   RGB(0x00,0x00,0xFF), RGB(0xFF,0xFF,0x00),
   RGB(0xFF,0x00,0xFF), RGB(0x00,0xFF,0xFF),
   RGB(0x7F,0x00,0x00), RGB(0x00,0x7F,0x00),
   RGB(0x00,0x00,0x7F), RGB(0x7F,0x7F,0x00),
   RGB(0x7F,0x00,0x7F), RGB(0x00,0x7F,0x7F),
   RGB(0xFF,0xFF,0xFF), RGB(0xC0,0xC0,0xC0),
   RGB(0x7F,0x7F,0x7F), RGB(0x00,0x00,0x00)};


HBRUSH MapWindow::hbCompass;
HBRUSH MapWindow::hbThermalBand;
HBRUSH MapWindow::hbBestCruiseTrack;
HBRUSH MapWindow::hbFinalGlideBelow;
HBRUSH MapWindow::hbFinalGlideBelowLandable;
HBRUSH MapWindow::hbFinalGlideAbove;
HBRUSH MapWindow::hbWind;


HPEN MapWindow::hpAircraft;
HPEN MapWindow::hpAircraftBorder;
HPEN MapWindow::hpWind;
HPEN MapWindow::hpWindThick;
HPEN MapWindow::hpBearing;
HPEN MapWindow::hpBestCruiseTrack;
HPEN MapWindow::hpCompass;
HPEN MapWindow::hpThermalBand;
HPEN MapWindow::hpThermalBandGlider;
HPEN MapWindow::hpFinalGlideAbove;
HPEN MapWindow::hpFinalGlideBelow;
HPEN MapWindow::hpFinalGlideBelowLandable;
HPEN MapWindow::hpMapScale;
HPEN MapWindow::hpTerrainLine;
HPEN MapWindow::hpTerrainLineBg;
HPEN MapWindow::hpSpeedSlow;
HPEN MapWindow::hpSpeedFast;
HPEN MapWindow::hpStartFinishThick;
HPEN MapWindow::hpStartFinishThin;
HPEN MapWindow::hpVisualGlideLightBlack; // VENTA3
HPEN MapWindow::hpVisualGlideHeavyBlack; // VENTA3
HPEN MapWindow::hpVisualGlideLightRed; // VENTA3
HPEN MapWindow::hpVisualGlideHeavyRed; // VENTA3


COLORREF MapWindow::BackgroundColor = RGB(0xFF,0xFF,0xFF); //sjt 02NOV05 - was F5F5F5. Changed to increase screen clarity at oblique viewing angles.

bool MapWindow::MapDirty = true;
DWORD MapWindow::fpsTime0 = 0;
bool MapWindow::MapFullScreen = false;
bool MapWindow::RequestFullScreen = false;
bool MapWindow::ForceVisibilityScan = false;

/////////////////////////////////

NMEA_INFO MapWindow::DrawInfo;
DERIVED_INFO MapWindow::DerivedDrawInfo;

int SelectedWaypoint = -1;
bool EnableCDICruise = false;
bool EnableCDICircling = false;

extern int iround(double i);
extern void ShowMenu();

extern HFONT  TitleWindowFont;
extern HFONT  MapWindowFont;
extern HFONT  MapWindowBoldFont;
extern HFONT  InfoWindowFont;
extern HFONT  CDIWindowFont;
extern HFONT  StatisticsFont;
extern HFONT  MapLabelFont; // VENTA6
extern HFONT  TitleSmallWindowFont; // VENTA6


///////////////////

int timestats_av = 0;

DWORD MapWindow::timestamp_newdata=0;
int cpuload=0;

bool timestats_dirty=false;

void MapWindow::UpdateTimeStats(bool start) {
  static long tottime=0;
  if (start) {
    timestamp_newdata = ::GetTickCount();
    timestats_dirty = false;
  } else {
    if (!timestats_dirty) {
      DWORD time = ::GetTickCount();
      tottime = (2*tottime+(time-timestamp_newdata))/3;
      timestats_av = tottime;
      cpuload=0;
#ifdef DEBUG_MEM
      cpuload= MeasureCPULoad();
      DebugStore("%d # mem\n%d # latency\n", CheckFreeRam()/1024, timestats_av);
#endif
    }
    timestats_dirty = false;
  }
}


///////////////////


bool MapWindow::Event_NearestWaypointDetails(double lon, double lat,
                                             double range,
                                             bool pan) {
  /*
    if (!pan) {
    dlgWayPointSelect(lon, lat, 0, 1);
    } else {
    dlgWayPointSelect(PanLongitude, PanLatitude, 0, 1);
    }
  */

  int i;
  if (!pan || !EnablePan) {
    i=FindNearestWayPoint(lon, lat, range);
  } else {
    // nearest to center of screen if in pan mode
    i=FindNearestWayPoint(PanLongitude, PanLatitude, range);
  }
  if(i != -1)
    {
      SelectedWaypoint = i;
      PopupWaypointDetails();
      return true;
    }

  return false;
}


bool MapWindow::Event_InteriorAirspaceDetails(double lon, double lat) {
  unsigned int i;
  bool found=false;
  bool inside;

  if (AirspaceCircle) {
    for (i=0; i<NumberOfAirspaceCircles; i++) {
      inside = false;
      if (AirspaceCircle[i].Visible) {
        inside = InsideAirspaceCircle(lon, lat, i);
      }
      if (inside) {
	dlgAirspaceDetails(i, -1);

	/*
	  DisplayAirspaceWarning(AirspaceCircle[i].Type ,
	  AirspaceCircle[i].Name ,
	  AirspaceCircle[i].Base,
	  AirspaceCircle[i].Top );
	*/
        found = true;
      }
    }
  }
  if (AirspaceArea) {
    for (i=0; i<NumberOfAirspaceAreas; i++) {
      inside = false;
      if (AirspaceArea[i].Visible) {
        inside = InsideAirspaceArea(lon, lat, i);
      }
      if (inside) {
	dlgAirspaceDetails(-1, i);

	/*
	  DisplayAirspaceWarning(AirspaceArea[i].Type ,
	  AirspaceArea[i].Name ,
	  AirspaceArea[i].Base,
	  AirspaceArea[i].Top );
	*/
        found = true;
      }
    }
  }

  return found; // nothing found..
}


void MapWindow::SwitchZoomClimb(void) {

  //static double CruiseMapScale = 10;
  static double CruiseMapScale = MapWindow::RequestMapScale*2; // VNT 090621
  //static double ClimbMapScale = 0.25;
  static double ClimbMapScale = MapWindow::RequestMapScale/20;
  static bool last_isclimb = false;
  static bool last_targetpan = false;
  bool isclimb = (DisplayMode == dmCircling);

  if (TargetPan != last_targetpan) {
    if (TargetPan) {
      // save starting values
      if (isclimb) {
        ClimbMapScale = MapScale;
      } else {
        CruiseMapScale = MapScale;
      }
    } else {
      // restore scales
      if (isclimb) {
        RequestMapScale = LimitMapScale(ClimbMapScale);
      } else {
        RequestMapScale = LimitMapScale(CruiseMapScale);
      }
      BigZoom = true;
    }
    last_targetpan = TargetPan;
    return;
  }
  if (!TargetPan && CircleZoom) {
    if (isclimb != last_isclimb) {
      if (isclimb) {
        // save cruise scale
        CruiseMapScale = MapScale;
        // switch to climb scale
        RequestMapScale = LimitMapScale(ClimbMapScale);
      } else {
        // leaving climb
        // save cruise scale
        ClimbMapScale = MapScale;
        RequestMapScale = LimitMapScale(CruiseMapScale);
        // switch to climb scale
      }
      BigZoom = true;
      last_isclimb = isclimb;
    } else {
      // nothing to do.
    }
  }

}

bool MapWindow::isAutoZoom() {
  return AutoZoom;
}


bool userasked = false;

void MapWindow::RequestFastRefresh() {
  drawTriggerEvent.trigger();
}

void MapWindow::RefreshMap() {
  MapDirty = true;
  userasked = true;
  timestats_dirty = true;
  drawTriggerEvent.trigger();
}

bool MapWindow::IsMapFullScreen() {
  // SDP - Seems that RequestFullScreen
  // is always more accurate (MapFullSCreen is delayed)
  return RequestFullScreen;
  // return  MapFullScreen;
}


void MapWindow::ToggleFullScreenStart() {

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
  GaugeVario::Show(!MapFullScreen);

}


void MapWindow::RequestToggleFullScreen() {
  RequestFullScreen = !RequestFullScreen;
  RefreshMap();
}

void MapWindow::RequestOnFullScreen() {
  RequestFullScreen = true;
  RefreshMap();
}

void MapWindow::RequestOffFullScreen() {
  RequestFullScreen = false;
  RefreshMap();
}



extern BOOL extGPSCONNECT;
extern bool DialogActive;


///////////////////////////////////////////////////////////////////////////


void MapWindow::Event_AutoZoom(int vswitch) {
  if (vswitch== -1) {
    AutoZoom = !AutoZoom;
  } else {
    AutoZoom = (vswitch != 0); // 0 off, 1 on
  }

  if (AutoZoom) {
    if (EnablePan) {
      EnablePan = false;
      InputEvents::setMode(TEXT("default"));
      StoreRestoreFullscreen(false);
    }
  }
  RefreshMap();
}


void MapWindow::Event_PanCursor(int dx, int dy) {
  int X= (MapRect.right+MapRect.left)/2;
  int Y= (MapRect.bottom+MapRect.top)/2;
  double Xstart, Ystart, Xnew, Ynew;

  Screen2LatLon(X, Y, Xstart, Ystart);

  X+= (MapRect.right-MapRect.left)*dx/4;
  Y+= (MapRect.bottom-MapRect.top)*dy/4;
  Screen2LatLon(X, Y, Xnew, Ynew);

  if (EnablePan) {
    PanLongitude += Xstart-Xnew;
    PanLatitude += Ystart-Ynew;
  }
  RefreshMap();
}

bool MapWindow::isPan() {
  return EnablePan;
}

/* Event_TerrainToplogy Changes
   0       Show
   1       Toplogy = ON
   2       Toplogy = OFF
   3       Terrain = ON
   4       Terrain = OFF
   -1      Toggle through 4 stages (off/off, off/on, on/off, on/on)
   -2      Toggle terrain
   -3      Toggle toplogy
*/
void MapWindow::Event_TerrainTopology(int vswitch) {
  char val;

  if (vswitch== -1) { // toggle through 4 possible options
    val = 0;
    if (EnableTopology) val++;
    if (EnableTerrain) val += (char)2;
    val++;
    if (val>3) val=0;
    EnableTopology = ((val & 0x01) == 0x01);
    EnableTerrain  = ((val & 0x02) == 0x02);
    RefreshMap();

  } else if (vswitch == -2) { // toggle terrain
    EnableTerrain = !EnableTerrain;
    RefreshMap();

  } else if (vswitch == -3) { // toggle topology
    EnableTopology = !EnableTopology;
    RefreshMap();

  } else if (vswitch == 1) { // Turn on toplogy
    EnableTopology = true;
    RefreshMap();

  } else if (vswitch == 2) { // Turn off toplogy
    EnableTopology = false;
    RefreshMap();

  } else if (vswitch == 3) { // Turn on terrain
    EnableTerrain = true;
    RefreshMap();

  } else if (vswitch == 4) { // Turn off terrain
    EnableTerrain = false;
    RefreshMap();

  } else if (vswitch == 0) { // Show terrain/Topology
    // ARH Let user know what's happening
    TCHAR buf[128];

    if (EnableTopology)
      _stprintf(buf, TEXT("\r\n%s / "), gettext(TEXT("ON")));
    else
      _stprintf(buf, TEXT("\r\n%s / "), gettext(TEXT("OFF")));

    if (EnableTerrain)
      _stprintf(buf+_tcslen(buf), TEXT("%s"), gettext(TEXT("ON")));
    else
      _stprintf(buf+_tcslen(buf), TEXT("%s"), gettext(TEXT("OFF")));
    DoStatusMessage(TEXT("Topology / Terrain"), buf);
  }
}


void MapWindow::StoreRestoreFullscreen(bool store) {
  static bool oldfullscreen = 0;
  static bool SuperPan = false;
  if (store) {
    // pan not active on entry, save fullscreen status
    SuperPan = true;
    oldfullscreen = MapWindow::IsMapFullScreen();
  } else {
    if (SuperPan) {
      // pan is active, need to restore
      if (!oldfullscreen) {
        // change it if necessary
        RequestFullScreen = false;
      }
      SuperPan = false;
    }
  }
}



///////////////////////////////////////////////////////////////////////////


static void SetFontInfo(HDC hDC, FontHeightInfo_t *FontHeightInfo){
  TEXTMETRIC tm;
  int x,y=0;
  RECT  rec;
  int top, bottom;

  GetTextMetrics(hDC, &tm);
  FontHeightInfo->Height = tm.tmHeight;
  FontHeightInfo->AscentHeight = tm.tmAscent;
  FontHeightInfo->CapitalHeight = 0;

  SetBkMode(hDC, OPAQUE);
  SetBkColor(hDC,RGB(0xff,0xff,0xff));
  SetTextColor(hDC,RGB(0x00,0x00,0x00));
  rec.left = 0;
  rec.top = 0;
  rec.right = tm.tmAveCharWidth;
  rec.bottom = tm.tmHeight;
  ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rec, TEXT("M"), _tcslen(TEXT("M")), NULL);

  top = tm.tmHeight;
  bottom = 0;

  FontHeightInfo->CapitalHeight = 0;
  for (x=0; x<tm.tmAveCharWidth; x++){
    for (y=0; y<tm.tmHeight; y++){
      if ((GetPixel(hDC, x, y)) != RGB(0xff,0xff,0xff)){
        if (top > y)
          top = y;
        if (bottom < y)
          bottom = y;
      }
    }
  }

#ifdef GNAV
  // JMW: don't know why we need this in GNAV, but we do.
  if (FontHeightInfo->CapitalHeight<y)
    FontHeightInfo->CapitalHeight = bottom - top + 1;
#endif
  // This works for PPC
  if (FontHeightInfo->CapitalHeight <= 0)
    FontHeightInfo->CapitalHeight = tm.tmAscent - 1 -(tm.tmHeight/10);

  //  int lx = GetDeviceCaps(hDC,LOGPIXELSX);
  // dpi
}


LRESULT CALLBACK MapWindow::MapWndProc (HWND hWnd, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam)
{
  int i;
  static double Xstart, Ystart;
  static int XstartScreen, YstartScreen;
  int X,Y;
  double Xlat, Ylat;
  double distance;
  int width = (int) LOWORD(lParam);
  int height = (int) HIWORD(lParam);
  static bool ignorenext=true;

  static DWORD dwDownTime= 0L, dwUpTime= 0L, dwInterval= 0L;

  switch (uMsg)
    {
      /* JMW THIS IS BAD!  Now done with GCE_AIRSPACE
	 case WM_USER+1:
	 dlgAirspaceWarningShowDlg(false);
	 return(0);
      */
    case WM_ERASEBKGND:
      // JMW trying to reduce flickering
      /*
	if (first || MapDirty) {
	first = false;
	MapDirty = true;
	return (DefWindowProc (hWnd, uMsg, wParam, lParam));
	} else
	return TRUE;
      */
      return TRUE;
    case WM_SIZE:

      hDrawBitMap = CreateCompatibleBitmap (hdcScreen, width, height);
      SelectObject(hdcDrawWindow, (HBITMAP)hDrawBitMap);

      hDrawBitMapTmp = CreateCompatibleBitmap (hdcScreen, width, height);
      SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);

      hMaskBitMap = CreateBitmap(width+1, height+1, 1, 1, NULL);
      SelectObject(hDCMask, (HBITMAP)hMaskBitMap);

      {
	HFONT      oldFont;

	oldFont = (HFONT)SelectObject(hDCTemp, TitleWindowFont);
	SetFontInfo(hDCTemp, &Appearance.TitleWindowFont);

	SelectObject(hDCTemp, MapWindowFont);
	SetFontInfo(hDCTemp, &Appearance.MapWindowFont);

	SelectObject(hDCTemp, MapWindowBoldFont);
	SetFontInfo(hDCTemp, &Appearance.MapWindowBoldFont);

	SelectObject(hDCTemp, InfoWindowFont);
	SetFontInfo(hDCTemp, &Appearance.InfoWindowFont);

	SelectObject(hDCTemp, CDIWindowFont);
	SetFontInfo(hDCTemp, &Appearance.CDIWindowFont);
//VENTA6
	SelectObject(hDCTemp, StatisticsFont);
	SetFontInfo(hDCTemp, &Appearance.StatisticsFont);

	SelectObject(hDCTemp, MapLabelFont);
	SetFontInfo(hDCTemp, &Appearance.MapLabelFont);

	SelectObject(hDCTemp, TitleSmallWindowFont);
	SetFontInfo(hDCTemp, &Appearance.TitleSmallWindowFont);

	SelectObject(hDCTemp, oldFont);
      }

      break;

    case WM_CREATE:

      hdcScreen = GetDC(hWnd);
      hdcDrawWindow = CreateCompatibleDC(hdcScreen);
      hDCTemp = CreateCompatibleDC(hdcDrawWindow);
      hDCMask = CreateCompatibleDC(hdcDrawWindow);

      hBackgroundBrush = CreateSolidBrush(BackgroundColor);

      hFLARMTraffic=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FLARMTRAFFIC));
      hTerrainWarning=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TERRAINWARNING));
      hTurnPoint=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TURNPOINT));
      hSmall=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SMALL));
      hAutoMacCready=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AUTOMCREADY));
      hGPSStatus1=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GPSSTATUS1));
      hGPSStatus2=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GPSSTATUS2));
      hLogger=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGGER));
      hLoggerOff=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGGEROFF));
      hBmpTeammatePosition = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TEAMMATE_POS));

      //if (Appearance.FlightModeIcon == apFlightModeIconAltA){
	hCruise=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CRUISE));
	hClimb=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMB));
	hFinalGlide=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FINALGLIDE));
	hAbort=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ABORT));

      //hBmpCompassBg = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_COMPASSBG));


      // airspace brushes and colours

      hAirspaceBitmap[0]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE0));
      hAirspaceBitmap[1]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE1));
      hAirspaceBitmap[2]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE2));
      hAirspaceBitmap[3]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE3));
      hAirspaceBitmap[4]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE4));
      hAirspaceBitmap[5]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE5));
      hAirspaceBitmap[6]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE6));
      hAirspaceBitmap[7]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE7));

      hAboveTerrainBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ABOVETERRAIN));

      for (i=0; i<NUMAIRSPACEBRUSHES; i++) {
	hAirspaceBrushes[i] =
	  CreatePatternBrush((HBITMAP)hAirspaceBitmap[i]);
      }
      hAboveTerrainBrush = CreatePatternBrush((HBITMAP)hAboveTerrainBitmap);

      BYTE Red,Green,Blue;
      int iwidth;
      int minwidth;
      minwidth = max(IBLSCALE(2),IBLSCALE(SnailWidthScale)/16);
      for (i=0; i<NUMSNAILCOLORS; i++) {
	short ih = i*200/(NUMSNAILCOLORS-1);
	ColorRampLookup(ih,
			Red, Green, Blue,
			snail_colors, NUMSNAILRAMP, 6);
	if (i<NUMSNAILCOLORS/2) {
	  iwidth= minwidth;
	} else {
	  iwidth = max(minwidth,
		       (i-NUMSNAILCOLORS/2)
		       *IBLSCALE(SnailWidthScale)/NUMSNAILCOLORS);
	}

	hSnailColours[i] = RGB((BYTE)Red,(BYTE)Green,(BYTE)Blue);
	hSnailPens[i] = (HPEN)CreatePen(PS_SOLID, iwidth, hSnailColours[i]);

      }

      /* JMW created all re-used pens here */

      hpCompassBorder = (HPEN)CreatePen(PS_SOLID, IBLSCALE(3),
					RGB(0xff,0xff,0xff));

      // testing only    Appearance.InverseAircraft = true;

      if (Appearance.InverseAircraft) {
	hpAircraft = (HPEN)CreatePen(PS_SOLID, IBLSCALE(3),
				     RGB(0x00,0x00,0x00));
	hpAircraftBorder = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
					   RGB(0xff,0xff,0xff));
      } else {
	hpAircraft = (HPEN)CreatePen(PS_SOLID, IBLSCALE(3),
				     RGB(0xff,0xff,0xff));
	hpAircraftBorder = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
					   RGB(0x00,0x00,0x00));
      }

#if (MONOCHROME_SCREEN > 0)
      hpWind = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2), RGB(0,0,0));
#else
      hpWind = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2), RGB(255,0,0));
#endif

      hpWindThick = (HPEN)CreatePen(PS_SOLID, IBLSCALE(4),
				    RGB(255,220,220));

      hpBearing = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2),
				  RGB(0,0,0));
      hpBestCruiseTrack = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
					  RGB(0,0,255));
#if (MONOCHROME_SCREEN > 0)
      hpCompass = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(0x00,0x00,0x00));
      //hpCompass = (HPEN)CreatePen(PS_SOLID, 1, RGB(0xff,0xff,0xff));
#else
      hpCompass = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(0xcf,0xcf,0xFF));
#endif
      hpThermalBand = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2), RGB(0x40,0x40,0xFF));
      hpThermalBandGlider = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2), RGB(0x00,0x00,0x30));

      hpFinalGlideBelow = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(0xFF,0xA0,0xA0));
      hpFinalGlideBelowLandable = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(255,196,0));

      // TODO enhancement: support red/green Color blind
      hpFinalGlideAbove = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(0xA0,0xFF,0xA0));

      hpSpeedSlow=(HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
				  RGB(0xFF,0x00,0x00));
      hpSpeedFast=(HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
				  RGB(0x00,0xFF,0x00));

      hpStartFinishThick=(HPEN)CreatePen(PS_SOLID, IBLSCALE(5),
					 taskcolor);

      hpStartFinishThin=(HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
					RGB(255,0,0));

      hpMapScale = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
				   RGB(0,0,0));
      hpTerrainLine = (HPEN)CreatePen(PS_DASH, (1),
				      RGB(0x30,0x30,0x30));
      hpTerrainLineBg = (HPEN)CreatePen(PS_SOLID, (1),
					RGB(0xFF,0xFF,0xFF));
      // VENTA3
      hpVisualGlideLightBlack = (HPEN)CreatePen(PS_DASH, (1), RGB(0x0,0x0,0x0));
      hpVisualGlideHeavyBlack = (HPEN)CreatePen(PS_DASH, (2), RGB(0x0,0x0,0x0));
      hpVisualGlideLightRed = (HPEN)CreatePen(PS_DASH, (1), RGB(0xff,0x0,0x0));
      hpVisualGlideHeavyRed = (HPEN)CreatePen(PS_DASH, (2), RGB(0xff,0x0,0x0));

#if (MONOCHROME_SCREEN > 0)
      hbCompass=(HBRUSH)CreateSolidBrush(RGB(0xff,0xff,0xff));
#else
      hbCompass=(HBRUSH)CreateSolidBrush(RGB(0x40,0x40,0xFF));
#endif
      hbThermalBand=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0xFF));
      hbBestCruiseTrack=(HBRUSH)CreateSolidBrush(RGB(0x0,0x0,0xFF));
      hbFinalGlideBelow=(HBRUSH)CreateSolidBrush(RGB(0xFF,0x00,0x00));
      hbFinalGlideBelowLandable=(HBRUSH)CreateSolidBrush(RGB(0xFF,180,0x00));
      hbFinalGlideAbove=(HBRUSH)CreateSolidBrush(RGB(0x00,0xFF,0x00));


#if (MONOCHROME_SCREEN > 0)
      hbWind=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0x80));
#else
      hbWind=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0x80));
#endif

      ScaleListCount = propGetScaleList(ScaleList, sizeof(ScaleList)/sizeof(ScaleList[0]));
      RequestMapScale = LimitMapScale(RequestMapScale);

      hBmpMapScale = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_MAPSCALE_A));

      hBrushFlyingModeAbort = (HBRUSH)CreateSolidBrush(RGB(0xff,0x00,0x00));

      if (Appearance.IndLandable == wpLandableDefault){
	hBmpAirportReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REACHABLE));
	hBmpAirportUnReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LANDABLE));
	hBmpFieldReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REACHABLE));
	hBmpFieldUnReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LANDABLE));
      }else
	if (Appearance.IndLandable == wpLandableAltA){
	  hBmpAirportReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRPORT_REACHABLE));
	  hBmpAirportUnReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRPORT_UNREACHABLE));
	  hBmpFieldReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OUTFILED_REACHABLE));
	  hBmpFieldUnReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OUTFILED_UNREACHABLE));
	}

      hBmpThermalSource = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_THERMALSOURCE));
      hBmpTarget = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TARGET));

      // Signal that draw thread can run now
      Initialised = TRUE;

      break;

    case WM_DESTROY:

      ReleaseDC(hWnd, hdcScreen);
      DeleteDC(hdcDrawWindow);
      DeleteDC(hDCTemp);
      DeleteDC(hDCMask);
      DeleteObject(hDrawBitMap);
      DeleteObject(hMaskBitMap);

      DeleteObject(hTurnPoint);
      DeleteObject(hSmall);
      DeleteObject(hCruise);
      DeleteObject(hClimb);
      DeleteObject(hFinalGlide);
      DeleteObject(hAutoMacCready);
      DeleteObject(hFLARMTraffic);
      DeleteObject(hTerrainWarning);
      DeleteObject(hGPSStatus1);
      DeleteObject(hGPSStatus2);
      DeleteObject(hAbort);
      DeleteObject(hLogger);
      DeleteObject(hLoggerOff);

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
      DeleteObject((HPEN)hpFinalGlideBelowLandable);
      DeleteObject((HPEN)hpMapScale);
      DeleteObject((HPEN)hpTerrainLine);
      DeleteObject((HPEN)hpTerrainLineBg);
      DeleteObject((HPEN)hpSpeedFast);
      DeleteObject((HPEN)hpSpeedSlow);
      DeleteObject((HPEN)hpStartFinishThick);
      DeleteObject((HPEN)hpStartFinishThin);

      DeleteObject((HPEN)hpVisualGlideLightBlack); // VENTA3
      DeleteObject((HPEN)hpVisualGlideLightRed); // VENTA3
      DeleteObject((HPEN)hpVisualGlideHeavyRed); // VENTA3
      DeleteObject((HPEN)hpVisualGlideHeavyBlack); // VENTA3

      DeleteObject((HBRUSH)hbCompass);
      DeleteObject((HBRUSH)hbThermalBand);
      DeleteObject((HBRUSH)hbBestCruiseTrack);
      DeleteObject((HBRUSH)hbFinalGlideBelow);
      DeleteObject((HBRUSH)hbFinalGlideBelowLandable);
      DeleteObject((HBRUSH)hbFinalGlideAbove);
      DeleteObject((HBRUSH)hbWind);

      DeleteObject(hBmpMapScale);
      DeleteObject(hBmpCompassBg);
      DeleteObject(hBackgroundBrush);
      DeleteObject(hBmpClimbeAbort);

      DeleteObject((HPEN)hpCompassBorder);
      DeleteObject((HBRUSH)hBrushFlyingModeAbort);

      DeleteObject(hBmpAirportReachable);
      DeleteObject(hBmpAirportUnReachable);
      DeleteObject(hBmpFieldReachable);
      DeleteObject(hBmpFieldUnReachable);
      DeleteObject(hBmpThermalSource);
      DeleteObject(hBmpTarget);
      DeleteObject(hBmpTeammatePosition);

      for(i=0;i<NUMAIRSPACEBRUSHES;i++)
	{
	  DeleteObject(hAirspaceBrushes[i]);
	  DeleteObject(hAirspaceBitmap[i]);
	}

      DeleteObject(hAboveTerrainBitmap);
      DeleteObject(hAboveTerrainBrush);

      for (i=0; i<AIRSPACECLASSCOUNT; i++) {
	DeleteObject(hAirspacePens[i]);
      }

      for (i=0; i<NUMSNAILCOLORS; i++) {
	DeleteObject(hSnailPens[i]);
      }

      PostQuitMessage (0);

      break;

    case WM_LBUTTONDBLCLK:
      // Added by ARH to show menu button when mapwindow is double clicked.
      //
      // VNT TODO: do not handle this event and remove CS_DBLCLKS in register class.
      // Only handle timed clicks in BUTTONDOWN with no proximity.
      //
      dwDownTime = GetTickCount();

      #ifndef DISABLEAUDIO
      if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
      #endif
      ShowMenu();
      break;

    case WM_MOUSEMOVE:
      if (AATEnabled && TargetPan && (TargetDrag_State>0)) {  // target follows "finger" so easier to drop near edge of sector
        if (TargetDrag_State == 1) {
          POINT Pos;
          double mouseMovelon, mouseMovelat;
          Pos.x = LOWORD(lParam);
          Pos.y = HIWORD(lParam);

          Screen2LatLon((int)Pos.x, (int)Pos.y, mouseMovelon, mouseMovelat);
          if (InAATTurnSector(mouseMovelon, mouseMovelat, TargetPanIndex)) {
            // update waypoints so if we drag out of the cylindar, it will remain adjacent to the edge
            Task[TargetPanIndex].AATTargetLat = mouseMovelat;
            Task[TargetPanIndex].AATTargetLon = mouseMovelon;
            TargetDrag_Latitude = mouseMovelat;
            TargetDrag_Longitude = mouseMovelon;
            DrawBitmapIn(hdcScreen, Pos, hBmpTarget);
          }
        }
      }
      break;

    case WM_LBUTTONDOWN:
      #ifdef DEBUG_DBLCLK
      DoStatusMessage(_T("BUTTONDOWN MapWindow"));
      #endif
      ResetDisplayTimeOut();
      dwDownTime = GetTickCount();
      if (ignorenext) break;
      XstartScreen = LOWORD(lParam); YstartScreen = HIWORD(lParam);
      // TODO VNT move Screen2LatLon in LBUTTONUP after making sure we really need Xstart and Ystart
      // so we save precious milliseconds waiting for BUTTONUP GetTickCount
      Screen2LatLon(XstartScreen, YstartScreen, Xstart, Ystart);

      LockTaskData();
      if (AATEnabled && TargetPan) {
	if (ValidTaskPoint(TargetPanIndex)) {
	  POINT tscreen;
	  LatLon2Screen(Task[TargetPanIndex].AATTargetLon,
			Task[TargetPanIndex].AATTargetLat,
			tscreen);
	  distance = isqrt4((long)((XstartScreen-tscreen.x)
				   *(XstartScreen-tscreen.x)+
				   (YstartScreen-tscreen.y)
				   *(YstartScreen-tscreen.y)))
	    /InfoBoxLayout::scale;

	  if (distance<10) {
	    TargetDrag_State = 1;
	  }
	}
      }
      UnlockTaskData();

      FullScreen();
      break;

    case WM_LBUTTONUP:

      if (ignorenext||dwDownTime==0) {
		ignorenext=false;
	  break;
	}
      RECT rc;
      dwUpTime = GetTickCount();
      dwInterval=dwUpTime-dwDownTime;
      dwDownTime=0; // do it once forever
/*
	TCHAR buf[80];
	wsprintf(buf,_T("Interval %ldms"),dwInterval);
        DoStatusMessage(buf);
*/
      GetClientRect(hWnd,&rc);

      X = LOWORD(lParam); Y = HIWORD(lParam);

      if (dwInterval == 0) {
		DoStatusMessage(_T("dwInterval==0 impossible!"));
		break; // should be impossible
      }


      distance = isqrt4((long)((XstartScreen-X)*(XstartScreen-X)+
			       (YstartScreen-Y)*(YstartScreen-Y)))
	/InfoBoxLayout::scale;

	#ifdef DEBUG_VIRTUALKEYS
	TCHAR buf[80]; char sbuf[80];
	sprintf(sbuf,"%.0f",distance);
	_stprintf(buf,_T("XY=%d,%d dist=%S Up=%ld Down=%ld Int=%ld"),X,Y,sbuf,dwUpTime,dwDownTime,dwInterval);
        DoStatusMessage(buf);
	#endif

	// Caution, timed clicks from PC with a mouse are different from real touchscreen devices

      if ((VirtualKeys==(VirtualKeys_t)vkEnabled) && distance<50 && (dwInterval>= DOUBLECLICKINTERVAL)) {
		wParam=ProcessVirtualKey(X,Y,dwInterval,0);
		if (wParam==0) {
			#ifdef DEBUG_VIRTUALKEYS
			DoStatusMessage(_T("E02 INVALID Virtual Key!"));
			#endif
			break;
		}
		goto Wirth;
      }

      Screen2LatLon(X, Y, Xlat, Ylat);

      if (AATEnabled && TargetPan && (TargetDrag_State>0)) {
	LockTaskData();
	TargetDrag_State = 2;
        if (InAATTurnSector(Xlat, Ylat, TargetPanIndex)) {
            // if release mouse out of sector, don't update w/ bad coords
	  TargetDrag_Latitude = Ylat;
	  TargetDrag_Longitude = Xlat;
        }
	UnlockTaskData();
	break;
      } else if (!TargetPan && EnablePan && (distance>36)) { // TODO FIX should be IBLSCALE 36 instead?
	PanLongitude += Xstart-Xlat;
	PanLatitude  += Ystart-Ylat;
	RefreshMap();
	break;
      }
#ifdef _SIM_
      else if (!TargetPan && (distance>IBLSCALE(36))) {
	// This drag moves the aircraft (changes speed and direction)
	double newbearing;
	double oldbearing = GPS_INFO.TrackBearing;
	double minspeed = 1.1*GlidePolar::Vminsink;
	DistanceBearing(Ystart, Xstart, Ylat, Xlat, NULL, &newbearing);
	if ((fabs(AngleLimit180(newbearing-oldbearing))<30)
	    || (GPS_INFO.Speed<minspeed)) {
	  GPS_INFO.Speed = min(100.0,max(minspeed,distance/3));
	  // 20080817 JMW change speed only if in direction
	}
	GPS_INFO.TrackBearing = newbearing;
	// change bearing without changing speed if direction change > 30
	// 20080815 JMW prevent dragging to stop glider

	// JMW trigger recalcs immediately
	TriggerGPSUpdate();

	break;
      }
#endif
      if (!TargetPan) {
		if ( InfoFocus>=0) { //
			DefocusInfoBox();
			SetFocus(hWnd);
			#ifndef DISABLEAUDIO
			 if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
			#endif
			break;
		}

		if (VirtualKeys==(VirtualKeys_t)vkEnabled) {
			if(dwInterval < VKSHORTCLICK) { //100ms is NOT  enough for a short click since GetTickCount is OEM custom!
	  if (Event_NearestWaypointDetails(Xstart, Ystart, 500*MapScale, false)) {
	    break;
	  }
	} else {
	  if (Event_InteriorAirspaceDetails(Xstart, Ystart)) {
	    break;
	  }
	}
		} else {
			if(dwInterval < AIRSPACECLICK) { // original and untouched interval
			  if (Event_NearestWaypointDetails(Xstart, Ystart, 500*MapScale, false)) {
			    break;
			  }
			} else {
			  if (Event_InteriorAirspaceDetails(Xstart, Ystart)) {
			    break;
			  }
      }
		} // VK enabled
      } // !TargetPan

      break;
      /*
	case WM_PAINT:
	if ((hWnd == hWndMapWindow) && (ProgramStarted==3)) {
	//    RequestFastRefresh();
	return TRUE;
	} else {
	break;
	}
      */




#if defined(GNAV) || defined(PNA) // VENTA FIXED PNA SCROLL WHEEL
    case WM_KEYDOWN: // JMW was keyup
#else
    case WM_KEYUP: // JMW was keyup
#endif
      // VENTA-TODO careful here, keyup no more trapped for PNA.
      // Forbidden usage of keypress timing.

#ifdef VENTA_DEBUG_KEY
      TCHAR ventabuffer[80];
      _stprintf(ventabuffer,TEXT("WMKEY uMsg=%d wParam=%ld lParam=%ld"), uMsg, wParam,lParam);
      DoStatusMessage(ventabuffer);
#endif
      ResetDisplayTimeOut();
      InterfaceTimeoutReset();

#if defined(PNA) // VENTA-ADDON HARDWARE KEYS TRANSCODING

      if ( GlobalModelType == MODELTYPE_PNA_HP31X )
	{
	  //		if (wParam == 0x7b) wParam=0xc1;  // VK_APP1
	  if (wParam == 0x7b) wParam=0x1b;  // VK_ESCAPE
	  //		if (wParam == 0x7b) wParam=0x27;  // VK_RIGHT
	  //		if (wParam == 0x7b) wParam=0x25;  // VK_LEFT
	} else
	if ( GlobalModelType == MODELTYPE_PNA_PN6000 )
	  {
	    switch(wParam) {
	    case 0x79:					// Upper Silver key short press
	      wParam = 0xc1;	// F10 -> APP1
	      break;
	    case 0x7b:					// Lower Silver key short press
	      wParam = 0xc2;	// F12 -> APP2
	      break;
	    case 0x72:					// Back key plus
	      wParam = 0xc3;	// F3  -> APP3
	      break;
	    case 0x71:					// Back key minus
	      wParam = 0xc4;	// F2  -> APP4
	      break;
	    case 0x7a:					// Upper silver key LONG press
	      wParam = 0x70;	// F11 -> F1
	      break;
	    case 0x7c:					// Lower silver key LONG press
	      wParam = 0x71;	// F13 -> F2
	      break;
	    }
	  }
      if ( GlobalModelType == MODELTYPE_PNA_NOKIA_500 )
	{
	  switch(wParam) {
	  case 0xc1:
	    wParam = 0x0d;	// middle key = enter
	    break;
	  case 0xc5:
	    wParam = 0x26;	// + key = pg Up
	    break;
	  case 0xc6:
	    wParam = 0x28;	// - key = pg Down
	    break;
	  }
	}
      if ( GlobalModelType == MODELTYPE_PNA_MEDION_P5 )
	{
	  switch(wParam) {
	  case 0x79:
	    wParam = 0x0d;	// middle key = enter
	    break;
	  case 0x75:
	    wParam = 0x26;	// + key = pg Up
	    break;
	  case 0x76:
	    wParam = 0x28;	// - key = pg Down
	    break;
	  }
	}

#endif


#if defined(GNAV)
      if (wParam == 0xF5){

	if (MessageBoxX(gettext(TEXT("Shutdown?")),
			gettext(TEXT("Altair system message")),
			MB_YESNO|MB_ICONQUESTION) == IDYES) {

	  SendMessage(hWnd,
		      WM_ACTIVATE,
		      MAKEWPARAM(WA_INACTIVE, 0),
		      (LPARAM)hWndMainWindow);
	  SendMessage (hWndMainWindow, WM_CLOSE, 0, 0);
	}

	break;

      }
#endif
Wirth:
      dwDownTime= 0L;

      if (!DialogActive) { // JMW prevent keys being trapped if dialog is active
	if (InputEvents::processKey(wParam)) {
	  // TODO code: change to debugging DoStatusMessage(TEXT("Event in default"));
	}
	// XXX Should we only do this if it IS processed above ?
	dwDownTime= 0L;
	return TRUE; // don't go to default handler
      } else {
	// TODO code: debugging DoStatusMessage(TEXT("Event in dialog"));
	if (InputEvents::processKey(wParam)) {
	}
	dwDownTime= 0L;
	return TRUE; // don't go to default handler
      }
      // break; unreachable!
    }

  return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}



bool MapWindow::isTargetPan(void) {
  return TargetPan;
}



bool MapWindow::RenderTimeAvailable() {
  DWORD fpsTime = ::GetTickCount();
  if (MapDirty) return false;

  if (fpsTime-timestamp_newdata<700) {
    // it's been less than 700 ms since last data
    // was posted
    return true;
  } else {
    return false;
  }
}


void MapWindow::RenderMapWindowBg(HDC hdc, const RECT rc,
				  const POINT &Orig,
				  const POINT &Orig_Aircraft)
{
  HFONT hfOld;

  // do slow calculations before clearing the screen
  // to reduce flicker
  CalculateWaypointReachable();
  CalculateScreenPositionsAirspace();
  CalculateScreenPositionsThermalSources();
  CalculateScreenPositionsGroundline();

  if (!EnableTerrain || !DerivedDrawInfo.TerrainValid
      || !RasterTerrain::isTerrainLoaded() ) {

    // JMW this isn't needed any more unless we're not drawing terrain

    // display border and fill background..
    if(InfoWindowActive) {
      SelectObject(hdc, GetStockObject(WHITE_BRUSH));
      SelectObject(hdc, GetStockObject(BLACK_PEN));
    }
    else {
      // JMW added light grey background
      SelectObject(hdc, hBackgroundBrush);
      SelectObject(hdc, GetStockObject(WHITE_PEN));
    }
    Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
  }

  SelectObject(hdc, GetStockObject(BLACK_BRUSH));
  SelectObject(hdc, GetStockObject(BLACK_PEN));
  hfOld = (HFONT)SelectObject(hdc, MapWindowFont);

  // ground first...

  if (BigZoom) {
    BigZoom = false;
  }

  if ((EnableTerrain && (DerivedDrawInfo.TerrainValid)
       && RasterTerrain::isTerrainLoaded())
      || RasterTerrain::render_weather) {
    double sunelevation = 40.0;
    double sunazimuth = DisplayAngle-DerivedDrawInfo.WindBearing;

    // draw sun from constant angle if very low wind speed
    if (DerivedDrawInfo.WindSpeed<0.5) {
      sunazimuth = DisplayAngle + 45.0;
    }

    if (MapDirty) {
      // map has been dirtied since we started drawing, so hurry up
      BigZoom = true;
    }
    LockTerrainDataGraphics();
    DrawTerrain(hdc, rc, sunazimuth, sunelevation, DrawInfo.Longitude, DrawInfo.Latitude);
    if ((FinalGlideTerrain==2) && DerivedDrawInfo.TerrainValid) {
      SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);
      DrawTerrainAbove(hdc, rc, hDCTemp);
    }
    UnlockTerrainDataGraphics();
  }

  if (EnableTopology) {
    DrawTopology(hdc, rc);
  }

  // reset label over-write preventer
  nLabelBlocks = 0;

  if (!TaskIsTemporary()) {
    SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);
    DrawTaskAAT(hdc, rc, hDCTemp);
  }

  // then airspace..
  if (OnAirSpace > 0) {
    // VENTA3 default is true, always true at startup no regsave
    SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);
    DrawAirSpace(hdc, rc, hDCTemp);
  }

  if(TrailActive) {
    // TODO enhancement: For some reason, the shadow drawing of the
    // trail doesn't work in portrait mode.  No idea why.
    if (1) {
      double TrailFirstTime =
	DrawTrail(hdc, Orig_Aircraft, rc);
      DrawTrailFromTask(hdc, rc, TrailFirstTime);
    } else {
      /*
      //  Draw trail with white outline --- very slow!
      // clear background bitmap
      SelectObject(hDCTemp, GetStockObject(WHITE_BRUSH));
      Rectangle(hDCTemp, rc.left, rc.top, rc.right, rc.bottom);

      SetBkColor(hDCMask, RGB(0xff,0xff,0xff));

      // draw trail on background bitmap
      DrawTrail(hDCTemp, Orig_Aircraft, rc);
      DrawTrailFromTask(hDCTemp, rc);

      // make mask
      BitBlt(hDCMask, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
      hDCTemp, rc.left, rc.top, SRCCOPY);

      BitBlt(hdcDrawWindowBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCMask, 1, 1, SRCAND);
      BitBlt(hdcDrawWindowBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCTemp, rc.left, rc.top, SRCPAINT);
      BitBlt(hdcDrawWindowBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCTemp, rc.left, rc.top, SRCAND);
      */
    }
  }

  DrawThermalEstimate(hdc, rc);

  if (TaskAborted) {
    DrawAbortedTask(hdc, rc, Orig_Aircraft);
  } else {
    DrawTask(hdc, rc, Orig_Aircraft);
  }

  // draw red cross on glide through terrain marker
  if (FinalGlideTerrain && DerivedDrawInfo.TerrainValid) {
    DrawGlideThroughTerrain(hdc, rc);
  }

  DrawWaypoints(hdc,rc);

  DrawTeammate(hdc, rc);

  if ((EnableTerrain && (DerivedDrawInfo.TerrainValid))
      || RasterTerrain::render_weather) {
    DrawSpotHeights(hdc);
  }

  if (extGPSCONNECT) {
    // TODO enhancement: don't draw offtrack indicator if showing spot heights
    DrawProjectedTrack(hdc, rc, Orig_Aircraft);
    DrawOffTrackIndicator(hdc, rc);
    DrawBestCruiseTrack(hdc, Orig_Aircraft);
  }
  DrawBearing(hdc, rc, extGPSCONNECT);


  // draw wind vector at aircraft
  if (!EnablePan) {
    DrawWindAtAircraft2(hdc, Orig_Aircraft, rc);
  } else if (TargetPan) {
    DrawWindAtAircraft2(hdc, Orig, rc);
  }

  // Draw traffic
  DrawFLARMTraffic(hdc, rc, Orig_Aircraft);

  // finally, draw you!

  if (EnablePan && !TargetPan) {
    DrawCrossHairs(hdc, Orig, rc);
  }

  if (extGPSCONNECT) {
    DrawAircraft(hdc, Orig_Aircraft);
  }

  if ( (!TargetPan) && (!EnablePan) && (VisualGlide>0) ) {
    DrawGlideCircle(hdc, Orig, rc);
  }

  // marks on top...
  DrawMarks(hdc, rc);
  SelectObject(hdcDrawWindow, hfOld);

}


void MapWindow::RenderMapWindow(HDC hdc, const RECT rc)
{
  bool drawmap = false;
  HFONT hfOld;

  DWORD fpsTime = ::GetTickCount();

  // only redraw map part every 800 s unless triggered
  if (((fpsTime-fpsTime0)>800)||(fpsTime0== 0)||(userasked)) {
    fpsTime0 = fpsTime;
    drawmap = true;
    userasked = false;
  }
  MapWindow::UpdateTimeStats(true);

  POINT Orig, Orig_Aircraft;

  CalculateOrigin(rc, &Orig);

  CalculateScreenPositions(Orig, rc, &Orig_Aircraft);

  RenderMapWindowBg(hdc, rc, Orig, Orig_Aircraft);

  // overlays
  DrawCDI();

  hfOld = (HFONT)SelectObject(hdc, MapWindowFont);

  DrawMapScale(hdc,rc, BigZoom);

  DrawMapScale2(hdc,rc, Orig_Aircraft);

  DrawCompass(hdc, rc);

  // JMW Experimental only! EXPERIMENTAL
#if 0
  //  #ifdef GNAV
  if (EnableAuxiliaryInfo) {
    DrawHorizon(hdc, rc);
  }
  //  #endif
#endif

  DrawFlightMode(hdc, rc);

  DrawThermalBand(hdc, rc);

  DrawFinalGlide(hdcDrawWindow,rc);

  //  DrawSpeedToFly(hdc, rc);

  DrawGPSStatus(hdc, rc);

  SelectObject(hdc, hfOld);

}


void MapWindow::UpdateInfo(NMEA_INFO *nmea_info,
                           DERIVED_INFO *derived_info) {
  LockFlightData();
  memcpy(&DrawInfo,nmea_info,sizeof(NMEA_INFO));
  memcpy(&DerivedDrawInfo,derived_info,sizeof(DERIVED_INFO));
  UpdateMapScale(); // done here to avoid double latency due to locks
  UnlockFlightData();
}


void MapWindow::UpdateCaches(bool force) {
  // map was dirtied while we were drawing, so skip slow process
  // (unless we haven't done it for 2000 ms)
  DWORD fpsTimeThis = ::GetTickCount();
  static double lastTime = 0;
  static DWORD fpsTimeMapCenter = 0;

  if (MapWindow::ForceVisibilityScan) {
    force = true;
    MapWindow::ForceVisibilityScan = false;
  }

  // have some time, do shape file cache update if necessary
  LockTerrainDataGraphics();
  SetTopologyBounds(MapRect, force);
  UnlockTerrainDataGraphics();

  // JMW experimental jpeg2000 rendering/tile management
  // Must do this even if terrain is not displayed, because
  // raster terrain is used by terrain footprint etc.
  if (lastTime>DrawInfo.Time) {
    lastTime = DrawInfo.Time;
  }

  if (force || (fpsTimeThis - fpsTimeMapCenter > 5000)) {

    fpsTimeThis = fpsTimeMapCenter;
    RasterTerrain::ServiceTerrainCenter(DrawInfo.Latitude,
                                        DrawInfo.Longitude);
  }

  fpsTimeThis = ::GetTickCount();
  static DWORD fpsTimeLast_terrain=0;

  if (RenderTimeAvailable() ||
      (fpsTimeThis-fpsTimeLast_terrain>5000) || force) {
    // have some time, do graphics terrain cache update if necessary
    if (EnableTerrain) {
      fpsTimeLast_terrain = fpsTimeThis;
      RasterTerrain::ServiceCache();
    }
  }
}

extern MapWindow hWndMapWindow; // TODO try to avoid this


DWORD MapWindow::DrawThread (LPVOID lpvoid)
{
  HWND hWndMapWindow = ::hWndMapWindow;

  while ((!ProgramStarted) || (!Initialised)) {
    Sleep(100);
  }

  //  THREADRUNNING = FALSE;
  THREADEXIT = FALSE;

  nLabelBlocks = 0;

  GetClientRect(hWndMapWindow, &MapRectBig);

  UpdateTimeStats(true);

  MapRectSmall = MapRect;
  MapRect = MapRectSmall;

  SetBkMode(hdcDrawWindow,TRANSPARENT);
  SetBkMode(hDCTemp,OPAQUE);
  SetBkMode(hDCMask,OPAQUE);

  // paint draw window black to start
  SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
  Rectangle(hdcDrawWindow,MapRectBig.left,MapRectBig.top,
            MapRectBig.right,MapRectBig.bottom);

  BitBlt(hdcScreen, 0, 0, MapRectBig.right-MapRectBig.left,
         MapRectBig.bottom-MapRectBig.top,
         hdcDrawWindow, 0, 0, SRCCOPY);

  ////// This is just here to give fully rendered start screen
  UpdateInfo(&GPS_INFO, &CALCULATED_INFO);
  MapDirty = true;
  UpdateTimeStats(true);
  //////

  RequestMapScale = MapScale;
  ModifyMapScale();

  bool first = true;

  for (int i=0; i<AIRSPACECLASSCOUNT; i++) {
    hAirspacePens[i] =
      CreatePen(PS_SOLID, IBLSCALE(2), Colours[iAirspaceColour[i]]);
  }

  while (!CLOSETHREAD)
    {
      drawTriggerEvent.wait(5000);
      if (CLOSETHREAD) break; // drop out without drawing

      if ((!THREADRUNNING) || (!GlobalRunning)) {
	Sleep(100);
	continue;
      }

      if (!MapDirty && !first) {
	// redraw old screen, must have been a request for fast refresh
	BitBlt(hdcScreen, 0, 0, MapRectBig.right-MapRectBig.left,
	       MapRectBig.bottom-MapRectBig.top,
	       hdcDrawWindow, 0, 0, SRCCOPY);
	continue;
      } else {
	MapDirty = false;
      }

      if (BigZoom) {
	// quickly draw zoom level on top
	DrawMapScale(hdcScreen, MapRect, true);
      }

      MapWindow::UpdateInfo(&GPS_INFO, &CALCULATED_INFO);

      if (RequestFullScreen != MapFullScreen) {
	ToggleFullScreenStart();
      }

      GaugeFLARM::Render(&DrawInfo);

      RenderMapWindow(hdcDrawWindow, MapRect);

      if (!first) {
	BitBlt(hdcScreen, 0, 0,
	       MapRectBig.right-MapRectBig.left,
	       MapRectBig.bottom-MapRectBig.top,
	       hdcDrawWindow, 0, 0, SRCCOPY);
	InvalidateRect(hWndMapWindow, &MapRect, false);
      }
      UpdateTimeStats(false);

      // we do caching after screen update, to minimise perceived delay
      UpdateCaches(first);
      first = false;
      if (ProgramStarted==psInitDone) {
	ProgramStarted = psFirstDrawDone;
	GaugeVario::Show(!MapFullScreen);
      }

    }
  THREADEXIT = TRUE;
  return 0;
}

/////////////////////////////////////////////////////////////////////

bool MapWindow::IsDisplayRunning() {
  return (THREADRUNNING && GlobalRunning && !ScreenBlanked && ProgramStarted);
}


void MapWindow::CreateDrawingThread(void)
{
  CLOSETHREAD = FALSE;
  THREADEXIT = FALSE;
  hDrawThread = CreateThread (NULL, 0,
                              (LPTHREAD_START_ROUTINE )MapWindow::DrawThread,
                              0, 0, &dwDrawThreadID);
  SetThreadPriority(hDrawThread,THREAD_PRIORITY_NORMAL);
}

void MapWindow::SuspendDrawingThread(void)
{
  LockTerrainDataGraphics();
  THREADRUNNING = FALSE;
  UnlockTerrainDataGraphics();
  //  SuspendThread(hDrawThread);
}

void MapWindow::ResumeDrawingThread(void)
{
  LockTerrainDataGraphics();
  THREADRUNNING = TRUE;
  UnlockTerrainDataGraphics();
  //  ResumeThread(hDrawThread);
}

void MapWindow::CloseDrawingThread(void)
{
  CLOSETHREAD = TRUE;
  drawTriggerEvent.trigger(); // wake self up
  LockTerrainDataGraphics();
  SuspendDrawingThread();
  UnlockTerrainDataGraphics();
  while(!THREADEXIT) { Sleep(100); };
}


void MapWindow::DisplayAirspaceWarning(int Type, const TCHAR *Name,
                                       AIRSPACE_ALT Base, AIRSPACE_ALT Top )
{
  TCHAR szMessageBuffer[1024];
  TCHAR szTitleBuffer[1024];

  FormatWarningString(Type, Name , Base, Top, szMessageBuffer, szTitleBuffer );

  DoStatusMessage(TEXT("Airspace Query"), szMessageBuffer);
}




//////////////////////////////////////////////////////////////////////////////
