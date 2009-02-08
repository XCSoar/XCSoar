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

#include "StdAfx.h"
#include "compatibility.h"
#include "MapWindow.h"
#include "OnLineContest.h"
#include "Utils.h"
#include "Units.h"
#include "Logger.h"
#include "McReady.h"
#include "Airspace.h"
#include "Waypointparser.h"
#include "Dialogs.h"
#include "externs.h"
#include "VarioSound.h"
#include "InputEvents.h"
// #include <assert.h>
#include <windows.h>
#include <math.h>

#include <tchar.h>

#include "Terrain.h"
#include "options.h"
#include "Task.h"
#include "AATDistance.h"

#include "GaugeVarioAltA.h"
#include "GaugeCDI.h"
#include "GaugeFLARM.h"
#include "InfoBoxLayout.h"
#include "RasterTerrain.h"

#if (WINDOWSPC>0)
#include <wingdi.h>
#endif

int misc_tick_count=0;

#ifdef DEBUG
#define DRAWLOAD
#endif

int TrailActive = TRUE;

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

double MapWindow::RequestMapScale = 5;
double MapWindow::MapScale = 5;
double MapWindow::MapScaleOverDistanceModify = 5/DISTANCEMODIFY;
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

  
COLORREF MapWindow::BackgroundColor = RGB(0xFF,0xFF,0xFF); //sjt 02NOV05 - was F5F5F5. Changed to increase screen clarity at oblique viewing angles.

bool MapWindow::MapDirty = true;
DWORD MapWindow::fpsTime0 = 0;
bool MapWindow::MapFullScreen = false;
bool MapWindow::RequestFullScreen = false;
bool MapWindow::ForceVisibilityScan = false;

/////////////////////////////////

extern int DisplayTimeOut;

NMEA_INFO MapWindow::DrawInfo;
DERIVED_INFO MapWindow::DerivedDrawInfo;

int SelectedWaypoint = -1;
bool EnableCDICruise = false;
bool EnableCDICircling = false;

extern HWND hWndCDIWindow;
extern int iround(double i);
extern void ShowMenu();

extern HFONT  TitleWindowFont;
extern HFONT  MapWindowFont;
extern HFONT  MapWindowBoldFont;
extern HFONT  InfoWindowFont;
extern HFONT  CDIWindowFont;


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
      char tmptext[100];
      sprintf(tmptext,"%d # mem\n%d # latency\n", CheckFreeRam()/1024, timestats_av);
      DebugStore(tmptext);
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

  static double CruiseMapScale = 10;
  static double ClimbMapScale = 0.25;
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

bool TextInBoxMoveInView(POINT *offset, RECT *brect){

  bool res = false;

  int LabelMargin = 4;

  offset->x = 0;
  offset->y = 0;

  if (MapWindow::MapRect.top > brect->top){
    int d = MapWindow::MapRect.top - brect->top;
    brect->top += d;
    brect->bottom += d;
    offset->y += d;
    brect->bottom -= d;
    brect->left -= d;
    offset->x -= d;
    res = true;
  }

  if (MapWindow::MapRect.right < brect->right){
    int d = MapWindow::MapRect.right - brect->right;

    if (offset->y < LabelMargin){
      int dy;

      if (d > -LabelMargin){
        dy = LabelMargin-offset->y;
        if (d > -dy)
          dy = -d;
      } else {
        int x = d + (brect->right - brect->left) + 10;

        dy = x - offset->y;

        if (dy < 0)
          dy = 0;

        if (dy > LabelMargin)
          dy = LabelMargin;
      }

      brect->top += dy;
      brect->bottom += dy;
      offset->y += dy;

    }

    brect->right += d;
    brect->left += d;
    offset->x += d;

    res = true;
  }

  if (MapWindow::MapRect.bottom < brect->bottom){
    if (offset->x == 0){
      int d = MapWindow::MapRect.bottom - brect->bottom;
      brect->top += d;
      brect->bottom += d;
      offset->y += d;
    } else
    if (offset->x < -LabelMargin){
      int d = -(brect->bottom - brect->top) - 10;
      brect->top += d;
      brect->bottom += d;
      offset->y += d;
    } else {
      int d = -(2*offset->x + (brect->bottom - brect->top));
      brect->top += d;
      brect->bottom += d;
      offset->y += d;
    }

    res = true;
  }

  if (MapWindow::MapRect.left > brect->left){
    int d = MapWindow::MapRect.left - brect->left;
    brect->right+= d;
    brect->left += d;
    offset->x += d;
    res = true;
  }

  return(res);

}

void MapWindow::TextInBox(HDC hDC, TCHAR* Value, int x, int y, 
                          int size, TextInBoxMode_t Mode, bool noOverlap) {

  #define WPCIRCLESIZE        2

  SIZE tsize;
  RECT brect;
  HFONT oldFont;
  POINT org;

  if ((x<MapRect.left-WPCIRCLESIZE) || 
      (x>MapRect.right+(WPCIRCLESIZE*3)) || 
      (y<MapRect.top-WPCIRCLESIZE) ||
      (y>MapRect.bottom+WPCIRCLESIZE)) {
    return;
  }

  org.x = x;
  org.y = y;

  if (size==0) {
    size = _tcslen(Value);
  }
  
  HBRUSH hbOld;
  hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));

  if (Mode.AsFlag.Reachable){
    if (Appearance.IndLandable == wpLandableDefault){
      x += 5;  // make space for the green circle
    }else
    if (Appearance.IndLandable == wpLandableAltA){
      x += 0;
    }
  }

  if (Mode.AsFlag.Border){
    oldFont = (HFONT)SelectObject(hDC, MapWindowBoldFont);
  } else {
    oldFont = (HFONT)SelectObject(hDC, MapWindowFont);
  }
  
  GetTextExtentPoint(hDC, Value, size, &tsize);

  if (Mode.AsFlag.AlligneRight){
    x -= tsize.cx;
  } else 
    if (Mode.AsFlag.AlligneCenter){
      x -= tsize.cx/2;
      y -= tsize.cy/2;
    }

  bool notoverlapping = true;

  if (Mode.AsFlag.Border || Mode.AsFlag.WhiteBorder){

    POINT offset;

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    if (Mode.AsFlag.AlligneRight)
      x -= 3;

    if (TextInBoxMoveInView(&offset, &brect)){
      x += offset.x;
      y += offset.y;
    }

    notoverlapping = checkLabelBlock(brect);
  
    if (!noOverlap || notoverlapping) {
      HPEN oldPen;
      if (Mode.AsFlag.Border) {
        oldPen = (HPEN)SelectObject(hDC, hpMapScale);
      } else {
        oldPen = (HPEN)SelectObject(hDC, GetStockObject(WHITE_PEN));
      }
      RoundRect(hDC, brect.left, brect.top, brect.right, brect.bottom, 
                IBLSCALE(8), IBLSCALE(8));
      SelectObject(hDC, oldPen);
#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);
#else
      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
    }


  } else if (Mode.AsFlag.FillBackground) {

    POINT offset;

    brect.left = x-1;
    brect.right = brect.left+tsize.cx+1;  
    brect.top = y+((tsize.cy+4)>>3);
    brect.bottom = brect.top+tsize.cy-((tsize.cy+4)>>3);

    if (Mode.AsFlag.AlligneRight)
      x -= 2;

    if (TextInBoxMoveInView(&offset, &brect)){
      x += offset.x;
      y += offset.y;
    }

    notoverlapping = checkLabelBlock(brect);
  
    if (!noOverlap || notoverlapping) {
      COLORREF oldColor = SetBkColor(hDC, RGB(0xff, 0xff, 0xff));
      ExtTextOut(hDC, x, y, ETO_OPAQUE, &brect, Value, size, NULL);
      SetBkColor(hDC, oldColor);
    }

  } else if (Mode.AsFlag.WhiteBold) {

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    notoverlapping = checkLabelBlock(brect);
  
    if (!noOverlap || notoverlapping) {
      SetTextColor(hDC,RGB(0xff,0xff,0xff));
#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x+2, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x+1, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-1, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-2, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y+1, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y-1, 0, NULL, Value, size, NULL);
      SetTextColor(hDC,RGB(0x00,0x00,0x00));
      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);
#else
      ExtTextOut(hDC, x+2, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x+1, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-1, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-2, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y+1, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y-1, ETO_OPAQUE, NULL, Value, size, NULL);
      SetTextColor(hDC,RGB(0x00,0x00,0x00));
      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
    }

  } else {

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    notoverlapping = checkLabelBlock(brect);
  
    if (!noOverlap || notoverlapping) {
#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);
#else
      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
    }

  }
  
  SelectObject(hDC, oldFont);
  SelectObject(hDC, hbOld);

}

bool userasked = false;

void MapWindow::RequestFastRefresh() {
  SetEvent(drawTriggerEvent);
}

void MapWindow::RefreshMap() {
  MapDirty = true;
  userasked = true;
  timestats_dirty = true;
  SetEvent(drawTriggerEvent);
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


void MapWindow::Event_Pan(int vswitch) {
//  static bool oldfullscreen = 0;  never assigned!
  bool oldPan = EnablePan;
  if (vswitch == -2) { // superpan, toggles fullscreen also

    if (!EnablePan) {
      StoreRestoreFullscreen(true);
    } else {
      StoreRestoreFullscreen(false);
    }
    // new mode
    EnablePan = !EnablePan;
    if (EnablePan) { // pan now on, so go fullscreen
      RequestFullScreen = true;
    }

  } else if (vswitch == -1) {
    EnablePan = !EnablePan;
  } else {
    EnablePan = (vswitch != 0); // 0 off, 1 on
  }

  if (EnablePan != oldPan) {
    if (EnablePan) {
      PanLongitude = DrawInfo.Longitude;
      PanLatitude = DrawInfo.Latitude;
      InputEvents::setMode(TEXT("pan"));
    } else 
      InputEvents::setMode(TEXT("default"));
  }
  RefreshMap();
}


double MapWindow::LimitMapScale(double value) {

  double minreasonable = 0.05;
  if (AutoZoom && DisplayMode != dmCircling) {
    if (AATEnabled && (ActiveWayPoint>0)) {
      minreasonable = 0.88;
    } else {
      minreasonable = 0.44; // was 0.22
    }
  }

  if (ScaleListCount>0) {
    return FindMapScale(max(minreasonable,min(160.0,value)));
  } else {
    return max(minreasonable,min(160.0,value));
  }
}


void MapWindow::Event_SetZoom(double value) {

  static double lastRequestMapScale = RequestMapScale;

  RequestMapScale = LimitMapScale(value);
  if (lastRequestMapScale != RequestMapScale){
    lastRequestMapScale = RequestMapScale;
    BigZoom = true;
    RefreshMap();
  }
}


void MapWindow::Event_ScaleZoom(int vswitch) {

  static double lastRequestMapScale = RequestMapScale;
  double value = RequestMapScale;
  static int nslow=0;

  // For best results, zooms should be multiples or roots of 2

  if (ScaleListCount > 0){
    value = FindMapScale(RequestMapScale);
    value = StepMapScale(-vswitch);
  } else {

    if (abs(vswitch)>=4) {
      nslow++;
      if (nslow %2 != 0) {
        // JMW disabled        return;
      }
      if (vswitch==4) {
        vswitch = 1;
      }
      if (vswitch==-4) {
        vswitch = -1;
      }
    }
    if (vswitch==1) { // zoom in a little
      value /= 1.414;
    }
    if (vswitch== -1) { // zoom out a little
      value *= 1.414;
    }
    if (vswitch==2) { // zoom in a lot
      value /= 2.0;
    }
    if (vswitch== -2) { // zoom out a lot
      value *= 2.0;
    } 

  }
  RequestMapScale = LimitMapScale(value);

  if (lastRequestMapScale != RequestMapScale){
    lastRequestMapScale = RequestMapScale;
    BigZoom = true;
    RefreshMap();

    //    DrawMapScale(hdcScreen, MapRect, true);
    // JMW this is bad, happening from wrong thread.
  }
}


int MapWindow::GetMapResolutionFactor(void) {
  return IBLSCALE(30);
}

double MapWindow::StepMapScale(int Step){
  static int nslow=0;
  if (abs(Step)>=4) {
    nslow++;
    //    if (nslow %2 == 0) {
      ScaleCurrent += Step/4;
      //    }
  } else {
    ScaleCurrent += Step;
  }
  ScaleCurrent = max(0,min(ScaleListCount-1, ScaleCurrent));
  return((ScaleList[ScaleCurrent]*GetMapResolutionFactor())
         /(IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right)));
}

double MapWindow::FindMapScale(double Value){

  int    i;
  double BestFit = 99999;
  int    BestFitIdx=-1;
  double DesiredScale = 
    (Value*IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right))/GetMapResolutionFactor();

  for (i=0; i<ScaleListCount; i++){
    double err = fabs(DesiredScale - ScaleList[i])/DesiredScale;
    if (err < BestFit){
      BestFit = err;
      BestFitIdx = i;
    }
  }

  if (BestFitIdx != -1){
    ScaleCurrent = BestFitIdx;
    return((ScaleList[ScaleCurrent]*GetMapResolutionFactor())
           /IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right));
  }
  return(Value);
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
  
  static DWORD dwDownTime= 0, dwUpTime= 0;

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

    if (Appearance.FlightModeIcon == apFlightModeIconAltA){
      //hCruise=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CRUISE_B));
      //hClimb=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMB_B));
      //hFinalGlide=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FINALGLIDE_B));
      //hAbort=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FINALGLIDE_ABORT_B));
      //hBmpClimbeAbort=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMB_ABORT_B));
    } else {
      hCruise=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CRUISE));
      hClimb=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMB));
      hFinalGlide=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FINALGLIDE));
      hAbort=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ABORT));
    }

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

    // JMW TODO red/green Color blind
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
    // Added by ARH to show menu button when mapwindow is
    // double clicked.
    ShowMenu();
    break;

  case WM_LBUTTONDOWN:
    DisplayTimeOut = 0;
    dwDownTime = GetTickCount();
    XstartScreen = LOWORD(lParam); YstartScreen = HIWORD(lParam);
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
    if(InfoWindowActive)
    {
      dwDownTime= 0;
      DefocusInfoBox();
      SetFocus(hWnd);
      break;
    }
    if (dwDownTime== 0) 
      break;

    dwUpTime = GetTickCount(); dwDownTime = dwUpTime - dwDownTime;
    
    X = LOWORD(lParam); Y = HIWORD(lParam);                     
    distance = isqrt4((long)((XstartScreen-X)*(XstartScreen-X)+
                             (YstartScreen-Y)*(YstartScreen-Y)))
      /InfoBoxLayout::scale;

    Screen2LatLon(X, Y, Xlat, Ylat);
    
    if (AATEnabled && TargetPan && (TargetDrag_State>0)) {
      LockTaskData();
      TargetDrag_State = 2;
      TargetDrag_Latitude = Ylat;
      TargetDrag_Longitude = Xlat;
      UnlockTaskData();
      break;
    } else if (!TargetPan && EnablePan && (distance>36)) {
      PanLongitude += Xstart-Xlat;
      PanLatitude  += Ystart-Ylat;
      RefreshMap();
      break; // disable picking when in pan mode
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
      if(dwDownTime < 1000) {
	if (Event_NearestWaypointDetails(Xstart, Ystart, 500*MapScale, false)) {
	  dwDownTime= 0;
	  break;
	}
      } else {
	if (Event_InteriorAirspaceDetails(Xstart, Ystart)) {
	  dwDownTime= 0;
	  break;
	}
      }
    }
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
#ifdef GNAV
    case WM_KEYDOWN: // JMW was keyup
#else
    case WM_KEYUP: // JMW was keyup
#endif
    DisplayTimeOut = 0;
    InterfaceTimeoutReset();

      #if defined(GNAV)
        if (wParam == 0xF5){

          if (MessageBoxX(hWnd,
                          gettext(TEXT("Shutdown?")),
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

        dwDownTime= 0;

    if (!DialogActive) { // JMW prevent keys being trapped if dialog is active
      if (InputEvents::processKey(wParam)) {
        // TODO - change to debugging DoStatusMessage(TEXT("Event in default"));
      }
      // XXX Should we only do this if it IS processed above ?
      dwDownTime= 0;
      return TRUE; // don't go to default handler
    } else {
      // TODO - debugging DoStatusMessage(TEXT("Event in dialog"));
      if (InputEvents::processKey(wParam)) {
      }
      dwDownTime= 0;
      return TRUE; // don't go to default handler
    }
    // break; unreachable!
  }

  return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}


void MapWindow::ModifyMapScale(void) {
  // limit zoomed in so doesn't reach silly levels
  RequestMapScale = LimitMapScale(RequestMapScale);
  MapScaleOverDistanceModify = RequestMapScale/DISTANCEMODIFY;
  ResMapScaleOverDistanceModify = 
    GetMapResolutionFactor()/MapScaleOverDistanceModify;
  DrawScale = MapScaleOverDistanceModify;
  DrawScale = DrawScale/111194;
  DrawScale = GetMapResolutionFactor()/DrawScale;
  InvDrawScale = 1.0/DrawScale;
  MapScale = RequestMapScale;
}


bool MapWindow::isTargetPan(void) {
  return TargetPan;
}


void MapWindow::UpdateMapScale()
{
  static int AutoMapScaleWaypointIndex = -1;
  static double StartingAutoMapScale=0.0;
  double AutoZoomFactor;

  bool useraskedforchange = false;

  // if there is user intervention in the scale
  if(MapScale != RequestMapScale)
  {
    ModifyMapScale();
    useraskedforchange = true;
  }

  double wpd;
  if (TargetPan) {
    wpd = TargetZoomDistance;
  } else {
    wpd = DerivedDrawInfo.ZoomDistance;
  }
  if (TargetPan) {
    // set scale exactly so that waypoint distance is the zoom factor
    // across the screen
    RequestMapScale = LimitMapScale(wpd
                                    *DISTANCEMODIFY/ 4.0);
    ModifyMapScale();
    return;
  } 
  
  if (AutoZoom) {
    if(wpd > 0)
    {
      
      if(
         (((DisplayOrientation == NORTHTRACK)
          &&(DisplayMode != dmCircling))
         ||(DisplayOrientation == NORTHUP) 
         || 
         (((DisplayOrientation == NORTHCIRCLE) 
           || (DisplayOrientation == TRACKCIRCLE)) 
          && (DisplayMode == dmCircling) ))
         && !TargetPan
        )
      {
        AutoZoomFactor = 2.5;
      }
      else
      {
        AutoZoomFactor = 4;
      }
      
      if(
         (wpd < ( AutoZoomFactor * MapScaleOverDistanceModify))
         || 
         (StartingAutoMapScale==0.0))
      {
        // waypoint is too close, so zoom in
        // OR just turned waypoint

        // this is the first time this waypoint has gotten close,
        // so save original map scale

        if (StartingAutoMapScale==0.0) {
          StartingAutoMapScale = MapScale;
        }

        // set scale exactly so that waypoint distance is the zoom factor
        // across the screen
        RequestMapScale = LimitMapScale(wpd
                                        *DISTANCEMODIFY/ AutoZoomFactor);
        ModifyMapScale();

      } else {

        if (useraskedforchange) {

          // user asked for a zoom change and it was achieved, so
          // reset starting map scale


          ////?JMW TODO for frank          StartingAutoMapScale = MapScale;
        }

      }
    }
  } else {
    
    // reset starting map scale for auto zoom if momentarily switch
    // off autozoom
    //    StartingAutoMapScale = RequestMapScale;
  }

  if (TargetPan) {
    return;
  }

  LockTaskData();  // protect from external task changes
  #ifdef HAVEEXCEPTIONS
  __try{
  #endif
  // if we aren't looking at a waypoint, see if we are now
  if (AutoMapScaleWaypointIndex == -1) {
    if (ValidTaskPoint(ActiveWayPoint)) {
      AutoMapScaleWaypointIndex = Task[ActiveWayPoint].Index;
    }
  }

  // if there is an active waypoint
  if (ValidTaskPoint(ActiveWayPoint)) {

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
  #ifdef HAVEEXCEPTIONS
  }__finally
  #endif
  {
    UnlockTaskData();
  }

}


bool MapWindow::GliderCenter=false;


void MapWindow::CalculateOrientationNormal(void) {
  double trackbearing = DrawInfo.TrackBearing;
  //  trackbearing = DerivedDrawInfo.NextTrackBearing;
  if( (DisplayOrientation == NORTHUP) 
      ||
      ((DisplayOrientation == NORTHTRACK)
       &&(DisplayMode != dmCircling))
      || 
      (
       ((DisplayOrientation == NORTHCIRCLE)
        ||(DisplayOrientation==TRACKCIRCLE))
       && (DisplayMode == dmCircling) )
      ) {
    GliderCenter = true;
    
    if (DisplayOrientation == TRACKCIRCLE) {
      DisplayAngle = DerivedDrawInfo.WaypointBearing;
      DisplayAircraftAngle = trackbearing-DisplayAngle;
    } else {
      DisplayAngle = 0.0;
      DisplayAircraftAngle = trackbearing;
    }
  } else {
    // normal, glider forward
    GliderCenter = false;
    DisplayAngle = trackbearing;
    DisplayAircraftAngle = 0.0;    
  }
  DisplayAngle = AngleLimit360(DisplayAngle);
  DisplayAircraftAngle = AngleLimit360(DisplayAircraftAngle);
}


void MapWindow::CalculateOrientationTargetPan(void) {
  // Target pan mode, show track up when looking at current task point,
  // otherwise north up.  If circling, orient towards target.
  GliderCenter = true;
  if ((ActiveWayPoint==TargetPanIndex)
      &&(DisplayOrientation != NORTHUP)
      &&(DisplayOrientation != NORTHTRACK)
      )    {
    if (DisplayMode == dmCircling) {
      // target-up
      DisplayAngle = DerivedDrawInfo.WaypointBearing;
      DisplayAircraftAngle = 
        DrawInfo.TrackBearing-DisplayAngle;
    } else {
      // track up
      DisplayAngle = DrawInfo.TrackBearing;
      DisplayAircraftAngle = 0.0;
    }
  } else {
    // North up
    DisplayAngle = 0.0;
    DisplayAircraftAngle = DrawInfo.TrackBearing;
  }
 
}


void MapWindow::CalculateOrigin(RECT rc, POINT *Orig)
{
  if (TargetPan) {
    CalculateOrientationTargetPan();
  } else {
    CalculateOrientationNormal();
  }
  
  if (GliderCenter || EnablePan) {
    Orig->x = (rc.left + rc.right)/2;
    Orig->y = (rc.bottom + rc.top)/2;
  } else {
    Orig->x = (rc.left + rc.right)/2;
    Orig->y = ((rc.top - rc.bottom )*GliderScreenPosition/100)+rc.bottom;
  }
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


void MapWindow::DrawThermalEstimate(HDC hdc, RECT rc) {
  POINT screen;
  if (!EnableThermalLocator) 
    return;

  if (DisplayMode == dmCircling) {
    if (DerivedDrawInfo.ThermalEstimate_R>0) {
      LatLon2Screen(DerivedDrawInfo.ThermalEstimate_Longitude, 
                    DerivedDrawInfo.ThermalEstimate_Latitude, 
                    screen);
        DrawBitmapIn(hdc, 
                     screen, 
                     hBmpThermalSource);
        /*
      SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
      SelectObject(hdc, GetStockObject(BLACK_PEN));
      Circle(hdc,
             screen.x,
             screen.y, IBLSCALE(5), rc);
        */
    }
  } else {
    if (MapScale <= 4) {
      for (int i=0; i<MAX_THERMAL_SOURCES; i++) {
        if (DerivedDrawInfo.ThermalSources[i].Visible) {
          DrawBitmapIn(hdc, 
                       DerivedDrawInfo.ThermalSources[i].Screen, 
                       hBmpThermalSource);
        }
      }
    }
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
    DrawTerrain(hdc, rc, sunazimuth, sunelevation);
    if ((FinalGlideTerrain==2) && DerivedDrawInfo.TerrainValid) {
      DrawTerrainAbove(hdc, rc);
    }
    UnlockTerrainDataGraphics();
  }
  
  if (EnableTopology) {
    DrawTopology(hdc, rc);
  }
  
  // reset label over-write preventer
  nLabelBlocks = 0;
  
  if (!TaskIsTemporary()) {
    DrawTaskAAT(hdc, rc);
  }
  
  // then airspace..
  DrawAirSpace(hdc, rc);
  
  if(TrailActive) {
    // TODO: For some reason, the shadow drawing of the 
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
    // TODO don't draw offtrack indicator if showing spot heights
    DrawProjectedTrack(hdc, Orig_Aircraft);
    DrawOffTrackIndicator(hdc);
    DrawBestCruiseTrack(hdc, Orig_Aircraft);
    DrawBearing(hdc);
  }

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
    DrawCrossHairs(hdc, Orig);
  }
  
  if (extGPSCONNECT) {
    DrawAircraft(hdc, Orig_Aircraft);
  }
  // marks on top...
  DrawMarks(hdc, rc);
  SelectObject(hdcDrawWindow, hfOld);

}

void MapWindow::RenderMapWindow(  RECT rc)
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

  RenderMapWindowBg(hdcDrawWindow, rc, Orig, Orig_Aircraft);

  // overlays
  DrawCDI();

  hfOld = (HFONT)SelectObject(hdcDrawWindow, MapWindowFont);
  
  DrawMapScale(hdcDrawWindow,rc, BigZoom);

  DrawMapScale2(hdcDrawWindow,rc, Orig_Aircraft);
  
  DrawCompass(hdcDrawWindow, rc);

  // JMW Experimental only! EXPERIMENTAL
#if 0
  //  #ifdef GNAV
  if (EnableAuxiliaryInfo) {
    DrawHorizon(hdcDrawWindow, rc);
  }
  //  #endif
#endif

  DrawFlightMode(hdcDrawWindow, rc);

  DrawThermalBand(hdcDrawWindow, rc);

  DrawFinalGlide(hdcDrawWindow,rc);

  //  DrawSpeedToFly(hdcDrawWindow, rc);

  DrawGPSStatus(hdcDrawWindow, rc);

  SelectObject(hdcDrawWindow, hfOld);

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


DWORD MapWindow::DrawThread (LPVOID lpvoid)
{
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
    WaitForSingleObject(drawTriggerEvent, 5000);
    ResetEvent(drawTriggerEvent);
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

    RenderMapWindow(MapRect);
    
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


void MapWindow::DrawCrossHairs(HDC hdc, POINT Orig)
{
  POINT o1, o2;
  
  o1.x = Orig.x+20;
  o2.x = Orig.x-20;
  o1.y = Orig.y;
  o2.y = Orig.y;

  DrawDashLine(hdc, 1, o1, o2,
               RGB(50,50,50));

  o1.x = Orig.x;
  o2.x = Orig.x;
  o1.y = Orig.y+20;
  o2.y = Orig.y-20;

  DrawDashLine(hdc, 1, o1, o2,
               RGB(50,50,50));

}


void PolygonRotateShift(POINT* poly, const int n, const int xs, const int ys, const double angle) {
  static double lastangle = -1;
  static int cost=1024, sint=0;

  if(angle != lastangle) {
    lastangle = angle;
    int deg = DEG_TO_INT(AngleLimit360(angle));
    cost = ICOSTABLE[deg]*InfoBoxLayout::scale;
    sint = ISINETABLE[deg]*InfoBoxLayout::scale;
  }
  const int xxs = xs*1024+512;
  const int yys = ys*1024+512;
  POINT *p = poly;
  const POINT *pe = poly+n;

  while (p<pe) {
    int x= p->x;
    int y= p->y;
    p->x = (x*cost - y*sint + xxs)/1024;
    p->y = (y*cost + x*sint + yys)/1024;
    p++;
  }
}


void MapWindow::DrawAircraft(HDC hdc, POINT Orig)
{

  if (Appearance.Aircraft == afAircraftDefault){

  #define NUMAIRCRAFTPOINTS 16

    POINT Aircraft[NUMAIRCRAFTPOINTS] = {
      { 1,-6},
      {2,-1},
      {15,0},
      {15,2},
      {1,2},
      {0,10},
      {4,11},
      {4,12},
      {-4,12},
      {-4,11},
      {0,10},
      {-1,2},
      {-15,2},
      {-15,0},
      {-2,-1},
      {-1,-6}
    };

    int i;
    HPEN hpOld;
    HBRUSH hbAircraftSolid; 
    HBRUSH hbAircraftSolidBg;

    if (Appearance.InverseAircraft) {
      hbAircraftSolid = (HBRUSH) CreateSolidBrush(RGB(0xff,0xff,0xff));
      hbAircraftSolidBg = (HBRUSH) CreateSolidBrush(RGB(0x00,0x00,0x00));
    } else {
      hbAircraftSolid = (HBRUSH) CreateSolidBrush(RGB(0x00,0x00,0x00));
      hbAircraftSolidBg = (HBRUSH) CreateSolidBrush(RGB(0xff,0xff,0xff));
    }

    HBRUSH hbOld = (HBRUSH)SelectObject(hdc, hbAircraftSolidBg);
    hpOld = (HPEN)SelectObject(hdc, hpAircraft);
  
    PolygonRotateShift(Aircraft, NUMAIRCRAFTPOINTS, Orig.x+1, Orig.y+1,
                       DisplayAircraftAngle+
                       (DerivedDrawInfo.Heading-DrawInfo.TrackBearing));

    Polygon(hdc, Aircraft, NUMAIRCRAFTPOINTS);

    // draw it again so can get white border
    SelectObject(hdc, hpAircraftBorder);
    SelectObject(hdc, hbAircraftSolid);

    for(i=0; i<NUMAIRCRAFTPOINTS; i++)
    {
      Aircraft[i].x -= 1;  Aircraft[i].y -= 1;
    }

    Polygon(hdc, Aircraft, NUMAIRCRAFTPOINTS);

    SelectObject(hdc, hpOld);
    SelectObject(hdc, hbOld);

    DeleteObject(hbAircraftSolid);
    DeleteObject(hbAircraftSolidBg);
    
  } else

  if (Appearance.Aircraft == afAircraftAltA){

    HPEN oldPen;
    POINT Aircraft[] = {
      {1, -5},
      {1, 0},
      {14, 0}, 
      {14, 1}, 
      {1, 1},
      {1, 8},
      {4, 8},
      {4, 9},
      {-3, 9},
      {-3, 8},
      {0, 8},
      {0, 1},
      {-13, 1}, 
      {-13, 0}, 
      {0, 0},
      {0, -5},
      {1, -5},
    };

    /* Experiment, when turning show the high wing larger, 
       low wing smaller
    if (DerivedDrawInfo.TurnRate>10) {
      Aircraft[3].y = 0;
      Aircraft[12].y = 2;
    } else if (DerivedDrawInfo.TurnRate<-10) {
      Aircraft[3].y = 2;
      Aircraft[12].y = 0;
    }
    */

    int n = sizeof(Aircraft)/sizeof(Aircraft[0]);

    double angle = DisplayAircraftAngle+
      (DerivedDrawInfo.Heading-DrawInfo.TrackBearing);

    PolygonRotateShift(Aircraft, n,
                       Orig.x-1, Orig.y, angle);

    oldPen = (HPEN)SelectObject(hdc, hpAircraft);
    Polygon(hdc, Aircraft, n);

    HBRUSH hbOld;
    if (Appearance.InverseAircraft) {
      hbOld = (HBRUSH)SelectObject(hdc, GetStockObject(WHITE_BRUSH));
    } else {
      hbOld = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    }
    SelectObject(hdc, hpAircraftBorder); // hpBearing
    Polygon(hdc, Aircraft, n);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, hbOld);

  }

}

void MapWindow::DrawBitmapX(HDC hdc, int x, int y,
                            int sizex, int sizey,
                            HDC source,
                            int offsetx, int offsety,
                            DWORD mode) {
  if (InfoBoxLayout::scale>1) {
    StretchBlt(hdc, x, y, 
               IBLSCALE(sizex), 
               IBLSCALE(sizey), 
               source,
               offsetx, offsety, sizex, sizey,
               mode);
  } else {
    BitBlt(hdc, x, y, sizex, sizey, 
           source, offsetx, offsety, mode); 
  }
}

void MapWindow::DrawBitmapIn(const HDC hdc, const POINT &sc, const HBITMAP h) {
  if (!PointVisible(sc)) return;

  SelectObject(hDCTemp, h);

  DrawBitmapX(hdc,
              sc.x-IBLSCALE(5),
              sc.y-IBLSCALE(5),
              10,10,
    hDCTemp,0,0,SRCPAINT);
  DrawBitmapX(hdc,
              sc.x-IBLSCALE(5),
              sc.y-IBLSCALE(5),
              10,10,
              hDCTemp,10,0,SRCAND);
}


void MapWindow::DrawGPSStatus(HDC hDC, RECT rc)
{

  if (extGPSCONNECT && !(DrawInfo.NAVWarning) && (DrawInfo.SatellitesUsed != 0)) 
    // nothing to do
    return;

  TCHAR gpswarningtext1[] = TEXT("GPS not connected");
  TCHAR gpswarningtext2[] = TEXT("GPS waiting for fix");
  TextInBoxMode_t TextInBoxMode = {2};

  if (!extGPSCONNECT) {
    SelectObject(hDCTemp,hGPSStatus2);
    DrawBitmapX(hDC, 
                rc.left+IBLSCALE(2),
                rc.bottom+IBLSCALE(Appearance.GPSStatusOffset.y-22),
                20, 20,
                hDCTemp, 
                0, 0, SRCAND);

    TextInBox(hDC, gettext(gpswarningtext1), 
              rc.left+IBLSCALE(24), 
              rc.bottom+IBLSCALE(Appearance.GPSStatusOffset.y-19),
              0, TextInBoxMode);

  } else
    if (DrawInfo.NAVWarning || (DrawInfo.SatellitesUsed == 0)) {
      SelectObject(hDCTemp,hGPSStatus1);

      DrawBitmapX(hDC, 
                  rc.left+IBLSCALE(2),
                  rc.bottom+IBLSCALE(Appearance.GPSStatusOffset.y-22),
                  20, 20,
                  hDCTemp, 
                  0, 0, SRCAND);

      TextInBox(hDC, gettext(gpswarningtext2), 
                rc.left+IBLSCALE(24), 
                rc.bottom+
                IBLSCALE(Appearance.GPSStatusOffset.y-19),
                0, TextInBoxMode);

    }

}

void MapWindow::DrawFlightMode(HDC hdc, RECT rc)
{
  static bool flip= true;
  static double LastTime = 0;
  bool drawlogger = true;
  static bool lastLoggerActive=false;
  int offset = -1;

  if (!Appearance.DontShowLoggerIndicator){

    // has GPS time advanced?
    if(DrawInfo.Time <= LastTime) {
        LastTime = DrawInfo.Time;
    } else {
      flip = !flip;

      // don't bother drawing logger if not active for more than one second
      if ((!LoggerActive)&&(!lastLoggerActive)) {
        drawlogger = false;
      }
      lastLoggerActive = LoggerActive;
    }

    if (drawlogger) {
      offset -= 7;

      if (LoggerActive && flip) {
        SelectObject(hDCTemp,hLogger);
      } else {
        SelectObject(hDCTemp,hLoggerOff);
      }
      //changed draw mode & icon for higher opacity 12aug -st
      DrawBitmapX(hdc,
                  rc.right+IBLSCALE(offset+Appearance.FlightModeOffset.x),
                  rc.bottom+IBLSCALE(-7+Appearance.FlightModeOffset.y),
                  7,7,
                  hDCTemp,
                  0,0,SRCPAINT);

      DrawBitmapX(hdc,
                  rc.right+IBLSCALE(offset+Appearance.FlightModeOffset.x),
                  rc.bottom+IBLSCALE(-7+Appearance.FlightModeOffset.y),
                  7,7,
                  hDCTemp,
                  7,0,SRCAND);
    }
  }

  if (Appearance.FlightModeIcon == apFlightModeIconDefault){

    if (TaskAborted) {
      SelectObject(hDCTemp,hAbort);
    } else {
      if (DisplayMode == dmCircling) {
        SelectObject(hDCTemp,hClimb);
      } else if (DisplayMode == dmFinalGlide) {
        SelectObject(hDCTemp,hFinalGlide);
      } else {
        SelectObject(hDCTemp,hCruise);
      }
    }
    // Code already commented as of 12aug05 - redundant? -st
    //          BitBlt(hdc,rc.right-35,5,24,20,
    //                           hDCTemp,20,0,SRCAND);

    // code for pre 12aug icons - st
    //BitBlt(hdc,rc.right-24-3,rc.bottom-20-3,24,20,
    //  hDCTemp,0,0,SRCAND);

    offset -= 24;

    DrawBitmapX(hdc,
                rc.right+IBLSCALE(offset-1+Appearance.FlightModeOffset.x),
                rc.bottom+IBLSCALE(-20-1+Appearance.FlightModeOffset.y),
                24,20,
                hDCTemp,
                0,0,SRCPAINT);
    
    DrawBitmapX(hdc,
                rc.right+IBLSCALE(offset-1+Appearance.FlightModeOffset.x),
                rc.bottom+IBLSCALE(-20-1+Appearance.FlightModeOffset.y),
                24,20,
                hDCTemp,
                24,0,SRCAND);

  } else if (Appearance.FlightModeIcon == apFlightModeIconAltA){

#define SetPoint(Idx,X,Y) Arrow[Idx].x = X; Arrow[Idx].y = Y

    POINT Arrow[3];
    POINT Center;
    HBRUSH oldBrush;
    HPEN   oldPen;

    Center.x = rc.right-10;
    Center.y = rc.bottom-10;

    if (DisplayMode == dmCircling) {

      SetPoint(0, 
               Center.x,
               Center.y-IBLSCALE(4));
      SetPoint(1, 
               Center.x-IBLSCALE(8), 
               Center.y+IBLSCALE(4));
      SetPoint(2, 
               Center.x+IBLSCALE(8), 
               Center.y+IBLSCALE(4));

    } else if (DisplayMode == dmFinalGlide) {

      SetPoint(0, 
               Center.x, 
               Center.y+IBLSCALE(4));
      SetPoint(1, 
               Center.x-IBLSCALE(8), 
               Center.y-IBLSCALE(4));
      SetPoint(2, 
               Center.x+IBLSCALE(8), 
               Center.y-IBLSCALE(4));
    } else {

      SetPoint(0, 
               Center.x+IBLSCALE(4), 
               Center.y);
      SetPoint(1, 
               Center.x-IBLSCALE(4), 
               Center.y+IBLSCALE(8));
      SetPoint(2, 
               Center.x-IBLSCALE(4), 
               Center.y-IBLSCALE(8));

    }

    if (TaskAborted)
      oldBrush = (HBRUSH)SelectObject(hdc, hBrushFlyingModeAbort);
    else
      oldBrush = (HBRUSH)SelectObject(hdc, hbCompass);

    oldPen = (HPEN)SelectObject(hdc, hpCompassBorder);
    Polygon(hdc, Arrow, 3);

    SelectObject(hdc, hpCompass);
    Polygon(hdc, Arrow, 3);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);

  }


  if (!Appearance.DontShowAutoMacCready && DerivedDrawInfo.AutoMacCready) {
    SelectObject(hDCTemp,hAutoMacCready);

    offset -= 24;

    //changed draw mode & icon for higher opacity 12aug -st

      DrawBitmapX(hdc,
                  rc.right+IBLSCALE(offset-3+Appearance.FlightModeOffset.x),
                  rc.bottom+IBLSCALE(-20-3+Appearance.FlightModeOffset.y),
                  24,20,
                  hDCTemp,
                  0,0,SRCPAINT);

      DrawBitmapX(hdc,
                  rc.right+IBLSCALE(offset-3+Appearance.FlightModeOffset.x),
                  rc.bottom+IBLSCALE(-20-3+Appearance.FlightModeOffset.y),
                  24,20,
                  hDCTemp,
                  24,0,SRCAND);

    //  commented @ 12aug st
    //  BitBlt(hdc,rc.right-48-3,rc.bottom-20-3,24,20,
    //    hDCTemp,0,0,SRCAND);
  };
  
}


typedef struct{
  TCHAR Name[NAME_SIZE+1];
  POINT Pos;
  TextInBoxMode_t Mode;
  int AltArivalAGL;
  bool inTask;
}MapWaypointLabel_t;

bool MapWindow::WaypointInTask(int ind) {
  if (!WayPointList) return false;
  return WayPointList[ind].InTask;
}

static void MapWaypointLabelAdd(TCHAR *Name, int X, int Y, TextInBoxMode_t Mode, int AltArivalAGL, bool inTask=false);
static int _cdecl MapWaypointLabelListCompare(const void *elem1, const void *elem2 );
static MapWaypointLabel_t MapWaypointLabelList[50];
static int MapWaypointLabelListCount=0;

bool MapWindow::WaypointInRange(int i) {
  return ((WayPointList[i].Zoom >= MapScale*10) 
          || (WayPointList[i].Zoom == 0)) 
    && (MapScale <= 10);
}

void MapWindow::DrawWaypoints(HDC hdc, RECT rc)
{
  unsigned int i;
  TCHAR Buffer[32];
  TCHAR Buffer2[32];
  TCHAR sAltUnit[4];
  TextInBoxMode_t TextDisplayMode;

  // if pan mode, show full names
  int pDisplayTextType = DisplayTextType;
  if (EnablePan) {
    pDisplayTextType = DISPLAYNAME;
  }

  if (!WayPointList) return;

  _tcscpy(sAltUnit, Units::GetAltitudeName());

  MapWaypointLabelListCount = 0;

  for(i=0;i<NumberOfWayPoints;i++)
  {
    if(WayPointList[i].Visible )
    {

      #ifdef HAVEEXCEPTIONS
      __try{
      #endif

      bool irange = false;
      bool intask = false;
      bool islandable = false;
      bool dowrite;

      intask = WaypointInTask(i);
      dowrite = intask;

      TextDisplayMode.AsInt = 0;

      irange = WaypointInRange(i);

      if(MapScale > 20) {
        SelectObject(hDCTemp,hSmall);
      } else if( ((WayPointList[i].Flags & AIRPORT) == AIRPORT) 
                 || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) ) {
        islandable = true; // so we can always draw them
        if(WayPointList[i].Reachable){

	  TextDisplayMode.AsFlag.Reachable = 1;

	  if ((DeclutterLabels<2)||intask) {

	    if (intask || (DeclutterLabels<1)) {
	      TextDisplayMode.AsFlag.Border = 1;
	    }
            // show all reachable landing fields unless we want a decluttered
            // screen.
            dowrite = true;
          }

          if ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
            SelectObject(hDCTemp,hBmpAirportReachable);
          else
            SelectObject(hDCTemp,hBmpFieldReachable);
        } else {
          if ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
            SelectObject(hDCTemp,hBmpAirportUnReachable);
          else
            SelectObject(hDCTemp,hBmpFieldUnReachable);
        }
      } else {
        if(MapScale > 4) {
          SelectObject(hDCTemp,hSmall);
        } else {
          SelectObject(hDCTemp,hTurnPoint);
        }
      }

      if (intask) {
	TextDisplayMode.AsFlag.WhiteBold = 1;
      }

      if(irange || intask || islandable || dowrite) {
        
        DrawBitmapX(hdc,
                    WayPointList[i].Screen.x-IBLSCALE(10), 
                    WayPointList[i].Screen.y-IBLSCALE(10),
                    20,20,
                    hDCTemp,0,0,SRCPAINT);
        
        DrawBitmapX(hdc,
                    WayPointList[i].Screen.x-IBLSCALE(10), 
                    WayPointList[i].Screen.y-IBLSCALE(10),
                    20,20,
                    hDCTemp,20,0,SRCAND);
      }

      if(intask || irange || dowrite) {
	bool draw_alt = TextDisplayMode.AsFlag.Reachable 
	  && ((DeclutterLabels<1) || intask);

        switch(pDisplayTextType) {
        case DISPLAYNAMEIFINTASK:
	  dowrite = intask;
          if (intask) {
            if (draw_alt)
              wsprintf(Buffer, TEXT("%s:%d%s"),
                       WayPointList[i].Name, 
                       (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
                       sAltUnit);
            else
              wsprintf(Buffer, TEXT("%s"),WayPointList[i].Name);
          }
          break;
        case DISPLAYNAME:
	  dowrite = (DeclutterLabels<2) || intask;
          if (draw_alt)
            wsprintf(Buffer, TEXT("%s:%d%s"),
                     WayPointList[i].Name, 
                     (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
                     sAltUnit);
          else
            wsprintf(Buffer, TEXT("%s"),WayPointList[i].Name);
          
          break;
        case DISPLAYNUMBER:
	  dowrite = (DeclutterLabels<2) || intask;
          if (draw_alt)
            wsprintf(Buffer, TEXT("%d:%d%s"),
                     WayPointList[i].Number, 
                     (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
                     sAltUnit);
          else
            wsprintf(Buffer, TEXT("%d"),WayPointList[i].Number);
          
          break;
        case DISPLAYFIRSTFIVE:
	  dowrite = (DeclutterLabels<2) || intask;
          _tcsncpy(Buffer2, WayPointList[i].Name, 5);
          Buffer2[5] = '\0';
          if (draw_alt)
            wsprintf(Buffer, TEXT("%s:%d%s"),
                     Buffer2, 
                     (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
                     sAltUnit);
          else
            wsprintf(Buffer, TEXT("%s"),Buffer2);
          
          break;
        case DISPLAYFIRSTTHREE:
	  dowrite = (DeclutterLabels<2) || intask;
          _tcsncpy(Buffer2, WayPointList[i].Name, 3);
          Buffer2[3] = '\0';
          if (draw_alt)
            wsprintf(Buffer, TEXT("%s:%d%s"),
                     Buffer2, 
                     (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
                     sAltUnit);
          else
            wsprintf(Buffer, TEXT("%s"),Buffer2);
          
          break;
        case DISPLAYNONE:
	  dowrite = (DeclutterLabels<2) || intask;
          if (draw_alt)
            wsprintf(Buffer, TEXT("%d%s"), 
                     (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
                     sAltUnit);
          else
            Buffer[0]= '\0';
        default:
#if (WINDOWSPC<1)
          ASSERT(0);
#endif
          break;
        }
        
        if (dowrite) {
          MapWaypointLabelAdd(
                              Buffer,
                              WayPointList[i].Screen.x+5,
                              WayPointList[i].Screen.y,
                              TextDisplayMode,
                              (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
                              intask);
        }
        
      }
      
#ifdef HAVEEXCEPTIONS
      }__finally
#endif
         { ; }
    }
  }
  
  qsort(&MapWaypointLabelList, 
        MapWaypointLabelListCount,
        sizeof(MapWaypointLabel_t), 
        MapWaypointLabelListCompare);

  int j;

  // now draw task/landable waypoints in order of range (closest last)
  // writing unconditionally
  for (j=MapWaypointLabelListCount-1; j>=0; j--){
    MapWaypointLabel_t *E = &MapWaypointLabelList[j];
    // draws if they are in task unconditionally,
    // otherwise, does comparison
    if (E->inTask) {
      TextInBox(hdc, E->Name, E->Pos.x,
                E->Pos.y, 0, E->Mode, 
                false);
    }
  }

  // now draw normal waypoints in order of range (furthest away last)
  // without writing over each other (or the task ones)
  for (j=0; j<MapWaypointLabelListCount; j++) {
    MapWaypointLabel_t *E = &MapWaypointLabelList[j];
    if (!E->inTask) {
      TextInBox(hdc, E->Name, E->Pos.x,
                E->Pos.y, 0, E->Mode, 
                true);
    }
  }

}

static int _cdecl MapWaypointLabelListCompare(const void *elem1, const void *elem2 ){

  // Now sorts elements in task preferentially.
  /*
  if (((MapWaypointLabel_t *)elem1)->inTask && ! ((MapWaypointLabel_t *)elem2)->inTask)
    return (-1);
  */
  if (((MapWaypointLabel_t *)elem1)->AltArivalAGL > ((MapWaypointLabel_t *)elem2)->AltArivalAGL)
    return (-1);
  if (((MapWaypointLabel_t *)elem1)->AltArivalAGL < ((MapWaypointLabel_t *)elem2)->AltArivalAGL)
    return (+1);
  return (0);
}


static void MapWaypointLabelAdd(TCHAR *Name, int X, int Y, 
                                TextInBoxMode_t Mode, 
                                int AltArivalAGL, bool inTask){
  MapWaypointLabel_t *E;

  if ((X<MapWindow::MapRect.left-WPCIRCLESIZE)
      || (X>MapWindow::MapRect.right+(WPCIRCLESIZE*3))
    || (Y<MapWindow::MapRect.top-WPCIRCLESIZE)
         || (Y>MapWindow::MapRect.bottom+WPCIRCLESIZE)){
          return;
  }

  if (MapWaypointLabelListCount >= (sizeof(MapWaypointLabelList)/sizeof(MapWaypointLabel_t))-1){
#if (WINDOWSPC<1)
    ASSERT(0);
#endif
    return;
  }

  E = &MapWaypointLabelList[MapWaypointLabelListCount];

  _tcscpy(E->Name, Name);
  E->Pos.x = X;
  E->Pos.y = Y;
  E->Mode = Mode;
  E->AltArivalAGL = AltArivalAGL;
  E->inTask = inTask;

  MapWaypointLabelListCount++;

}


void MapWindow::DrawAbortedTask(HDC hdc, RECT rc, POINT me)
{
  int i;
  if (!WayPointList) return;
  
  LockTaskData();  // protect from external task changes
  #ifdef HAVEEXCEPTIONS
  __try{
  #endif
  for(i=0;i<MAXTASKPOINTS-1;i++)
  {
    int index = Task[i].Index;
    if(ValidWayPoint(index))
    {
      DrawDashLine(hdc, 1, 
        WayPointList[index].Screen,
        me,
        taskcolor);
    }
    }
  #ifdef HAVEEXCEPTIONS
  }__finally
  #endif
  {
    UnlockTaskData();
  }
}


void MapWindow::DrawStartSector(HDC hdc, RECT rc, 
                                POINT &Start,
                                POINT &End, int Index) {
  double tmp;

  if(StartLine) {
    _DrawLine(hdc, PS_SOLID, IBLSCALE(5), WayPointList[Index].Screen,
              Start, taskcolor);
    _DrawLine(hdc, PS_SOLID, IBLSCALE(5), WayPointList[Index].Screen,
              End, taskcolor);
    _DrawLine(hdc, PS_SOLID, IBLSCALE(1), WayPointList[Index].Screen,
              Start, RGB(255,0,0));
    _DrawLine(hdc, PS_SOLID, IBLSCALE(1), WayPointList[Index].Screen,
              End, RGB(255,0,0));
  } else {
    tmp = StartRadius*ResMapScaleOverDistanceModify;
    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    SelectObject(hdc, hpStartFinishThick);
    Circle(hdc,
           WayPointList[Index].Screen.x,
           WayPointList[Index].Screen.y,(int)tmp, rc, false, false);
    SelectObject(hdc, hpStartFinishThin);
    Circle(hdc,
           WayPointList[Index].Screen.x,
           WayPointList[Index].Screen.y,(int)tmp, rc, false, false);
  }

}


void MapWindow::DrawTask(HDC hdc, RECT rc, const POINT &Orig_Aircraft)
{
  int i;
  double tmp;

  COLORREF whitecolor = RGB(0xff,0xff, 0xff);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  if (!WayPointList) return;

  LockTaskData();  // protect from external task changes
  #ifdef HAVEEXCEPTIONS
  __try{
  #endif

  if(ValidTaskPoint(0) && ValidTaskPoint(1) && (ActiveWayPoint<2))
  {
    DrawStartSector(hdc,rc, Task[0].Start, Task[0].End, Task[0].Index);
    if (EnableMultipleStartPoints) {
      for (i=0; i<MAXSTARTPOINTS; i++) {
        if (StartPoints[i].Active && ValidWayPoint(StartPoints[i].Index)) {
          DrawStartSector(hdc,rc, 
                          StartPoints[i].Start, 
                          StartPoints[i].End, StartPoints[i].Index);
        }
      }
    }
  }
  
  for(i=1;i<MAXTASKPOINTS-1;i++) {

    if(ValidTaskPoint(i) && !ValidTaskPoint(i+1)) { // final waypoint
      if (ActiveWayPoint>1) { 
        // only draw finish line when past the first
        // waypoint.
        if(FinishLine) {
          _DrawLine(hdc, PS_SOLID, IBLSCALE(5), 
                    WayPointList[Task[i].Index].Screen,
                    Task[i].Start, taskcolor);
          _DrawLine(hdc, PS_SOLID, IBLSCALE(5), 
                    WayPointList[Task[i].Index].Screen,
                    Task[i].End, taskcolor);
          _DrawLine(hdc, PS_SOLID, IBLSCALE(1), 
                    WayPointList[Task[i].Index].Screen,
                    Task[i].Start, RGB(255,0,0));
          _DrawLine(hdc, PS_SOLID, IBLSCALE(1), 
                    WayPointList[Task[i].Index].Screen,
                    Task[i].End, RGB(255,0,0));
        } else {
          tmp = FinishRadius*ResMapScaleOverDistanceModify; 
          SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
          SelectObject(hdc, hpStartFinishThick);
          Circle(hdc,
                 WayPointList[Task[i].Index].Screen.x,
                 WayPointList[Task[i].Index].Screen.y,
                 (int)tmp, rc, false, false); 
          SelectObject(hdc, hpStartFinishThin);
          Circle(hdc,
                 WayPointList[Task[i].Index].Screen.x,
                 WayPointList[Task[i].Index].Screen.y,
                 (int)tmp, rc, false, false); 
        }        
      }
    }
    if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) { // normal sector
      if(AATEnabled != TRUE) {
        DrawDashLine(hdc, 2, 
                     WayPointList[Task[i].Index].Screen, 
                     Task[i].Start, RGB(127,127,127));
        DrawDashLine(hdc, 2, 
                     WayPointList[Task[i].Index].Screen, 
                     Task[i].End, RGB(127,127,127));
        
        SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        SelectObject(hdc, GetStockObject(BLACK_PEN));
        if(SectorType== 0) {
          tmp = SectorRadius*ResMapScaleOverDistanceModify;
          Circle(hdc,
                 WayPointList[Task[i].Index].Screen.x,
                 WayPointList[Task[i].Index].Screen.y,
                 (int)tmp, rc, false, false); 
        }
        if(SectorType==1) {
          tmp = SectorRadius*ResMapScaleOverDistanceModify;
          Segment(hdc,
                  WayPointList[Task[i].Index].Screen.x,
                  WayPointList[Task[i].Index].Screen.y,(int)tmp, rc, 
                  Task[i].AATStartRadial-DisplayAngle, 
                  Task[i].AATFinishRadial-DisplayAngle); 
        }
        if(SectorType== 2) {
          // JMW added german rules
          tmp = 500*ResMapScaleOverDistanceModify;
          Circle(hdc,
                 WayPointList[Task[i].Index].Screen.x,
                 WayPointList[Task[i].Index].Screen.y,
                 (int)tmp, rc, false, false); 

          tmp = 10e3*ResMapScaleOverDistanceModify;
          
          Segment(hdc,
                  WayPointList[Task[i].Index].Screen.x,
                  WayPointList[Task[i].Index].Screen.y,(int)tmp, rc, 
                  Task[i].AATStartRadial-DisplayAngle, 
                  Task[i].AATFinishRadial-DisplayAngle); 

        }        
      } else {
        // JMW added iso lines
        if ((i==ActiveWayPoint) || (TargetPan && (i==TargetPanIndex))) {
	  // JMW 20080616 flash arc line if very close to target
	  static bool flip = false;
	  
	  if (DerivedDrawInfo.WaypointDistance<AATCloseDistance()*2.0) {
	    flip = !flip;
	  } else {
	    flip = true;
	  }
	  if (flip) {
	    for (int j=0; j<MAXISOLINES-1; j++) {
	      if (TaskStats[i].IsoLine_valid[j] 
		  && TaskStats[i].IsoLine_valid[j+1]) {
		_DrawLine(hdc, PS_SOLID, IBLSCALE(2), 
			  TaskStats[i].IsoLine_Screen[j], 
			  TaskStats[i].IsoLine_Screen[j+1],
			  RGB(0,0,255));
	      }
	    }
          }
        }
      }
    }
  }

  for(i=0;i<MAXTASKPOINTS-1;i++) {
    if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) {
      bool is_first = (Task[i].Index < Task[i+1].Index);
      int imin = min(Task[i].Index,Task[i+1].Index);
      int imax = max(Task[i].Index,Task[i+1].Index);
      // JMW AAT!
      double bearing = Task[i].OutBound;
      POINT sct1, sct2;
      if (AATEnabled && !TargetPan) {
	LatLon2Screen(Task[i].AATTargetLon, 
		      Task[i].AATTargetLat, 
		      sct1);
        LatLon2Screen(Task[i+1].AATTargetLon, 
                      Task[i+1].AATTargetLat, 
                      sct2);
	DistanceBearing(Task[i].AATTargetLat,
			Task[i].AATTargetLon,
			Task[i+1].AATTargetLat,
			Task[i+1].AATTargetLon,
			NULL, &bearing);

	// draw nominal track line
        DrawDashLine(hdc, 1, 
                     WayPointList[imin].Screen, 
                     WayPointList[imax].Screen, 
                     taskcolor);
      } else {
	sct1 = WayPointList[Task[i].Index].Screen;
	sct2 = WayPointList[Task[i+1].Index].Screen;
      }

      if (is_first) {
	DrawDashLine(hdc, 3, 
		     sct1, 
		     sct2, 
		     taskcolor);
      } else {
	DrawDashLine(hdc, 3, 
		     sct2, 
		     sct1, 
		     taskcolor);
      }

      // draw small arrow along task direction
      POINT p_p;
      POINT Arrow[2] = { {6,6}, {-6,6} };
      ScreenClosestPoint(sct1, sct2, 
			 Orig_Aircraft, &p_p, IBLSCALE(25));
      PolygonRotateShift(Arrow, 2, p_p.x, p_p.y, 
            bearing-DisplayAngle);

      _DrawLine(hdc, PS_SOLID, IBLSCALE(2), Arrow[0], p_p, taskcolor);
      _DrawLine(hdc, PS_SOLID, IBLSCALE(2), Arrow[1], p_p, taskcolor);
    }
  }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
     {
       UnlockTaskData();
     }

  // restore original color
  SetTextColor(hDCTemp, origcolor);

}


void MapWindow::DrawTaskAAT(HDC hdc, RECT rc)
{
  int i;
  double tmp;

  if (!WayPointList) return;
  if (!AATEnabled) return;
  
  LockTaskData();  // protect from external task changes
  #ifdef HAVEEXCEPTIONS
  __try{
  #endif

  COLORREF whitecolor = RGB(0xff,0xff, 0xff);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);

  SelectObject(hDCTemp, GetStockObject(WHITE_PEN));
  SelectObject(hDCTemp, GetStockObject(WHITE_BRUSH));
  Rectangle(hDCTemp,rc.left,rc.top,rc.right,rc.bottom);
    
  for(i=MAXTASKPOINTS-2;i>0;i--)
  {
    if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) {
      if(Task[i].AATType == CIRCLE)
        {
          tmp = Task[i].AATCircleRadius*ResMapScaleOverDistanceModify;
          
          // this color is used as the black bit
          SetTextColor(hDCTemp, 
                       Colours[iAirspaceColour[AATASK]]);
          
          // this color is the transparent bit
          SetBkColor(hDCTemp, 
                     whitecolor);

          if (i<ActiveWayPoint) {
            SelectObject(hDCTemp, GetStockObject(HOLLOW_BRUSH));
          } else {
            SelectObject(hDCTemp, hAirspaceBrushes[iAirspaceBrush[AATASK]]);
          }
          SelectObject(hDCTemp, GetStockObject(BLACK_PEN));
          
          Circle(hDCTemp,
                 WayPointList[Task[i].Index].Screen.x,
                 WayPointList[Task[i].Index].Screen.y,
                 (int)tmp, rc, true, true); 
        }
      else
        {
          
          // this color is used as the black bit
          SetTextColor(hDCTemp, 
                       Colours[iAirspaceColour[AATASK]]);
          
          // this color is the transparent bit
          SetBkColor(hDCTemp, 
                     whitecolor);
          
          if (i<ActiveWayPoint) {
            SelectObject(hDCTemp, GetStockObject(HOLLOW_BRUSH));
          } else {
            SelectObject(hDCTemp, hAirspaceBrushes[iAirspaceBrush[AATASK]]);
          }
          SelectObject(hDCTemp, GetStockObject(BLACK_PEN));
          
          tmp = Task[i].AATSectorRadius*ResMapScaleOverDistanceModify;
          
          Segment(hDCTemp,
                  WayPointList[Task[i].Index].Screen.x,
                  WayPointList[Task[i].Index].Screen.y,(int)tmp, rc, 
                  Task[i].AATStartRadial-DisplayAngle, 
                  Task[i].AATFinishRadial-DisplayAngle); 
          
          DrawSolidLine(hDCTemp,
                        WayPointList[Task[i].Index].Screen, Task[i].AATStart);
          DrawSolidLine(hDCTemp,
                        WayPointList[Task[i].Index].Screen, Task[i].AATFinish);

        }

    }
  }

  // restore original color
  SetTextColor(hDCTemp, origcolor);

  //////

  #if (WINDOWSPC<1)
  TransparentImage(hdc,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   hDCTemp,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   whitecolor
                   );

  #else
  TransparentBlt(hdc,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   hDCTemp,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   whitecolor
                   );
  #endif
  
  #ifdef HAVEEXCEPTIONS
  }__finally
  #endif
  {
    UnlockTaskData();
  }
}


void MapWindow::DrawWindAtAircraft2(HDC hdc, POINT Orig, RECT rc) {
  int i;
  POINT Start;
  HPEN hpOld;
  HBRUSH hbOld; 
  TCHAR sTmp[12];
  static SIZE tsize = {0,0};
  
  if (DerivedDrawInfo.WindSpeed<1) {
    return; // JMW don't bother drawing it if not significant
  }
  
  if (tsize.cx == 0){

    HFONT oldFont = (HFONT)SelectObject(hdc, MapWindowBoldFont);
    GetTextExtentPoint(hdc, TEXT("99"), 2, &tsize);
    SelectObject(hdc, oldFont);
    tsize.cx = tsize.cx/2;
  }

  hpOld = (HPEN)SelectObject(hdc, hpWind);
  hbOld = (HBRUSH)SelectObject(hdc, hbWind);
  
  int wmag = iround(4.0*DerivedDrawInfo.WindSpeed);
  
  Start.y = Orig.y;
  Start.x = Orig.x;

  int kx = tsize.cx/InfoBoxLayout::scale/2;

  POINT Arrow[7] = { {0,-20}, {-6,-26}, {0,-20}, 
                     {6,-26}, {0,-20}, 
                     {8+kx, -24}, 
                     {-8-kx, -24}};

  for (i=1;i<4;i++)
    Arrow[i].y -= wmag;

  PolygonRotateShift(Arrow, 7, Start.x, Start.y, 
            DerivedDrawInfo.WindBearing-DisplayAngle);

  if (WindArrowStyle==1) {
    POINT Tail[2] = {{0,-20}, {0,-26-min(20,wmag)*3}};
    double angle = AngleLimit360(DerivedDrawInfo.WindBearing-DisplayAngle);
    for(i=0; i<2; i++) {
      if (InfoBoxLayout::scale>1) {
        Tail[i].x *= InfoBoxLayout::scale;
        Tail[i].y *= InfoBoxLayout::scale;
      }
      protateshift(Tail[i], angle, Start.x, Start.y);
    }

    // optionally draw dashed line
    _DrawLine(hdc, PS_DASH, 1, Tail[0], Tail[1], RGB(0,0,0));
  }

  _itot(iround(DerivedDrawInfo.WindSpeed * SPEEDMODIFY), sTmp, 10);

  TextInBoxMode_t TextInBoxMode = { 16 | 32 }; // JMW test {2 | 16};
  if (Arrow[5].y>=Arrow[6].y) {
    TextInBox(hdc, sTmp, Arrow[5].x-kx, Arrow[5].y, 0, TextInBoxMode);
  } else {
    TextInBox(hdc, sTmp, Arrow[6].x-kx, Arrow[6].y, 0, TextInBoxMode);
  }

  Polygon(hdc,Arrow,5);

  SelectObject(hdc, hbOld);
  SelectObject(hdc, hpOld);
}


void MapWindow::DrawBearing(HDC hdc)
{

  if (!ValidTaskPoint(ActiveWayPoint)) {
    return; 
  }

  LockTaskData();  // protect from external task changes

  double startLat = DrawInfo.Latitude;
  double startLon = DrawInfo.Longitude;
  double targetLat;
  double targetLon;

  if (AATEnabled && (ActiveWayPoint>0) && ValidTaskPoint(ActiveWayPoint+1)) {
    targetLat = Task[ActiveWayPoint].AATTargetLat;
    targetLon = Task[ActiveWayPoint].AATTargetLon; 
  } else {
    targetLat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    targetLon = WayPointList[Task[ActiveWayPoint].Index].Longitude; 
  }
  UnlockTaskData();

  DrawGreatCircle(hdc, startLon, startLat,
                  targetLon, targetLat);

  if (TargetPan) {
    // Draw all of task if in target pan mode
    // TODO: draw inbound/outbound arrows?
    startLat = targetLat;
    startLon = targetLon;

    LockTaskData();
    for (int i=ActiveWayPoint+1; i<MAXTASKPOINTS; i++) {
      if (ValidTaskPoint(i)) {

        if (AATEnabled && ValidTaskPoint(i+1)) {
          targetLat = Task[i].AATTargetLat;
          targetLon = Task[i].AATTargetLon; 
        } else {
          targetLat = WayPointList[Task[i].Index].Latitude;
          targetLon = WayPointList[Task[i].Index].Longitude; 
        }
       
        DrawGreatCircle(hdc, startLon, startLat,
                        targetLon, targetLat);

        startLat = targetLat;
        startLon = targetLon;
      }
    }

    // JMW draw symbol at target, makes it easier to see

    if (AATEnabled) {
      for (int i=ActiveWayPoint+1; i<MAXTASKPOINTS; i++) {
        if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) {
          if (i>= ActiveWayPoint) {
            POINT sct;
            LatLon2Screen(Task[i].AATTargetLon, 
                          Task[i].AATTargetLat, 
                          sct);
            DrawBitmapIn(hdc, sct, hBmpTarget);
          }
        }
      }
    }

    UnlockTaskData();

  }

  if (AATEnabled) {
    LockTaskData();
    if (ValidTaskPoint(ActiveWayPoint+1) && (ActiveWayPoint>0)) {
      POINT sct;
      LatLon2Screen(Task[ActiveWayPoint].AATTargetLon, 
                    Task[ActiveWayPoint].AATTargetLat, 
                    sct);
      DrawBitmapIn(hdc, sct, hBmpTarget);
    }
    UnlockTaskData();
  }
}


double MapWindow::GetApproxScreenRange() {
  return (MapScale * max(MapRectBig.right-MapRectBig.left,
                         MapRectBig.bottom-MapRectBig.top))
    *1000.0/GetMapResolutionFactor();
}




extern bool ScreenBlanked;

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
  SetEvent(drawTriggerEvent); // wake self up
  LockTerrainDataGraphics();
  SuspendDrawingThread();
  UnlockTerrainDataGraphics();
  while(!THREADEXIT) { Sleep(100); };
}





bool MapWindow::PointInRect(const double &lon, const double &lat,
                            const rectObj &bounds) {
  if ((lon> bounds.minx) &&
      (lon< bounds.maxx) &&
      (lat> bounds.miny) &&
      (lat< bounds.maxy)) 
    return true;
  else
    return false;
}


bool MapWindow::PointVisible(const double &lon, const double &lat) {
  if ((lon> screenbounds_latlon.minx) &&
      (lon< screenbounds_latlon.maxx) &&
      (lat> screenbounds_latlon.miny) &&
      (lat< screenbounds_latlon.maxy)) 
    return true;
  else
    return false;
}


bool MapWindow::PointVisible(const POINT &P)
{
  if(( P.x >= MapRect.left ) 
    &&
    ( P.x <= MapRect.right ) 
    &&
    ( P.y >= MapRect.top  ) 
    &&
    ( P.y <= MapRect.bottom  ) 
    )
    return TRUE;
  else
    return FALSE;
}


void MapWindow::DisplayAirspaceWarning(int Type, TCHAR *Name , 
                                       AIRSPACE_ALT Base, AIRSPACE_ALT Top )
{
  TCHAR szMessageBuffer[1024];
  TCHAR szTitleBuffer[1024];
  
  FormatWarningString(Type, Name , Base, Top, szMessageBuffer, szTitleBuffer );

  DoStatusMessage(TEXT("Airspace Query"), szMessageBuffer);
}


////////////////////////////////////////////////////////////////////

// RETURNS Longitude, Latitude!

void MapWindow::OrigScreen2LatLon(const int &x, const int &y, 
                                  double &X, double &Y) 
{
  int sx = x;
  int sy = y;
  irotate(sx, sy, DisplayAngle);
  Y= PanLatitude  - sy*InvDrawScale;
  X= PanLongitude + sx*invfastcosine(Y)*InvDrawScale;
}


void MapWindow::Screen2LatLon(const int &x, const int &y, 
                              double &X, double &Y) 
{
  int sx = x-(int)Orig_Screen.x;
  int sy = y-(int)Orig_Screen.y;
  irotate(sx, sy, DisplayAngle);
  Y= PanLatitude  - sy*InvDrawScale;
  X= PanLongitude + sx*invfastcosine(Y)*InvDrawScale;
}

void MapWindow::LatLon2Screen(const double &lon, const double &lat, 
                              POINT &sc) {

  int Y = Real2Int((PanLatitude-lat)*DrawScale);
  int X = Real2Int((PanLongitude-lon)*fastcosine(lat)*DrawScale);
    
  irotate(X, Y, DisplayAngle);
    
  sc.x = Orig_Screen.x - X;
  sc.y = Orig_Screen.y + Y;
}

// This one is optimised for long polygons
void MapWindow::LatLon2Screen(pointObj *ptin, POINT *ptout, const int n,
			      const int skip) {
  static double lastangle = -1;
  static int cost=1024, sint=0;
  const double mDisplayAngle = DisplayAngle;

  if(mDisplayAngle != lastangle) {
    lastangle = mDisplayAngle;
    int deg = DEG_TO_INT(AngleLimit360(mDisplayAngle));
    cost = ICOSTABLE[deg];
    sint = ISINETABLE[deg];
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const double mDrawScale = DrawScale;
  const double mPanLongitude = PanLongitude;
  const double mPanLatitude = PanLatitude;
  pointObj* p = ptin;
  const pointObj* ptend = ptin+n;

  while (p<ptend) {
    int Y = Real2Int((mPanLatitude-p->y)*mDrawScale);
    int X = Real2Int((mPanLongitude-p->x)*fastcosine(p->y)*mDrawScale);
    ptout->x = (xxs-X*cost + Y*sint)/1024;
    ptout->y = (Y*cost + X*sint + yys)/1024;
    ptout++;
    p+= skip;
  }
}


////////////////////////////////////////////////////////////////////////



#define NUMPOINTS 2
void MapWindow::DrawSolidLine(const HDC& hdc, const POINT &ptStart, 
                              const POINT &ptEnd)
{
  POINT pt[2];
  
  pt[0]= ptStart;
  pt[0]= ptStart;
  pt[1]= ptEnd;
  pt[1]= ptEnd;
  Polyline(hdc, pt, NUMPOINTS);
} 

void _DrawLine(HDC hdc, int PenStyle, int width, 
               POINT ptStart, POINT ptEnd, COLORREF cr){

  HPEN hpDash,hpOld;
  POINT pt[2];
  //Create a dot pen
  hpDash = (HPEN)CreatePen(PenStyle, width, cr);
  hpOld = (HPEN)SelectObject(hdc, hpDash);

  pt[0].x = ptStart.x;
  pt[0].y = ptStart.y;
  pt[1].x = ptEnd.x;
  pt[1].y = ptEnd.y;

  Polyline(hdc, pt, NUMPOINTS);

  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDash);

}

void DrawDashLine(HDC hdc, INT width, 
                  POINT ptStart, POINT ptEnd, COLORREF cr)
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


void DrawDotLine(HDC hdc, POINT ptStart, POINT ptEnd, COLORREF cr)
{
  /*
  HPEN hpDot, hpOld;
  LOGPEN dashLogPen;
  POINT pt[2];
  //Create a dot pen
  dashLogPen.lopnColor = cr;
  dashLogPen.lopnStyle = PS_DOT;
  dashLogPen.lopnWidth.x = 0;
  dashLogPen.lopnWidth.y = 0;

  hpDot = (HPEN)CreatePenIndirect(&dashLogPen);
  hpOld = (HPEN)SelectObject(hdc, hpDot);

  pt[0].x = ptStart.x;
  pt[0].y = ptStart.y;
  pt[1].x = ptEnd.x;
  pt[1].y = ptEnd.y;
  
  Polyline(hdc, pt, NUMPOINTS);
  
  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDot);
  */
} 

