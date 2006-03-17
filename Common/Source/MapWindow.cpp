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

#include "Mapwindow.h"
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

#include "GaugeVarioAltA.h"
#include "GaugeCDI.h"
#include "GaugeFLARM.h"
#include "InfoBoxLayout.h"

#if (WINDOWSPC>0)
#include <Wingdi.h>
#endif

int TrailActive = TRUE;

///////////////////////////////// Initialisation

HBITMAP MapWindow::hBmpAirportReachable;
HBITMAP MapWindow::hBmpAirportUnReachable;
HBITMAP MapWindow::hBmpFieldReachable;
HBITMAP MapWindow::hBmpFieldUnReachable;

HPEN    MapWindow::hpCompassBorder;
HBRUSH  MapWindow::hBrushFlyingModeAbort;


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


RECT MapWindow::MapRect;
RECT MapWindow::MapRectBig;
RECT MapWindow::MapRectSmall;

HBITMAP MapWindow::hDrawBitMap = NULL;
HBITMAP MapWindow::hDrawBitMapBg = NULL;
HBITMAP MapWindow::hDrawBitMapTmp = NULL;
HDC MapWindow::hdcDrawWindow = NULL;
HDC MapWindow::hdcDrawWindowBg = NULL;
HDC MapWindow::hdcScreen = NULL;
HDC MapWindow::hDCTemp = NULL;

rectObj MapWindow::screenbounds_latlon;

double MapWindow::PanX=0.0;
double MapWindow::PanY=0.0;
double MapWindow::PanXr=0.0;
double MapWindow::PanYr=0.0;

bool MapWindow::EnablePan = FALSE;

BOOL MapWindow::CLOSETHREAD = FALSE;
BOOL MapWindow::THREADRUNNING = TRUE;
BOOL MapWindow::THREADEXIT = FALSE;

bool MapWindow::BigZoom = true;

DWORD  MapWindow::dwDrawThreadID;
HANDLE MapWindow::hDrawThread;

double MapWindow::RequestMapScale = 5;
double MapWindow::MapScale = 5;
double MapWindow::MapScaleOverDistanceModify = 5/DISTANCEMODIFY;
double MapWindow::ResMapScaleOverDistanceModify = 0.0;
double MapWindow::DisplayAngle = 0.0;
double MapWindow::DisplayAircraftAngle = 0.0;
double MapWindow::DrawScale;

bool MapWindow::AutoZoom = false;

int MapWindow::dTDisplay=0;

HBITMAP MapWindow::hLandable;
HBITMAP MapWindow::hReachable;
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

  // 12 is number of airspace types
int	 MapWindow::iAirspaceBrush[AIRSPACECLASSCOUNT];
int	 MapWindow::iAirspaceColour[AIRSPACECLASSCOUNT];
int      MapWindow::iAirspaceMode[AIRSPACECLASSCOUNT];

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
HPEN MapWindow::hpMapScale;
HPEN MapWindow::hpTerrainLine;
HPEN MapWindow::hpTerrainLineBg;
HPEN MapWindow::hpSpeedSlow;
HPEN MapWindow::hpSpeedFast;
  
COLORREF MapWindow::BackgroundColor = RGB(0xFF,0xFF,0xFF); //sjt 02NOV05 - was F5F5F5. Changed to increase screen clarity at oblique viewing angles.

bool MapWindow::RequestFastRefresh = false;
bool MapWindow::MapDirty = true;
bool MapWindow::RequestMapDirty = false;
bool MapWindow::AirDataDirty = true;
bool MapWindow::RequestAirDataDirty = false;
DWORD MapWindow::fpsTime0 = 0;
bool MapWindow::MapFullScreen = false;
bool MapWindow::RequestFullScreen = false;

/////////////////////////////////

extern int DisplayTimeOut;

NMEA_INFO DrawInfo;
DERIVED_INFO DerivedDrawInfo;

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

void debugUpdateStats(bool start) {
  static long tottime=0;
  static DWORD timelast = 0;
  if (start) {
    timelast = ::GetTickCount();
  } else {
    DWORD time = ::GetTickCount();
    tottime = (4*tottime+(time-timelast))/5;
    timestats_av = tottime;
  }
}


///////////////////


bool MapWindow::Event_NearestWaypointDetails(double lon, double lat, 
					     double range,
					     bool pan) {
  int i;
  if (!pan || !EnablePan) {
    i=FindNearestWayPoint(lon, lat, range);
  } else {
    // nearest to center of screen if in pan mode
    i=FindNearestWayPoint(PanXr, PanYr, range);
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
  int i;

  i= FindAirspaceCircle(lon,lat);
  if(i != -1)
    {
      DisplayAirspaceWarning(AirspaceCircle[i].Type , 
			     AirspaceCircle[i].Name , 
			     AirspaceCircle[i].Base, 
			     AirspaceCircle[i].Top );
      return true;
    }
  i= FindAirspaceArea(lon,lat);
  if(i != -1)
    {
      DisplayAirspaceWarning(AirspaceArea[i].Type , 
			     AirspaceArea[i].Name , 
			     AirspaceArea[i].Base, 
			     AirspaceArea[i].Top );
      return true;
    }

  return false; // nothing found..
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

void MapWindow::TextInBox(HDC hDC, TCHAR* Value, int x, int y, int size, TextInBoxMode_t Mode, bool noOverlap) {

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
  }

  if (Mode.AsFlag.AlligneCenter){
    x -= tsize.cx/2;
    y -= tsize.cy/2;
  }

  bool notoverlapping = true;

  if (Mode.AsFlag.Border){

    POINT offset;

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4) / 8)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4) / 8);

    if (Mode.AsFlag.AlligneRight)
      x -= 3;

    if (TextInBoxMoveInView(&offset, &brect)){
      x += offset.x;
      y += offset.y;
    }

    notoverlapping = checkLabelBlock(brect);
  
    if (!noOverlap || notoverlapping) {
      HPEN oldPen = (HPEN)SelectObject(hDC, hpMapScale);
      RoundRect(hDC, brect.left, brect.top, brect.right, brect.bottom, 8, 8);
      SelectObject(hDC, oldPen);

      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
    }


  } else if (Mode.AsFlag.FillBackground){

    POINT offset;

    brect.left = x-1;
    brect.right = brect.left+tsize.cx+1;  
    brect.top = y+((tsize.cy+4) / 8);
    brect.bottom = brect.top+tsize.cy-((tsize.cy+4) / 8);

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

  } else {

//    POINT offset;

    /*
    if (TextInBoxMoveInView(&offset, &brect)){
      x += offset.x;
      y += offset.y;
    }
    */

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4) / 8)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4) / 8);

    notoverlapping = checkLabelBlock(brect);
  
    if (!noOverlap || notoverlapping) {
      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
    }

  }
  
  SelectObject(hDC, oldFont);
  SelectObject(hDC, hbOld);

}

bool userasked = false;

void MapWindow::RefreshMap() {
  RequestMapDirty = true;
  userasked = true;
}

bool MapWindow::IsMapFullScreen() {
	// SDP - Seems that RequestFullScreen is always more accurate (MapFullSCreen is delayed)
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
      StoreRestoreFullscreen(false);
      PanX = 0.0;
      PanY = 0.0;
    }
  }
  RefreshMap();
}


void MapWindow::Event_PanCursor(int dx, int dy) {
  double X= (MapRect.right+MapRect.left)/2;
  double Y= (MapRect.bottom+MapRect.top)/2;
  double Xstart=X;
  double Ystart=Y;

  GetLocationFromScreen(&Xstart, &Ystart);

  X+= (MapRect.right-MapRect.left)/4.0*dx;
  Y+= (MapRect.bottom-MapRect.top)/4.0*dy;
  GetLocationFromScreen(&X, &Y);

  if (EnablePan) {
    PanX += Xstart-X;
    PanY += Ystart-Y;
  }
  RefreshMap();
}

bool MapWindow::isPan() {
	return EnablePan;
}

/* Event_TerrainToplogy Changes
	0	Show
	1	Toplogy = ON
	2	Toplogy = OFF
	3	Terrain = ON
	4	Terrain = OFF
	-1	Toggle through 4 stages (off/off, off/on, on/off, on/on)
	-2	Toggle terrain
	-3	Toggle toplogy
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
  static bool oldfullscreen = 0;
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

  if (!EnablePan) {
    PanX = 0.0;
    PanY = 0.0;
  }
  RefreshMap();
}


void MapWindow::Event_SetZoom(double value) {

  static double lastRequestMapScale = RequestMapScale;

  if (ScaleListCount > 0){
    RequestMapScale = FindMapScale(value);
  } else {
    RequestMapScale = max(0.1,min(160.0, value));
  }
  if (lastRequestMapScale != RequestMapScale){
    lastRequestMapScale = RequestMapScale;
    BigZoom = true;
    RefreshMap();
  }
}


void MapWindow::Event_ScaleZoom(int vswitch) {

  static double lastRequestMapScale = RequestMapScale;
  double value = RequestMapScale;

  if (ScaleListCount > 0){
    value = StepMapScale(-vswitch);
  } else {

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

    RequestMapScale = max(0.1,min(160.0, value));
  }

  if (lastRequestMapScale != RequestMapScale){
    lastRequestMapScale = RequestMapScale;
    BigZoom = true;
    RefreshMap();

    DrawMapScale(hdcScreen, MapRect, true);
  }
}


int MapWindow::GetMapResolutionFactor(void) {
  return 30*InfoBoxLayout::scale;
}

double MapWindow::StepMapScale(int Step){
  ScaleCurrent += Step;
  ScaleCurrent = max(0,min(ScaleListCount-1, ScaleCurrent));
  return((ScaleList[ScaleCurrent]*GetMapResolutionFactor())
	 /(Appearance.DefaultMapWidth*InfoBoxLayout::scale));
}

double MapWindow::FindMapScale(double Value){

  int    i;
  double BestFit = 99999;
  int    BestFitIdx=-1;
  double DesiredScale = 
    (Value 
     *Appearance.DefaultMapWidth*InfoBoxLayout::scale) / 
    GetMapResolutionFactor();

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
	   /Appearance.DefaultMapWidth*InfoBoxLayout::scale);
  }
  return(Value);
}

///////////////////////////////////////////////////////////////////////////


static void SetFontInfo(HDC hDC, FontHeightInfo_t *FontHeightInfo){
  TEXTMETRIC tm;
  int x,y;
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

  if (FontHeightInfo->CapitalHeight<y)
    FontHeightInfo->CapitalHeight = bottom - top + 1;

  if (FontHeightInfo->CapitalHeight <= 0)
    FontHeightInfo->CapitalHeight = tm.tmAscent - 1 -(tm.tmHeight/10);

}

extern bool ProgramStarted;

LRESULT CALLBACK MapWindow::MapWndProc (HWND hWnd, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam)
{
  int i;
  //  TCHAR szMessageBuffer[1024];
  double X,Y;
  static double Xstart, Ystart;
  static double XstartScreen, YstartScreen;
  double distance;
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
    hdcDrawWindowBg = CreateCompatibleDC(hdcScreen);
    hDCTemp = CreateCompatibleDC(hdcDrawWindow);

    hBackgroundBrush = CreateSolidBrush(BackgroundColor);

    hFLARMTraffic=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FLARMTRAFFIC));
    hTerrainWarning=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TERRAINWARNING));
    hLandable=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LANDABLE));
    hReachable=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REACHABLE));
    hTurnPoint=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TURNPOINT));
    hSmall=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SMALL));
    hAutoMacCready=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AUTOMCREADY));
    hGPSStatus1=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GPSSTATUS1));
    hGPSStatus2=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GPSSTATUS2));
    hLogger=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGGER));
    hLoggerOff=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGGEROFF));

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

    for (i=0; i<NUMAIRSPACEBRUSHES; i++) {
      hAirspaceBrushes[i] =
        CreatePatternBrush((HBITMAP)hAirspaceBitmap[i]);
    }
    for (i=0; i<AIRSPACECLASSCOUNT; i++) {
      hAirspacePens[i] =
        CreatePen(PS_SOLID, 2 /*todo*/, Colours[iAirspaceColour[i]]);
    }


    /* JMW created all re-used pens here */

    hpAircraft = (HPEN)CreatePen(PS_SOLID, 3*InfoBoxLayout::scale, 
				 RGB(0xf0,0xf0,0xf0));
    hpAircraftBorder = (HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, 
				       RGB(0x00,0x00,0x00));
    #if (MONOCHROME_SCREEN > 0)
    hpWind = (HPEN)CreatePen(PS_SOLID, 2*InfoBoxLayout::scale, RGB(0,0,0));
    #else
    hpWind = (HPEN)CreatePen(PS_SOLID, 2*InfoBoxLayout::scale, RGB(255,0,0));
    #endif

    hpWindThick = (HPEN)CreatePen(PS_SOLID, 4*InfoBoxLayout::scale, 
				  RGB(255,220,220));

    hpBearing = (HPEN)CreatePen(PS_SOLID, 2*InfoBoxLayout::scale, 
				RGB(0,0,0));
    hpBestCruiseTrack = (HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, 
					RGB(0,0,255));
    #if (MONOCHROME_SCREEN > 0)
    hpCompass = (HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, RGB(0x00,0x00,0x00));
    //hpCompass = (HPEN)CreatePen(PS_SOLID, 1, RGB(0xff,0xff,0xff));
    #else
    hpCompass = (HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, RGB(0xcf,0xcf,0xFF));
    #endif
    hpThermalBand = (HPEN)CreatePen(PS_SOLID, 2*InfoBoxLayout::scale, RGB(0x40,0x40,0xFF));
    hpThermalBandGlider = (HPEN)CreatePen(PS_SOLID, 2*InfoBoxLayout::scale, RGB(0x00,0x00,0x30));

    hpFinalGlideBelow = (HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, RGB(0xFF,0xA0,0xA0));
    hpFinalGlideAbove = (HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, RGB(0xA0,0xFF,0xA0));

    hpSpeedSlow=(HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, 
				RGB(0xFF,0x00,0x00));
    hpSpeedFast=(HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, 
				RGB(0x00,0xFF,0x00));

    hpMapScale = (HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, 
				 RGB(0,0,0));
    hpTerrainLine = (HPEN)CreatePen(PS_DASH, 1*InfoBoxLayout::scale, 
				    RGB(0x30,0x30,0x30));
    hpTerrainLineBg = (HPEN)CreatePen(PS_SOLID, 1*InfoBoxLayout::scale, 
				      RGB(0xFF,0xFF,0xFF));

    #if (MONOCHROME_SCREEN > 0)
    hbCompass=(HBRUSH)CreateSolidBrush(RGB(0xff,0xff,0xff));
    #else
    hbCompass=(HBRUSH)CreateSolidBrush(RGB(0x40,0x40,0xFF));
    #endif
    hbThermalBand=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0xFF));
    hbBestCruiseTrack=(HBRUSH)CreateSolidBrush(RGB(0x0,0x0,0xFF));
    hbFinalGlideBelow=(HBRUSH)CreateSolidBrush(RGB(0xFF,0x00,0x00));
    hbFinalGlideAbove=(HBRUSH)CreateSolidBrush(RGB(0x00,0xFF,0x00));


#if (MONOCHROME_SCREEN > 0)
    hbWind=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0x80));
#else
    hbWind=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0x80));
#endif

    ScaleListCount = propGetScaleList(ScaleList, sizeof(ScaleList)/sizeof(ScaleList[0]));
    RequestMapScale = FindMapScale(RequestMapScale);

    hBmpMapScale = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_MAPSCALE_A));

    hpCompassBorder = (HPEN)CreatePen(PS_SOLID, 3*InfoBoxLayout::scale, 
				      RGB(0xff,0xff,0xff));
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

    //JMWFOO


    break;

  case WM_DESTROY:

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
    DeleteObject((HPEN)hpMapScale);
    DeleteObject((HPEN)hpTerrainLine);
    DeleteObject((HPEN)hpTerrainLineBg);
    DeleteObject((HPEN)hpSpeedFast);
    DeleteObject((HPEN)hpSpeedSlow);
    
    DeleteObject((HBRUSH)hbCompass);
    DeleteObject((HBRUSH)hbThermalBand);
    DeleteObject((HBRUSH)hbBestCruiseTrack);
    DeleteObject((HBRUSH)hbFinalGlideBelow);
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

    for(i=0;i<NUMAIRSPACEBRUSHES;i++)
    {
      DeleteObject(hAirspaceBrushes[i]);
      DeleteObject(hAirspaceBitmap[i]);
    }

    for (i=0; i<AIRSPACECLASSCOUNT; i++) {
      DeleteObject(hAirspacePens[i]);
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
      DefocusInfoBox();
      SetFocus(hWnd);
      break;
    }
    dwUpTime = GetTickCount(); dwDownTime = dwUpTime - dwDownTime;
    
    distance = isqrt4((long)((XstartScreen-X)*(XstartScreen-X)+
			     (YstartScreen-Y)*(YstartScreen-Y)))
      /InfoBoxLayout::scale;
    
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
        GPS_INFO.Speed = min(100.0,distance/3);
        break;
      }
#endif
    }
    
    if(dwDownTime < 1000)
    {
      if (Event_NearestWaypointDetails(Xstart, Ystart, 500*MapScale, false)) {
	break;
      }
    }
    else
    {
      if (Event_InteriorAirspaceDetails(Xstart, Ystart)) {
	break;
      }
    }
    break;
    
#if NEWINFOBOX > 0
  case WM_KEYDOWN:
    void WindowControlTest(void);
    if (wParam == 191){
      WindowControlTest();
    }
  break;
#endif
  case WM_KEYUP:
    if (!DialogActive) { // JMW prevent keys being trapped if dialog is active
      if (InputEvents::processKey(wParam)) {
	// TODO - change to debugging DoStatusMessage(TEXT("Event in default"));
      }
      // XXX Should we only do this if it IS processed above ?
      return TRUE; // don't go to default handler
    } else {
      // TODO - debugging DoStatusMessage(TEXT("Event in dialog"));
      if (InputEvents::processKey(wParam)) {
      }
      return TRUE; // don't go to default handler
    }
    break;
  }

  return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}

extern int FrameCount;


void MapWindow::ModifyMapScale(void) {
  // limit zoomed in so doesn't reach silly levels
  if (MapScale<0.1) {
    MapScale = 0.1; 
  }
  MapScaleOverDistanceModify = MapScale/DISTANCEMODIFY;
  ResMapScaleOverDistanceModify = 
    GetMapResolutionFactor()/MapScaleOverDistanceModify;
  RequestMapScale = MapScale;
  DrawScale = MapScaleOverDistanceModify;
  DrawScale = DrawScale/111000;
  DrawScale = GetMapResolutionFactor()/DrawScale;
}


void MapWindow::UpdateMapScale()
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
    ModifyMapScale();
    useraskedforchange = true;
  }
  
  if (AutoZoom) {
    if(DerivedDrawInfo.WaypointDistance > 0)
    {
      
      if(
        (DisplayOrientation == NORTHUP) 
        || 
        (((DisplayOrientation == NORTHCIRCLE) 
	  || (DisplayOrientation == TRACKCIRCLE)) 
	 && (DerivedDrawInfo.Circling == TRUE) )
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
          < ( AutoZoomFactor * MapScaleOverDistanceModify))
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
        MapScale = DerivedDrawInfo.WaypointDistance*DISTANCEMODIFY 
	  / AutoZoomFactor;
	ModifyMapScale();

      }	else {

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


void MapWindow::CalculateOrigin(RECT rc, POINT *Orig)
{
  bool GliderCenter=false;
  double trackbearing = DrawInfo.TrackBearing;

  //  trackbearing = DerivedDrawInfo.NextTrackBearing;
  
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
      DisplayAircraftAngle = trackbearing-DisplayAngle;
    } else {
      DisplayAngle = 0.0;
      DisplayAircraftAngle = trackbearing;
    }
    
  } else {
    // normal, glider forward
    GliderCenter = FALSE;
    DisplayAngle = trackbearing;
    DisplayAircraftAngle = 0.0;
    
  }
  
  if(GliderCenter || EnablePan) {
    Orig->x = iround((rc.left + rc.right ) /2.0);
    Orig->y = iround((rc.bottom - rc.top) /2.0+rc.top);
  }
  else
  {
    Orig->x = iround((rc.left + rc.right ) /2.0);
    Orig->y = iround((rc.bottom - rc.top) - ((rc.bottom - rc.top )/5.0)+rc.top);
  }
}


bool MapWindow::RenderTimeAvailable() {
  DWORD	fpsTime = ::GetTickCount();
  if (fpsTime-fpsTime0<800) {
    return true;
  } else {
    return false;
  }
}


void MapWindow::RenderMapWindow(  RECT rc)
{
  bool drawmap;
  HFONT hfOld;
  
  DWORD	fpsTime = ::GetTickCount();

  UpdateMapScale();

  // only redraw map part every 800 s unless triggered
  if (((fpsTime-fpsTime0)>800)||(fpsTime0== -1)||(userasked)) {
    fpsTime0 = fpsTime;
    drawmap = true;
    userasked = false;
  }
  
  POINT Orig, Orig_Aircraft;
  
  CalculateOrigin(rc, &Orig);
  
  CalculateScreenPositions(Orig, rc, &Orig_Aircraft);

  if (drawmap) {

    // do slow calculations before clearing the screen
    // to reduce flicker
    CalculateWaypointReachable();
    CalculateScreenPositionsAirspace(Orig, rc, &Orig_Aircraft);    
    
    // display border and fill background..
    
    if(InfoWindowActive) {
      SelectObject(hdcDrawWindowBg, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindowBg, GetStockObject(BLACK_PEN));
    }
    else {
      // JMW added light grey background
      SelectObject(hdcDrawWindowBg, hBackgroundBrush);
      SelectObject(hdcDrawWindowBg, GetStockObject(WHITE_PEN));
    }

    Rectangle(hdcDrawWindowBg,rc.left,rc.top,rc.right,rc.bottom);

    SelectObject(hdcDrawWindowBg, GetStockObject(BLACK_BRUSH));
    SelectObject(hdcDrawWindowBg, GetStockObject(BLACK_PEN));
    hfOld = (HFONT)SelectObject(hdcDrawWindowBg, MapWindowFont);

    // ground first...

    if (EnableTerrain) {
      double sunelevation = 40.0;
      double sunazimuth = -DerivedDrawInfo.WindBearing;

      // BUG?   sunazimuth+= DisplayAngle;
      DrawTerrain(hdcDrawWindowBg, rc, sunazimuth, sunelevation);
    }

    if (EnableTopology) {
      DrawTopology(hdcDrawWindowBg, rc);
    }
    
    // reset label over-write preventer
    nLabelBlocks = 0;

    if (!TaskAborted) {
      DrawTaskAAT(hdcDrawWindowBg, rc);
    }

    // then airspace..
    DrawAirSpace(hdcDrawWindowBg, rc);

    if(TrailActive)
      DrawTrail(hdcDrawWindowBg, Orig_Aircraft, rc);

    if (TaskAborted) {
      DrawAbortedTask(hdcDrawWindowBg, rc, Orig_Aircraft);
    } else {
      DrawTask(hdcDrawWindowBg, rc);
    }

    if (FinalGlideTerrain && DerivedDrawInfo.TerrainValid) {
      DrawGlideThroughTerrain(hdcDrawWindowBg, rc);
    }

    DrawWaypoints(hdcDrawWindowBg,rc);

    // draw wind vector at aircraft
    #if (ALTERNATEWINDVECTOR == 0)
    if (!DerivedDrawInfo.Circling && (!EnablePan)) {
      DrawWindAtAircraft(hdcDrawWindowBg, Orig, rc);
    }
    #endif


    #if (ALTERNATEWINDVECTOR == 1)
    DrawWindAtAircraft2(hdcDrawWindowBg, Orig, rc);
    #endif

    DrawFLARMTraffic(hdcDrawWindowBg, rc);

    DrawBestCruiseTrack(hdcDrawWindowBg, Orig_Aircraft);

    DrawBearing(hdcDrawWindowBg, Orig_Aircraft);

    // finally, draw you!

    if (EnablePan) {
      DrawCrossHairs(hdcDrawWindowBg, Orig);
    }

    if (extGPSCONNECT) {
      DrawAircraft(hdcDrawWindowBg, Orig_Aircraft);
    }

    // marks on top...
    DrawMarks(hdcDrawWindowBg, rc);
  }

  BitBlt(hdcDrawWindow, 0, 0, MapRectBig.right, MapRectBig.bottom,
    hdcDrawWindowBg, 0, 0, SRCCOPY);

  // overlays
  DrawCDI();

  hfOld = (HFONT)SelectObject(hdcDrawWindow, MapWindowFont);

  DrawMapScale(hdcDrawWindow,rc, false);
  DrawMapScale2(hdcDrawWindow,rc, Orig_Aircraft);

  DrawCompass(hdcDrawWindow, rc);

  #if (ALTERNATEWINDVECTOR == 0)
  if (DerivedDrawInfo.Circling || EnablePan) {
    DrawWind(hdcDrawWindow, Orig, rc); // JMW shouldn't need Orig here
  }
  #endif

  DrawFlightMode(hdcDrawWindow, rc);

  DrawFinalGlide(hdcDrawWindow,rc);

  DrawSpeedToFly(hdcDrawWindow, rc);

  DrawThermalBand(hdcDrawWindow, rc);

  DrawGPSStatus(hdcDrawWindow, rc);

  SelectObject(hdcDrawWindow, hfOld);

}

void MapWindow::UpdateInfo(NMEA_INFO *nmea_info,
			   DERIVED_INFO *derived_info) {
  memcpy(&DrawInfo,nmea_info,sizeof(NMEA_INFO));
  memcpy(&DerivedDrawInfo,derived_info,sizeof(DERIVED_INFO));
}


DWORD MapWindow::DrawThread (LPVOID lpvoid)
{
  MapScale = RequestMapScale;
  ModifyMapScale();
  
  //  THREADRUNNING = FALSE;
  THREADEXIT = FALSE;

  nLabelBlocks = 0;

  GetClientRect(hWndMapWindow, &MapRectBig);

  debugUpdateStats(true);
  
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
  UpdateInfo(&GPS_INFO, &CALCULATED_INFO);

  RenderMapWindow(MapRect);
  LockTerrainDataGraphics();
  SetTopologyBounds(MapRect);
  UnlockTerrainDataGraphics();
  
  //////

  static int nodrawtimeout = 10; // force redraw if haven't redrawn
				 // for 5 seconds
  while (!CLOSETHREAD) 
  {
    if (!THREADRUNNING) {
      Sleep(100);
      continue;
    }

    // force refresh of terrain, topo and screen update if it hasn't
    // been redrawn for 5 seconds
    if (nodrawtimeout<=0) {
      MapDirty = true;
      // should this be RequestFastRefresh?
      nodrawtimeout = 50;
      fpsTime0 = -1;
    }

    if (!MapDirty && !RequestFastRefresh) { 
      Sleep(100);
      nodrawtimeout--;
      continue;
    } 
    //    debugUpdateStats(true);

    nodrawtimeout= 50;

    // draw previous frame so screen is immediately refreshed
    if (RequestFastRefresh  && (!MapDirty)) {
      BitBlt(hdcScreen, 0, 0, MapRectBig.right-MapRectBig.left,
	     MapRectBig.bottom-MapRectBig.top, 
	     hdcDrawWindow, 0, 0, SRCCOPY);
    }
    
    if (RequestFullScreen != MapFullScreen) {
      ToggleFullScreenStart();
    }

    if (MapDirty) {
      MapDirty = false;
      // if map is dirty, there's no need for a fast refresh anyway
      RequestFastRefresh = false;
      //      fpsTime0 = -1;
    } else {
      if (RequestFastRefresh) {
        RequestFastRefresh = false;
	continue;
      }
    }

    /* JMW moved to calculations.cpp
    LockTerrainDataCalculations();
    terrain_dem_calculations.SetCacheTime();
    UnlockTerrainDataCalculations();
    */

    GaugeFLARM::Render(&DrawInfo);

    RenderMapWindow(MapRect);
    
    BitBlt(hdcScreen, 0, 0, MapRectBig.right-MapRectBig.left,
	     MapRectBig.bottom-MapRectBig.top, 
       hdcDrawWindow, 0, 0, SRCCOPY);
    
    FrameCount ++;
    
    // we do caching after screen update, to minimise perceived delay

    // have some time, do shape file cache update if necessary
    if (EnableTopology) {
      LockTerrainDataGraphics();
      SetTopologyBounds(MapRect);
      UnlockTerrainDataGraphics();
    }

    // have some time, do graphics terrain cache update if necessary
    if (EnableTerrain) {
      if (RenderTimeAvailable()) {
        OptimizeTerrainCache();
      }
    }

    // have some time, do calculations terrain cache update if necessary
    if (RenderTimeAvailable()) {

    }

    debugUpdateStats(false);

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

    double dX,dY;
    int i;
    HPEN hpOld;
    HBRUSH hbAircraftSolid = (HBRUSH) CreateSolidBrush(RGB(0x00,0x00,0x00));
    HBRUSH hbAircraftSolidBg = (HBRUSH) CreateSolidBrush(RGB(0xff,0xff,0xff));
    HBRUSH hbOld = (HBRUSH)SelectObject(hdc, hbAircraftSolidBg);
    hpOld = (HPEN)SelectObject(hdc, hpAircraft);
  
    for(i=0; i<NUMAIRCRAFTPOINTS; i++)
    {

  // JMW now corrects displayed aircraft heading for wind

      dX = (double)Aircraft[i].x;
      dY = (double)Aircraft[i].y;
      rotate(&dX, &dY, DisplayAircraftAngle+
             (DerivedDrawInfo.Heading-DrawInfo.TrackBearing) 
             );
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif

      Aircraft[i].x =iround(dX+Orig.x)+1;  Aircraft[i].y = iround(dY+Orig.y)+1;
    }

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

    int i;
    double dX,dY;
    HPEN oldPen;
    POINT Aircraft[] = {
      {0, -5},
      {1, -5},
      {1, 0},
      {14, 0},
      {14, 1},
      {1, 1},
      {1, 8},
      {3, 8},
      {3, 9},
      {-2, 9},
      {-2, 8},
      {0, 8},
      {0, 1},
      {-13, 1},
      {-13, 0},
      {0 ,0},
      {0, -5}
    };

    for(i=0; i<(sizeof(Aircraft)/sizeof(Aircraft[0])); i++){
      dX = (double)Aircraft[i].x-2 ;dY = (double)Aircraft[i].y;
      rotate(&dX, &dY, DisplayAircraftAngle+
             (DerivedDrawInfo.Heading-DrawInfo.TrackBearing)
             );
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif

      Aircraft[i].x =iround(dX+Orig.x);  Aircraft[i].y = iround(dY+Orig.y);
    }

    oldPen = (HPEN)SelectObject(hdc, hpCompassBorder);

    Polygon(hdc, Aircraft, (sizeof(Aircraft)/sizeof(Aircraft[0])));

    SelectObject(hdc, GetStockObject(BLACK_PEN));

    Polygon(hdc, Aircraft, (sizeof(Aircraft)/sizeof(Aircraft[0])));

    SelectObject(hdc, oldPen);

  }

}

void MapWindow::DrawBitmapX(HDC hdc, int x, int y,
			    int sizex, int sizey,
			    HDC source,
			    int offsetx, int offsety,
			    DWORD mode) {
#if (WINDOWSPC>0) 
  StretchBlt(hdc, x, y, 
	     sizex*InfoBoxLayout::scale, 
	     sizey*InfoBoxLayout::scale, 
	     source,
	     offsetx, offsety, sizex, sizey,
	     mode);
#else
  BitBlt(hdc, x, y, sizex, sizey, 
	 source, offsetx, offsety, mode); 
#endif

}

void MapWindow::DrawBitmapIn(HDC hdc, int x, int y, HBITMAP h) {
  SelectObject(hDCTemp, h);

  DrawBitmapX(hdc,
	      x-5*InfoBoxLayout::scale,
	      y-5*InfoBoxLayout::scale,
	      10,10,
    hDCTemp,0,0,SRCPAINT);
  DrawBitmapX(hdc,
	      x-5*InfoBoxLayout::scale,
	      y-5*InfoBoxLayout::scale,
	      10,10,
    hDCTemp,10,0,SRCAND);
}



void MapWindow::DrawGPSStatus(HDC hDC, RECT rc)
{

  if (extGPSCONNECT && !(DrawInfo.NAVWarning) && (DrawInfo.SatellitesUsed>0)) 
    // nothing to do
    return;

  TCHAR gpswarningtext1[] = TEXT("GPS not connected");
  TCHAR gpswarningtext2[] = TEXT("GPS waiting for fix");
  TextInBoxMode_t TextInBoxMode = {2};

  if (!extGPSCONNECT) {
    SelectObject(hDCTemp,hGPSStatus2);
    DrawBitmapX(hDC, 
		rc.left+2*InfoBoxLayout::scale,
		rc.bottom+(Appearance.GPSStatusOffset.y-22)
		*InfoBoxLayout::scale,
		20, 20,
		hDCTemp, 
		0, 0, SRCAND);

    TextInBox(hDC, gettext(gpswarningtext1), 
	      rc.left+24*InfoBoxLayout::scale, 
	      rc.bottom+(Appearance.GPSStatusOffset.y-19)
	      *InfoBoxLayout::scale, 
	      0, TextInBoxMode);

  } else
    if (DrawInfo.NAVWarning || (DrawInfo.SatellitesUsed==0)) {
      SelectObject(hDCTemp,hGPSStatus1);

      DrawBitmapX(hDC, 
		  rc.left+2*InfoBoxLayout::scale,
		  rc.bottom+(Appearance.GPSStatusOffset.y-22)
		  *InfoBoxLayout::scale,
		  20, 20,
		  hDCTemp, 
		  0, 0, SRCAND);

      TextInBox(hDC, gettext(gpswarningtext2), 
		rc.left+24*InfoBoxLayout::scale, 
		rc.bottom+
		(Appearance.GPSStatusOffset.y-19)*InfoBoxLayout::scale,
		0, TextInBoxMode);

    }

}

void MapWindow::DrawFlightMode(HDC hdc, RECT rc)
{
  static bool flip= true;
  static double LastTime = 0;
  bool drawlogger = true;
  static bool lastLoggerActive=false;
  int offset = -3;

  if (!Appearance.DontShowLoggerIndicator){

    // has GPS time advanced?
    if(DrawInfo.Time <= LastTime)
      {
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
		  rc.right+(offset-3+Appearance.FlightModeOffset.x)
		  *InfoBoxLayout::scale,
		  rc.bottom+(-7-3+Appearance.FlightModeOffset.y)
		  *InfoBoxLayout::scale,
		  7,7,
		  hDCTemp,
		  0,0,SRCPAINT);

      DrawBitmapX(hdc,
		  rc.right+(offset-3+Appearance.FlightModeOffset.x)
		  *InfoBoxLayout::scale,
		  rc.bottom+(-7-3+Appearance.FlightModeOffset.y)
		  *InfoBoxLayout::scale,
		  7,7,
		  hDCTemp,
		  7,0,SRCAND);
    }
  }

  if (Appearance.FlightModeIcon == apFlightModeIconDefault){

    if (TaskAborted) {
      SelectObject(hDCTemp,hAbort);
    } else {
      if (DerivedDrawInfo.Circling) {
        SelectObject(hDCTemp,hClimb);
      } else if (DerivedDrawInfo.FinalGlide) {
        SelectObject(hDCTemp,hFinalGlide);
      } else {
        SelectObject(hDCTemp,hCruise);
      }
    }
    // Code already commented as of 12aug05 - redundant? -st
    //		BitBlt(hdc,rc.right-35,5,24,20,
    //				 hDCTemp,20,0,SRCAND);

    // code for pre 12aug icons - st
    //BitBlt(hdc,rc.right-24-3,rc.bottom-20-3,24,20,
    //  hDCTemp,0,0,SRCAND);

    offset -= 24;

      DrawBitmapX(hdc,
		  rc.right+(offset-3+Appearance.FlightModeOffset.x)
		  *InfoBoxLayout::scale,
		  rc.bottom+(-20-3+Appearance.FlightModeOffset.y)
		  *InfoBoxLayout::scale,
		  24,20,
		  hDCTemp,
		  0,0,SRCPAINT);

      DrawBitmapX(hdc,
		  rc.right+(offset-3+Appearance.FlightModeOffset.x)
		  *InfoBoxLayout::scale,
		  rc.bottom+(-20-3+Appearance.FlightModeOffset.y)
		  *InfoBoxLayout::scale,
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

    if (DerivedDrawInfo.Circling) {

      SetPoint(0, 
	       Center.x, 
	       Center.y-4*InfoBoxLayout::scale);
      SetPoint(1, 
	       Center.x-8*InfoBoxLayout::scale, 
	       Center.y+4*InfoBoxLayout::scale);
      SetPoint(2, 
	       Center.x+8*InfoBoxLayout::scale, 
	       Center.y+4*InfoBoxLayout::scale);

    } else if (DerivedDrawInfo.FinalGlide) {

      SetPoint(0, 
	       Center.x, 
	       Center.y+4*InfoBoxLayout::scale);
      SetPoint(1, 
	       Center.x-8*InfoBoxLayout::scale, 
	       Center.y-4*InfoBoxLayout::scale);
      SetPoint(2, 
	       Center.x+8*InfoBoxLayout::scale, 
	       Center.y-4*InfoBoxLayout::scale);
    } else {

      SetPoint(0, 
	       Center.x+4*InfoBoxLayout::scale, 
	       Center.y);
      SetPoint(1, 
	       Center.x-4*InfoBoxLayout::scale, 
	       Center.y+8*InfoBoxLayout::scale);
      SetPoint(2, 
	       Center.x-4*InfoBoxLayout::scale, 
	       Center.y-8*InfoBoxLayout::scale);

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
		  rc.right+(offset-3+Appearance.FlightModeOffset.x)
		  *InfoBoxLayout::scale,
		  rc.bottom+(-20-3+Appearance.FlightModeOffset.y)
		  *InfoBoxLayout::scale,
		  24,20,
		  hDCTemp,
		  0,0,SRCPAINT);

      DrawBitmapX(hdc,
		  rc.right+(offset-3+Appearance.FlightModeOffset.x)
		  *InfoBoxLayout::scale,
		  rc.bottom+(-20-3+Appearance.FlightModeOffset.y)
		  *InfoBoxLayout::scale,
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

bool WaypointInTask(int ind) {
  int i;
  if (!WayPointList) return false;

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

static void MapWaypointLabelAdd(TCHAR *Name, int X, int Y, TextInBoxMode_t Mode, int AltArivalAGL, bool inTask=false);
static int _cdecl MapWaypointLabelListCompare(const void *elem1, const void *elem2 );
static MapWaypointLabel_t MapWaypointLabelList[50];
static int MapWaypointLabelListCount=0;


void MapWindow::DrawWaypoints(HDC hdc, RECT rc)
{
  unsigned int i;
  TCHAR Buffer[32];
  TCHAR Buffer2[32];
  TCHAR sAltUnit[4];
  TextInBoxMode_t DisplayMode;

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

      bool irange = false;
      bool intask = false;

      intask = WaypointInTask(i);

      DisplayMode.AsInt = 0;
      if(MapScale > 20)
      {
        SelectObject(hDCTemp,hSmall);
      }
      else if( ((WayPointList[i].Flags & AIRPORT) == AIRPORT) 
	       || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) )
      {
        if(WayPointList[i].Reachable){

          DisplayMode.AsFlag.Border = 1;
          DisplayMode.AsFlag.Reachable = 1;

          if ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
            SelectObject(hDCTemp,hBmpAirportReachable);
          else
            SelectObject(hDCTemp,hBmpFieldReachable);
	  intask = true; // so we can always draw them
        } else
          if ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
            SelectObject(hDCTemp,hBmpAirportUnReachable);
          else
            SelectObject(hDCTemp,hBmpFieldUnReachable);
      }
      else
      {
        SelectObject(hDCTemp,hTurnPoint);
      }

      irange = ((WayPointList[i].Zoom >= MapScale*10) || (WayPointList[i].Zoom == 0)) && (MapScale <= 10);

    if((Task[ActiveWayPoint].Index == (int)i) || irange)
      {

        DrawBitmapX(hdc,
		    WayPointList[i].Screen.x-10*InfoBoxLayout::scale, 
		    WayPointList[i].Screen.y-10*InfoBoxLayout::scale,
		    20,20,
		    hDCTemp,0,0,SRCPAINT);
	
        DrawBitmapX(hdc,
		    WayPointList[i].Screen.x-10*InfoBoxLayout::scale, 
		    WayPointList[i].Screen.y-10*InfoBoxLayout::scale,
		    20,20,
		    hDCTemp,20,0,SRCAND);
      }

      // JMW
      if (pDisplayTextType == DISPLAYNAMEIFINTASK) {

        if (intask
	    || ((((WayPointList[i].Flags & AIRPORT) == AIRPORT) || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT))&&(irange))) 
	  {

          if (DisplayMode.AsInt)
            wsprintf(Buffer, TEXT("%s:%d%s"),WayPointList[i].Name, (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), sAltUnit);
          else
            wsprintf(Buffer, TEXT("%s"),WayPointList[i].Name);

          MapWaypointLabelAdd(Buffer, 
			      WayPointList[i].Screen.x+5, 
			      WayPointList[i].Screen.y, 
			      DisplayMode, 
			      (int)
			      (WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
			      intask);

          //TextInBox(hdc, Buffer, WayPointList[i].Screen.x+5,
          //    WayPointList[i].Screen.y, 0, DisplayMode);

        }
        
      } else

        if((Task[ActiveWayPoint].Index == (int)i) || irange)
        {
          switch(pDisplayTextType)
          {

          case DISPLAYNAME:

            if (DisplayMode.AsInt)
              wsprintf(Buffer, TEXT("%s:%d%s"),
		       WayPointList[i].Name, 
		       (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
		       sAltUnit);
            else
              wsprintf(Buffer, TEXT("%s"),WayPointList[i].Name);

            break;
          case DISPLAYNUMBER:

            if (DisplayMode.AsInt)
              wsprintf(Buffer, TEXT("%d:%d%s"),
		       WayPointList[i].Number, 
		       (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
		       sAltUnit);
            else
              wsprintf(Buffer, TEXT("%d"),WayPointList[i].Number);

            break;
          case DISPLAYFIRSTFIVE:
            
            _tcsncpy(Buffer2, WayPointList[i].Name, 5);
            Buffer2[5] = '\0';
            if (DisplayMode.AsInt)
              wsprintf(Buffer, TEXT("%s:%d%s"),
		       Buffer2, 
		       (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
		       sAltUnit);
            else
              wsprintf(Buffer, TEXT("%s"),Buffer2);

            break;
          case DISPLAYFIRSTTHREE:
            _tcsncpy(Buffer2, WayPointList[i].Name, 3);
            Buffer2[3] = '\0';
            if (DisplayMode.AsInt)
              wsprintf(Buffer, TEXT("%s:%d%s"),
		       Buffer2, 
		       (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
		       sAltUnit);
            else
              wsprintf(Buffer, TEXT("%s"),Buffer2);

            break;
	  case DISPLAYNONE:
            if (DisplayMode.AsInt)
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

          MapWaypointLabelAdd(
            Buffer,
            WayPointList[i].Screen.x+5,
            WayPointList[i].Screen.y,
            DisplayMode,
            (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
	    intask);

        }
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


void MapWindow::DrawTask(HDC hdc, RECT rc)
{
  int i;
  double tmp;

  COLORREF whitecolor = RGB(0xff,0xff, 0xff);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  if (!WayPointList) return;
  
  if((Task[0].Index >=0) &&  (Task[1].Index >=0) && (ActiveWayPoint<2))
  {
    if(StartLine)
    { 
      DrawDashLine(hdc, 2, WayPointList[Task[0].Index].Screen, Task[0].End, RGB(127,127,127));
      DrawDashLine(hdc, 2, WayPointList[Task[0].Index].Screen, Task[0].Start , RGB(127,127,127));
    }
    tmp = StartRadius*ResMapScaleOverDistanceModify; 
    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    Circle(hdc,WayPointList[Task[0].Index].Screen.x,WayPointList[Task[0].Index].Screen.y,(int)tmp, rc); 
  }
  
  for(i=1;i<MAXTASKPOINTS-1;i++)
  {
    if((Task[i].Index >=0) &&  (Task[i+1].Index <0))
      { // final waypoint
	if (ActiveWayPoint>1) { 
	  // only draw finish line when past the first
	  // waypoint.

	if(FinishLine)
	  { 
	    DrawDashLine(hdc, 2, 
			 WayPointList[Task[i].Index].Screen, 
			 Task[i].End, 
			 RGB(127,127,127));
	    DrawDashLine(hdc, 2, 
			 WayPointList[Task[i].Index].Screen, 
			 Task[i].Start, 
			 RGB(127,127,127));
	  }
	tmp = FinishRadius*ResMapScaleOverDistanceModify; 
	SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	Circle(hdc,
	       WayPointList[Task[i].Index].Screen.x,
	       WayPointList[Task[i].Index].Screen.y,
	       (int)tmp, rc); 
	}
	
      }

    if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
    {
      if(AATEnabled != TRUE)
      {
        DrawDashLine(hdc, 2, 
		     WayPointList[Task[i].Index].Screen, 
		     Task[i].Start, RGB(127,127,127));
        DrawDashLine(hdc, 2, 
		     WayPointList[Task[i].Index].Screen, 
		     Task[i].End, RGB(127,127,127));
        
        if(SectorType== 0)
        {
          tmp = SectorRadius*ResMapScaleOverDistanceModify;
          SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	  SelectObject(hdc, GetStockObject(BLACK_PEN));
          Circle(hdc,
		 WayPointList[Task[i].Index].Screen.x,
		 WayPointList[Task[i].Index].Screen.y,
		 (int)tmp, rc); 
        }
        if(SectorType== 2)
        {
	  // JMW added german rules
          tmp = 500*ResMapScaleOverDistanceModify;
          SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	  SelectObject(hdc, GetStockObject(BLACK_PEN));
          Circle(hdc,
		 WayPointList[Task[i].Index].Screen.x,
		 WayPointList[Task[i].Index].Screen.y,
		 (int)tmp, rc); 
        }

      }
    }
  }

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

  // restore original color
  SetTextColor(hDCTemp, origcolor);


}


void MapWindow::DrawTaskAAT(HDC hdc, RECT rc)
{
  int i;
  double tmp;

  if (!WayPointList) return;
  if (!AATEnabled) return;

  COLORREF whitecolor = RGB(0xff,0xff, 0xff);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);

  SelectObject(hDCTemp, GetStockObject(WHITE_PEN));
  SelectObject(hDCTemp, GetStockObject(WHITE_BRUSH));
  Rectangle(hDCTemp,rc.left,rc.top,rc.right,rc.bottom);
    
  for(i=1;i<MAXTASKPOINTS-1;i++)
  {
    if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
    {
      if(Task[i].AATType == CIRCLE)
        {
          tmp = Task[i].AATCircleRadius*ResMapScaleOverDistanceModify;
	  
          // this color is used as the black bit
          SetTextColor(hDCTemp, 
		       Colours[iAirspaceColour[AATASK]]);
	  
          // this color is the transparent bit
          SetBkColor(hDCTemp, 
                     whitecolor);
	  
          SelectObject(hDCTemp, hAirspaceBrushes[iAirspaceBrush[AATASK]]);
          SelectObject(hDCTemp, GetStockObject(BLACK_PEN));
	  
          Circle(hDCTemp,
		 WayPointList[Task[i].Index].Screen.x,
		 WayPointList[Task[i].Index].Screen.y,
		 (int)tmp, rc); 
        }
      else
        {
	  
          // this color is used as the black bit
          SetTextColor(hDCTemp, 
		       Colours[iAirspaceColour[AATASK]]);
	  
          // this color is the transparent bit
          SetBkColor(hDCTemp, 
                     whitecolor);
	  
          SelectObject(hDCTemp, hAirspaceBrushes[iAirspaceBrush[AATASK]]);
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
    // old version
    //  BitBlt(hdcDrawWindowBg,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
    //	   hDCTemp,rc.left,rc.top,SRCAND /*SRCAND*/);

  TransparentImage(hdcDrawWindowBg,
		   rc.left,rc.top,
		   rc.right-rc.left,rc.bottom-rc.top,
		   hDCTemp,
		   rc.left,rc.top,
		   rc.right-rc.left,rc.bottom-rc.top,
		   whitecolor
		   );

  #else
  TransparentBlt(hdcDrawWindowBg,
		   rc.left,rc.top,
		   rc.right-rc.left,rc.bottom-rc.top,
		   hDCTemp,
		   rc.left,rc.top,
		   rc.right-rc.left,rc.bottom-rc.top,
		   whitecolor
		   );
  #endif

}



void MapWindow::DrawWindAtAircraft(HDC hdc, POINT Orig, RECT rc) {
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
      
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif

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

#if (ALTERNATEWINDVECTOR > 0)
void MapWindow::DrawWindAtAircraft2(HDC hdc, POINT Orig, RECT rc) {
  double dX,dY;
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

  POINT Arrow[7] = { {0,-20}, {-6,-26}, {0,-20}, {6,-26}, {0,-20}, {7 + tsize.cx, -22}, {-7 - tsize.cx, -22}};

  for (i=1;i<4;i++)
    Arrow[i].y -= wmag;

  for(i=0;i<7;i++)
  {
    dX = (double)Arrow[i].x ;dY = (double)Arrow[i].y;
    rotate(&dX, &dY, -1*(DisplayAngle - DerivedDrawInfo.WindBearing));
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif

    Arrow[i].x = iround(dX+Start.x);  Arrow[i].y = iround(dY+Start.y);
    
  }

  _itot(iround(DerivedDrawInfo.WindSpeed * SPEEDMODIFY), sTmp, 10);

  TextInBoxMode_t TextInBoxMode = {2 | 16};
  if (Arrow[5].y>=Arrow[6].y) {
    TextInBox(hdc, sTmp, Arrow[5].x, Arrow[5].y, 0, TextInBoxMode);
  } else {
    TextInBox(hdc, sTmp, Arrow[6].x, Arrow[6].y, 0, TextInBoxMode);
  }

  Polygon(hdc,Arrow,5);

  SelectObject(hdc, hbOld);
  SelectObject(hdc, hpOld);
}
#endif

void MapWindow::DrawWind(HDC hdc, POINT Orig, RECT rc)
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
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif
      
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


void MapWindow::DrawBearing(HDC hdc, POINT Orig)
{
  POINT Start, End;
  HPEN hpOld;

  if (!WayPointList) return;

  hpOld = (HPEN)SelectObject(hdc, hpBearing);
  
  if(ActiveWayPoint >= 0)
  {
    if (AATEnabled) {
      Start.x = Task[ActiveWayPoint].Target.x;
      Start.y = Task[ActiveWayPoint].Target.y; 
    } else {
      Start.x = WayPointList[Task[ActiveWayPoint].Index].Screen.x;
      Start.y = WayPointList[Task[ActiveWayPoint].Index].Screen.y; 
    }
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

void MapWindow::GetLocationFromScreen(double *X, double *Y) 
{
  
  *X = (*X-Orig_Screen.x)/DrawScale;
  *Y = (*Y-Orig_Screen.y)/DrawScale;
  
  rotate(X,Y,DisplayAngle);
  
  *Y = (PanYr)  - *Y;
  
  *X = *X /fastcosine(*Y);

  *X = (PanXr) + *X;
}


void MapWindow::DrawMapScale(HDC hDC, RECT rc /* the Map Rect*/ , bool ScaleChangeFeedback)
{


  if (Appearance.MapScale == apMsDefault){

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
      _stprintf(Scale,TEXT("%1.2f"),MapScale);
    }
    else if(MapScale <3)
    {
      _stprintf(Scale,TEXT("%1.1f"),MapScale);
    }
    else
    {
      _stprintf(Scale,TEXT("%1.0f"),MapScale);
    }

    _tcscat(Scale, Units::GetDistanceName());

    if (AutoZoom) {
      _tcscat(Scale,TEXT(" A"));
    }
    if (EnablePan) {
      _tcscat(Scale,TEXT(" PAN"));
    }
    if (EnableAuxiliaryInfo) {
      _tcscat(Scale,TEXT(" AUX"));
    }
    if (ReplayLogger::IsEnabled()) {
      _tcscat(Scale,TEXT(" REPLAY"));
    }
    
  /*
  extern int CacheEfficiency;
  extern int Performance;

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
  */

    SIZE tsize;
    GetTextExtentPoint(hDC, Scale, _tcslen(Scale), &tsize);

    COLORREF whitecolor = RGB(0xd0,0xd0, 0xd0);
    COLORREF blackcolor = RGB(0x20,0x20, 0x20);
    COLORREF origcolor = SetTextColor(hDC, whitecolor);

    SetTextColor(hDC, whitecolor);
    ExtTextOut(hDC, rc.right-11-tsize.cx, End.y+8, 0, NULL, Scale, _tcslen(Scale), NULL);

    SetTextColor(hDC, blackcolor);
    ExtTextOut(hDC, rc.right-10-tsize.cx, End.y+7, 0, NULL, Scale, _tcslen(Scale), NULL);

#ifdef DEBUG
    wsprintf(Scale,TEXT("%dms"), timestats_av);
    ExtTextOut(hDC, rc.left, rc.bottom-40, 0, NULL, Scale, _tcslen(Scale), NULL);
#endif

    // restore original color
    SetTextColor(hDC, origcolor);

    SelectObject(hDC, hpOld);

  }
  if (Appearance.MapScale == apMsAltA){

    static LastMapWidth = 0;
    double MapWidth;
    TCHAR ScaleInfo[50];

    HFONT          oldFont;
    int            Height;
    SIZE           TextSize;
    HBRUSH         oldBrush;
    HPEN           oldPen;
    COLORREF       oldTextColor;
    HBITMAP        oldBitMap;
    Units_t        Unit;

    if (ScaleChangeFeedback)
      MapWidth = (RequestMapScale * rc.right)*1000.0/GetMapResolutionFactor();
    else
      MapWidth = (MapScale * rc.right)*1000.0/GetMapResolutionFactor();

    oldFont = (HFONT)SelectObject(hDC, MapWindowBoldFont);
    Units::FormatUserMapScale(&Unit, MapWidth, ScaleInfo, 
			      sizeof(ScaleInfo)/sizeof(TCHAR));
    GetTextExtentPoint(hDC, ScaleInfo, _tcslen(ScaleInfo), &TextSize);
    LastMapWidth = (int)MapWidth;

    Height = Appearance.MapWindowBoldFont.CapitalHeight+2;  // 2: add 1pix border

    oldBrush = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    oldPen = (HPEN)SelectObject(hDC, GetStockObject(WHITE_PEN));
    Rectangle(hDC, 0, rc.bottom-Height, TextSize.cx + 6 + 15, rc.bottom);
    if (ScaleChangeFeedback){
      SetBkMode(hDC, TRANSPARENT);
      oldTextColor = SetTextColor(hDC, RGB(0xff,0,0));
    }else
      oldTextColor = SetTextColor(hDC, RGB(0,0,0));

    ExtTextOut(hDC, 7, rc.bottom-Appearance.MapWindowBoldFont.AscentHeight-1, 0, NULL, ScaleInfo, _tcslen(ScaleInfo), NULL);

    oldBitMap = (HBITMAP)SelectObject(hDCTemp, hBmpMapScale);

    BitBlt(hDC, 0, rc.bottom-Height, 6, 11, hDCTemp, 0, 0, SRCCOPY);
    BitBlt(hDC, 7+7+TextSize.cx, rc.bottom-Height, 8, 11, hDCTemp, 6, 0, SRCCOPY);

    if (!ScaleChangeFeedback){
      HBITMAP Bmp;
      POINT   BmpPos, BmpSize;

      if (Units::GetUnitBitmap(Unit, &Bmp, &BmpPos, &BmpSize, 0)){
        HBITMAP oldBitMap = (HBITMAP)SelectObject(hDCTemp, Bmp);
        BitBlt(hDC, 7+TextSize.cx, rc.bottom-Height, BmpSize.x, BmpSize.y, hDCTemp, BmpPos.x, BmpPos.y, SRCCOPY);
        SelectObject(hDCTemp, oldBitMap);
      }
    }

    if (!ScaleChangeFeedback){
      int y = rc.bottom-Height-(Appearance.TitleWindowFont.AscentHeight+1);
      bool FontSelected = false;
      ScaleInfo[0] = 0;
      if (AutoZoom) {
        _tcscat(ScaleInfo, TEXT("AUTO "));
      }
      if (EnablePan) {
        _tcscat(ScaleInfo, TEXT("PAN "));
      }
      if (EnableAuxiliaryInfo) {
        _tcscat(ScaleInfo, TEXT("AUX "));
      }
      if (ReplayLogger::IsEnabled()) {
        _tcscat(ScaleInfo, TEXT("REPLAY "));
      }
      if (ScaleInfo[0]) {
        SelectObject(hDC, TitleWindowFont);
        FontSelected = true;
        ExtTextOut(hDC, 1, y, 0, NULL, ScaleInfo, _tcslen(ScaleInfo), NULL);
        y -= (Appearance.TitleWindowFont.CapitalHeight+1);
      }
    }

#ifdef DEBUG
    wsprintf(ScaleInfo,TEXT("%dms"), timestats_av);
    ExtTextOut(hDC, rc.left, rc.bottom-40, 0, NULL, ScaleInfo, _tcslen(ScaleInfo), NULL);
#endif

    SetTextColor(hDC, oldTextColor);
    SelectObject(hDC, oldPen);
    SelectObject(hDC, oldFont);
    SelectObject(hDC, oldBrush);
    SelectObject(hDCTemp, oldBitMap);

  }

}


pointObj MapWindow::GlideFootPrint[NUMTERRAINSWEEPS+1];

void MapWindow::DrawGlideThroughTerrain(HDC hDC, RECT rc) {
  POINT Groundline[NUMTERRAINSWEEPS+1];
  int scx, scy;
  HPEN hpOld;

  hpOld = (HPEN)SelectObject(hDC, 
			     hpTerrainLineBg);	//sjt 02feb06 added bg line

  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    LatLon2Screen(GlideFootPrint[i].x, 
		  GlideFootPrint[i].y, 
		  &scx, &scy);
    Groundline[i].x = scx;
    Groundline[i].y = scy;
  }
  Polyline(hDC, Groundline, NUMTERRAINSWEEPS+1);	//sjt 02feb06

  (HPEN)SelectObject(hDC, hpTerrainLine);
  
  Polyline(hDC, Groundline, NUMTERRAINSWEEPS+1);


  if ((DerivedDrawInfo.TerrainWarningLatitude != 0.0)
    &&(DerivedDrawInfo.TerrainWarningLongitude != 0.0)) {

    LatLon2Screen(DerivedDrawInfo.TerrainWarningLongitude,
      DerivedDrawInfo.TerrainWarningLatitude, &scx, &scy);
    DrawBitmapIn(hDC, scx, scy, hTerrainWarning);

  }

  SelectObject(hDC, hpOld);

}

void MapWindow::DrawBestCruiseTrack(HDC hdc, POINT Orig)
{
  double dX,dY;
  int i;
  HPEN hpOld;
  HBRUSH hbOld;

  if (ActiveWayPoint<0) {
    return; // nothing to draw..
  }

  if (DerivedDrawInfo.WaypointDistance < 0.010)
    return;

  hpOld = (HPEN)SelectObject(hdc, hpBestCruiseTrack);
  hbOld = (HBRUSH)SelectObject(hdc, hbBestCruiseTrack);

  if (Appearance.BestCruiseTrack == ctBestCruiseTrackDefault){

    int dy = (long)(70); //  DerivedDrawInfo.WindSpeed );
    POINT Arrow[7] = { {-1,-40}, {1,-40}, {1,0}, {6,8}, {-6,8}, {-1,0}, {-1,-40}};

    Arrow[2].y -= dy;
    Arrow[3].y -= dy;
    Arrow[4].y -= dy;
    Arrow[5].y -= dy;

    for(i=0;i<7;i++)
    {
      dX = (double)Arrow[i].x ;dY = (double)Arrow[i].y;
      rotate(&dX, &dY, -1*(DisplayAngle - DerivedDrawInfo.BestCruiseTrack));
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif

      Arrow[i].x = iround(dX+Orig.x);  Arrow[i].y = iround(dY+Orig.y);
    }

    Polygon(hdc,Arrow,7);

  } else
  if (Appearance.BestCruiseTrack == ctBestCruiseTrackAltA){

    POINT Arrow[] = { {-1,-40}, {-1,-62}, {-6,-62}, {0,-70}, {6,-62}, {1,-62}, {1,-40}, {-1,-40}};

    for(i=0;i<(sizeof(Arrow)/sizeof(Arrow[0]));i++){
      dX = (double)Arrow[i].x ;dY = (double)Arrow[i].y;
      rotate(&dX, &dY, -1*(DisplayAngle - DerivedDrawInfo.BestCruiseTrack));
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif

      Arrow[i].x = iround(dX+Orig.x);  Arrow[i].y = iround(dY+Orig.y);
    }

    Polygon(hdc, Arrow, (sizeof(Arrow)/sizeof(Arrow[0])));

  }

  SelectObject(hdc, hpOld);
  SelectObject(hdc, hbOld);
}


void MapWindow::DrawCompass(HDC hDC,RECT rc)
{
  //	TCHAR Scale[5];
  POINT Start;
  HPEN hpOld;
  HBRUSH hbOld; 

  if (Appearance.CompassAppearance == apCompassDefault){

    Start.y = 19*InfoBoxLayout::scale+rc.top;
    Start.x = rc.right - 19*InfoBoxLayout::scale;

    if (EnableVarioGauge && MapRectBig.right == rc.right)
        Start.x -= InfoBoxLayout::ControlWidth;

    POINT Arrow[5] = { {0,-18}, {-6,10}, {0,0}, {6,10}, {0,-18}};
    double dX,dY;
    int i;

    hpOld = (HPEN)SelectObject(hDC, hpCompass);
    hbOld = (HBRUSH)SelectObject(hDC, hbCompass);

    for(i=0;i<5;i++)
    {
      dX = (double)Arrow[i].x ;dY = (double)Arrow[i].y;
      rotate(&dX, &dY, -1*DisplayAngle);
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif

      Arrow[i].x = iround(dX+Start.x);  Arrow[i].y = iround(dY+Start.y);

    }

    Polygon(hDC,Arrow,5);

    SelectObject(hDC, hbOld);
    SelectObject(hDC, hpOld);

  } else
  if (Appearance.CompassAppearance == apCompassAltA){

    static double lastDisplayAngle = 9999.9;
    static int lastRcRight = 0;
    static POINT Arrow[5] = { {0,-11}, {-5,9}, {0,3}, {5,9}, {0,-11}};
    extern bool EnableVarioGauge;

    if (lastDisplayAngle != DisplayAngle || lastRcRight != rc.right){

      Arrow[0].x  = 0;
      Arrow[0].y  = -11;
      Arrow[1].x  = -5;
      Arrow[1].y  = 9;
      Arrow[2].x  = 0;
      Arrow[2].y  = 3;
      Arrow[3].x  = 5;
      Arrow[3].y  = 9;
      Arrow[4].x  = 0;
      Arrow[4].y  = -11;

      Start.y = rc.top + 10*InfoBoxLayout::scale;
      Start.x = rc.right - 11*InfoBoxLayout::scale;
      if (EnableVarioGauge && MapRectBig.right == rc.right)
        Start.x -= InfoBoxLayout::ControlWidth;

      double dX,dY;
      int i;

      for(i=0;i<5;i++){
        dX = (double)Arrow[i].x;
        dY = (double)Arrow[i].y;
        rotate(&dX, &dY, -1*DisplayAngle);
#if (WINDOWSPC>0) 
      if (InfoBoxLayout::scale>1) {
	dX*= InfoBoxLayout::scale;
	dY*= InfoBoxLayout::scale;
      }
#endif

        Arrow[i].x = iround(dX+Start.x);
        Arrow[i].y = iround(dY+Start.y);

      }
      lastDisplayAngle = DisplayAngle;
      lastRcRight = rc.right;
    }

    hpOld = (HPEN)SelectObject(hDC, hpCompassBorder);
    hbOld = (HBRUSH)SelectObject(hDC, hbCompass);
    Polygon(hDC,Arrow,5);

    SelectObject(hDC, hpCompass);
    Polygon(hDC,Arrow,5);

    SelectObject(hDC, hbOld);
    SelectObject(hDC, hpOld);

  }

}


void MapWindow::DrawAirSpace(HDC hdc, RECT rc)
{
  unsigned i;
  POINT *pt = NULL;

  COLORREF whitecolor = RGB(0xff,0xff, 0xff);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);

  SelectObject(hDCTemp, GetStockObject(WHITE_PEN));
  SelectObject(hDCTemp, GetStockObject(WHITE_BRUSH));
  Rectangle(hDCTemp,rc.left,rc.top,rc.right,rc.bottom);

  if (bAirspaceBlackOutline) {
    SelectObject(hDCTemp, GetStockObject(BLACK_PEN));
  } else {
    SelectObject(hDCTemp, GetStockObject(WHITE_PEN));
  }

  for(i=0;i<NumberOfAirspaceCircles;i++)
  {
    if (AirspaceCircle[i].Visible) {
      if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude, 
			       AirspaceCircle[i].Top.Altitude))
	{
	  
	  if (iAirspaceMode[AirspaceCircle[i].Type]%2 == 1) {
	    
	    // this color is used as the black bit
	    SetTextColor(hDCTemp, 
			 Colours[iAirspaceColour[AirspaceCircle[i].Type]]);
	    // this color is the transparent bit
	    SetBkColor(hDCTemp,
		       whitecolor);
	    
	    // get brush, can be solid or a 1bpp bitmap
	    SelectObject(hDCTemp, 
			 hAirspaceBrushes[iAirspaceBrush[AirspaceCircle[i].Type]]);
	    
	    //	    AirspaceCircle[i].Visible = 
	    Circle(hDCTemp,AirspaceCircle[i].ScreenX ,
		   AirspaceCircle[i].ScreenY ,
		   AirspaceCircle[i].ScreenR ,rc);
	  }
	}
    }
  }
  
  /////////
  for(i=0;i<NumberOfAirspaceAreas;i++)
  {
    if(AirspaceArea[i].Visible) {
      if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, 
			       AirspaceArea[i].Top.Altitude))
	{
	  // JMW now looks for overlap in lat/lon coordinates
	  
	  if (iAirspaceMode[AirspaceArea[i].Type]%2 == 1) {
	     
	    /*
	    pt = (POINT*)SfRealloc(pt, 
				   sizeof(POINT) * AirspaceArea[i].NumPoints);
	    for(j= 0; j < AirspaceArea[i].NumPoints; j++)
	      {
		pt[j].x = 
		  AirspacePoint[j+AirspaceArea[i].FirstPoint].Screen.x ;
		pt[j].y = 
		  AirspacePoint[j+AirspaceArea[i].FirstPoint].Screen.y ;
	      }
	    */

	    // this color is used as the black bit
	    SetTextColor(hDCTemp, 
			 Colours[iAirspaceColour[AirspaceArea[i].Type]]);
	    // this color is the transparent bit
	    SetBkColor(hDCTemp, 
		       whitecolor);
	    
	    SelectObject(hDCTemp,
			 hAirspaceBrushes[iAirspaceBrush[AirspaceArea[i].Type]]);	    
	    // test only!
	    SelectObject(hDCTemp, hAirspacePens[AirspaceArea[i].Type]);
	    Polygon(hDCTemp,pt,AirspaceArea[i].NumPoints);

	    Polygon(hDCTemp,AirspaceScreenPoint+AirspaceArea[i].FirstPoint,
		    AirspaceArea[i].NumPoints);

	  }
	}      
    }
  }

  // restore original color
  SetTextColor(hDCTemp, origcolor);

  #if (WINDOWSPC<1)
    // old version
    //  BitBlt(hdcDrawWindowBg,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
    //	   hDCTemp,rc.left,rc.top,SRCAND /*SRCAND*/);

  TransparentImage(hdcDrawWindowBg,
		   rc.left,rc.top,
		   rc.right-rc.left,rc.bottom-rc.top,
		   hDCTemp,
		   rc.left,rc.top,
		   rc.right-rc.left,rc.bottom-rc.top,
		   whitecolor
		   );

  #else

  TransparentBlt(hdcDrawWindowBg,
		   rc.left,rc.top,
		   rc.right-rc.left,rc.bottom-rc.top,
		   hDCTemp,
		   rc.left,rc.top,
		   rc.right-rc.left,rc.bottom-rc.top,
		   whitecolor
		   );
  #endif

  if (pt != NULL)
    free (pt);
}


void MapWindow::CreateDrawingThread(void)
{
  CLOSETHREAD = FALSE;
  THREADEXIT = FALSE;
  hDrawThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE )MapWindow::DrawThread, 0, 0, &dwDrawThreadID);
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
  LockTerrainDataGraphics();
  CLOSETHREAD = TRUE;
  SuspendDrawingThread();
  UnlockTerrainDataGraphics();
  while(!THREADEXIT) { Sleep(100); };
}


void MapWindow::DrawThermalBand(HDC hDC,RECT rc)
{
  POINT ThermalProfile[NUMTHERMALBUCKETS+2];
  POINT GliderBand[4] = { {2,0},{23,0},{22,0},{24,0} };
  
  if ((DerivedDrawInfo.TaskAltitudeDifference>50)
      &&(DerivedDrawInfo.FinalGlide)) {
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
  
  double mc = MACCREADY;
  
  // scale to mcready
  if (mc>0.5) {
    Wtot = mc;
  } else {
    // use whatever scale thermal average gives
  }
  
  // position of thermal band
  for (i=0; i<NUMTHERMALBUCKETS; i++) {    
    ThermalProfile[1+i].x = (7+iround((Wt[i]/Wtot)*TBSCALEX))*InfoBoxLayout::scale+rc.left;
    ThermalProfile[1+i].y = 5+iround(TBSCALEY*(1.0-(mth/maxh)*(i)/NUMTHERMALBUCKETS))+rc.top;
  }
  ThermalProfile[0].x = 7*InfoBoxLayout::scale+rc.left;
  ThermalProfile[0].y = ThermalProfile[1].y;
  ThermalProfile[NUMTHERMALBUCKETS+1].x = 7*InfoBoxLayout::scale+rc.left;
  ThermalProfile[NUMTHERMALBUCKETS+1].y = ThermalProfile[NUMTHERMALBUCKETS].y;
  
  
  // position of thermal band
  GliderBand[0].y = 5+iround(TBSCALEY*(1.0-h/maxh))+rc.top;
  GliderBand[1].y = GliderBand[0].y;
  GliderBand[2].y = GliderBand[0].y-4;
  GliderBand[3].y = GliderBand[0].y+4;
  GliderBand[1].x = (7+iround((mc/Wtot)*TBSCALEX))*InfoBoxLayout::scale+rc.left;
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


void MapWindow::DrawFinalGlide(HDC hDC,RECT rc)
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
    GlideBar[i].x += rc.left;
  }
  GlideBar[0].y -= (int)Offset;
  GlideBar[1].y -= (int)Offset;
  GlideBar[2].y -= (int)Offset;

  if (Appearance.IndFinalGlide == fgFinalGlideAltA){
    for(i=0;i<sizeof(GlideBar)/sizeof(GlideBar[0]);i++){
      GlideBar[i].x -= 5; 
    }
  }

  Polygon(hDC,GlideBar,5);

  if (Appearance.IndFinalGlide == fgFinalGlideDefault){

    _stprintf(Value,TEXT("%1.0f "), ALTITUDEMODIFY*DerivedDrawInfo.TaskAltitudeDifference);

    if (Offset>=0) {
      Offset = (GlideBar[2].y+Offset)+5;
    } else {
      Offset = (GlideBar[2].y+Offset)-15;
    }

    TextInBoxMode_t TextInBoxMode = {1|8};
    TextInBox(hDC, Value, 1, (int)Offset, 0, TextInBoxMode);

  }else
  if (Appearance.IndFinalGlide == fgFinalGlideAltA){

    SIZE  TextSize;
    HFONT oldFont;
    int y = ((rc.bottom - rc.top )/2)-rc.top-Appearance.MapWindowBoldFont.CapitalHeight/2-1;
    int x = GlideBar[2].x+1;
    HBITMAP Bmp;
    POINT  BmpPos;
    POINT  BmpSize;

    _stprintf(Value, TEXT("%1.0f"), Units::ToUserAltitude(DerivedDrawInfo.TaskAltitudeDifference));

    oldFont = (HFONT)SelectObject(hDC, MapWindowBoldFont);
    GetTextExtentPoint(hDC, Value, _tcslen(Value), &TextSize);

    SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    SelectObject(hDC, GetStockObject(WHITE_PEN));
    Rectangle(hDC, x, y, x+1+TextSize.cx, y+Appearance.MapWindowBoldFont.CapitalHeight+2);

    ExtTextOut(hDC, x+1, y+rc.top+Appearance.MapWindowBoldFont.CapitalHeight-Appearance.MapWindowBoldFont.AscentHeight+1, 0, NULL, Value, _tcslen(Value), NULL);

    if (Units::GetUnitBitmap(Units::GetUserAltitudeUnit(), &Bmp, &BmpPos, &BmpSize, 0)){
      HBITMAP oldBitMap = (HBITMAP)SelectObject(hDCTemp, Bmp);
      BitBlt(hDC, x+TextSize.cx+1, y, BmpSize.x, BmpSize.y, hDCTemp, BmpPos.x, BmpPos.y, SRCCOPY);
      SelectObject(hDCTemp, oldBitMap);
    }

    SelectObject(hDC, oldFont);

  }

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);

}


#define NUMSNAILRAMP 3

COLORRAMP snail_colors[] = {
  {-10,          0xff, 0x50, 0x50},
  {0,           0x8f, 0x8f, 0x8f},
  {10,           0x50, 0xff, 0x50}
};


static int iSnailNext=0;


void MapWindow::DrawTrail( HDC hdc, POINT Orig, RECT rc)
{
  int i, j;
  int P1,P2;
  HPEN	hpNew, hpOld, hpDelete;
  BYTE Red,Green,Blue;
  int scx, scy;
  bool p1Visible = false;
  bool p2Visible = false;
  bool havep2 = true;
  static double vmax= 5.0;
  static double vmin= -5.0;
  double this_vmax=0.0;
  double this_vmin=0.0;

  if(!TrailActive)
    return;
  
  hpDelete = (HPEN)CreatePen(PS_SOLID, 2, RGB(0xFF,0xFF,0xFF));
  hpOld = (HPEN)SelectObject(hdc, hpDelete);
  
  // JMW don't draw first bit from home airport

  havep2 = false;

  int ntrail;
  if (TrailActive==1) {
    ntrail = TRAILSIZE;
  } else {
    ntrail = TRAILSIZE/TRAILSHRINK;
  }

  for(i=0;i< ntrail;i++) 
  {
    j= iSnailNext-2-i;
    if (j<0) {
      j+= TRAILSIZE;
    }
    if (j>=TRAILSIZE) {
      j-= TRAILSIZE;
    }

    if( j < TRAILSIZE-1)
    {
      P1 = j; P2 = j+1;
    }
    else
    {
      P1 = j; P2 = 0;
    }
    
    if (i == 0) {
      // set first point up

      p2Visible = PointVisible(SnailTrail[P2].Longitude , 
                               SnailTrail[P2].Latitude);
    } 
      
    p1Visible = PointVisible(SnailTrail[P1].Longitude , 
                             SnailTrail[P1].Latitude);

    // the line is invalid
    if ((P1 == iSnailNext) || (P2 == iSnailNext) || 
        (!p2Visible) || (!p1Visible)) {

      p2Visible = p1Visible;

      // p2 wasn't computed in screen coordinates, better do it next
      // time if required
      havep2 = false;
      continue;
    }

    // now we know both points are visible, better get screen coords
    // if we don't already.

    if (!havep2) {
      LatLon2Screen(SnailTrail[P2].Longitude, 
                    SnailTrail[P2].Latitude, &scx, &scy);
      SnailTrail[P2].Screen.x = scx;
      SnailTrail[P2].Screen.y = scy;
    } else {
      havep2 = false;
    }

    LatLon2Screen(SnailTrail[P1].Longitude, 
                  SnailTrail[P1].Latitude, &scx, &scy);
    SnailTrail[P1].Screen.x = scx;
    SnailTrail[P1].Screen.y = scy;
    havep2 = true; // next time our p2 will be in screen coords

    // shuffle visibility along
    p2Visible = p1Visible;

    // ok, we got this far, so draw the line

    double cv = SnailTrail[P2].Vario;

    if (cv<this_vmin) {
      this_vmin = cv;
    }
    if (cv>this_vmax) {
      this_vmax = cv;
    }

    if (cv<0) {
      cv /= (-vmin); // JMW fixed bug here
    } else {
      cv /= vmax;
    }

    ColorRampLookup((short)(cv*10), 
                    &Red, &Green, &Blue,
                    snail_colors, NUMSNAILRAMP);
          
    int width = min(8,max(2,(int)SnailTrail[P2].Vario));
    
    hpNew = (HPEN)CreatePen(PS_SOLID, width, 
                            RGB((BYTE)Red,(BYTE)Green,(BYTE)Blue));
    SelectObject(hdc,hpNew);
    DeleteObject((HPEN)hpDelete);
    hpDelete = hpNew;
    
    DrawSolidLine(hdc,SnailTrail[P1].Screen,SnailTrail[P2].Screen);
  }

  if (this_vmin<0) {
    vmin = this_vmin;
  }
  if (this_vmax>0) {
    vmax = this_vmax;
  }

  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDelete);
}


bool MapWindow::PointVisible(double lon, double lat) {
  if (lon< screenbounds_latlon.minx) return false;
  if (lon> screenbounds_latlon.maxx) return false;
  if (lat< screenbounds_latlon.miny) return false;
  if (lat> screenbounds_latlon.maxy) return false;
  return true;
}


bool MapWindow::PointVisible(POINT *P, RECT *rc)
{
  if(( P->x > rc->left ) 
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


void MapWindow::DisplayAirspaceWarning(int Type, TCHAR *Name , 
				       AIRSPACE_ALT Base, AIRSPACE_ALT Top )
{
  TCHAR szMessageBuffer[1024];
  TCHAR szTitleBuffer[1024];
  
  FormatWarningString(Type, Name , Base, Top, szMessageBuffer, szTitleBuffer );

  DoStatusMessage(TEXT("Airspace Query"), szMessageBuffer);
}



void MapWindow::LatLon2Screen(float lon, float lat, int *scX, int *scY) {
  float X, Y;
  X = (float)DrawScale*((float)PanXr - lon)*ffastcosine(lat);
  Y = (float)DrawScale*((float)PanYr  - lat);
  
  frotate(&X, &Y, (float)DisplayAngle );
  
  *scX = Orig_Screen.x - iround(X);
  *scY = Orig_Screen.y + iround(Y);
}


void MapWindow::LatLon2Screen(double lon, double lat, int *scX, int *scY) {
  double X, Y;
  X = DrawScale*(PanXr - lon)*fastcosine(lat);
  Y = DrawScale*(PanYr  - lat);
  
  rotate(&X, &Y, DisplayAngle );
  
  *scX = Orig_Screen.x - iround(X);
  *scY = Orig_Screen.y + iround(Y);
}



void MapWindow::CalculateScreenPositionsAirspace(POINT Orig, RECT rc, 
                                                 POINT *Orig_Aircraft) {
  unsigned int i,j;
  double tmp;
  int scx, scy;

  for(i=0;i<NumberOfAirspaceCircles;i++)
  {
    AirspaceCircle[i].Visible = false;
    if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude, 
			     AirspaceCircle[i].Top.Altitude))
    {

      if (msRectOverlap(&AirspaceCircle[i].bounds, &screenbounds_latlon)) {
	AirspaceCircle[i].Visible = true;

	LatLon2Screen(AirspaceCircle[i].Longitude, 
		      AirspaceCircle[i].Latitude, &scx, &scy);
	
	AirspaceCircle[i].ScreenX = scx;
	AirspaceCircle[i].ScreenY = scy;
	
	tmp = AirspaceCircle[i].Radius*ResMapScaleOverDistanceModify;
	
	AirspaceCircle[i].ScreenR = (int)tmp;

      }
    }

  }
    
  for(i=0;i<NumberOfAirspaceAreas;i++)
  {
    AirspaceArea[i].Visible = false;
    if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, 
			     AirspaceArea[i].Top.Altitude)) {
      if (msRectOverlap(&AirspaceArea[i].bounds, &screenbounds_latlon)) {
	for(j=AirspaceArea[i].FirstPoint; 
	    j<(AirspaceArea[i].FirstPoint + AirspaceArea[i].NumPoints); 
	    j++)
	  {
	    // bug fix by Samuel Gisiger
	    LatLon2Screen(AirspacePoint[j].Longitude, 
			  AirspacePoint[j].Latitude, 
			  &scx, &scy);
	    
	    AirspaceScreenPoint[j].x = max(rc.left-1,
					   min(rc.right+1,scx));
	    AirspaceScreenPoint[j].y = max(rc.top-1,
					   min(rc.bottom+1,scy));
	  }
	AirspaceArea[i].Visible = true;
      }
    }
  }
}


void MapWindow::CalculateScreenPositions(POINT Orig, RECT rc, 
                                         POINT *Orig_Aircraft)
{
  unsigned int i;
  int scx, scy;

  // compute lat lon extents of visible screen
  screenbounds_latlon = GetRectBounds(rc);
  
  Orig_Screen = Orig;
  
  PanXr = DrawInfo.Longitude + PanX;
  PanYr = DrawInfo.Latitude + PanY;
  
  LatLon2Screen(DrawInfo.Longitude, 
		DrawInfo.Latitude, 
		&scx,
		&scy);
  Orig_Aircraft->x = scx;
  Orig_Aircraft->y = scy;

  // get screen coordinates for all task waypoints

  if (WayPointList) {
    for (i=0; i<MAXTASKPOINTS; i++) {
      if (Task[i].Index>=0) {
	
	LatLon2Screen(WayPointList[Task[i].Index].Longitude, 
		      WayPointList[Task[i].Index].Latitude, 
		      &scx, &scy);
	
	WayPointList[Task[i].Index].Screen.x = scx;
	WayPointList[Task[i].Index].Screen.y = scy;
      }
      
    }
    
  }

  // only calculate screen coordinates for waypoints that are visible

  for(i=0;i<NumberOfWayPoints;i++)
  {
    
    if(PointVisible(WayPointList[i].Longitude, WayPointList[i].Latitude) )
    {
      WayPointList[i].Visible = TRUE;

      LatLon2Screen(WayPointList[i].Longitude, WayPointList[i].Latitude, &scx, &scy);
    
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
      LatLon2Screen(SnailTrail[i].Longitude, 
        SnailTrail[i].Latitude, &scx, &scy);
      
      SnailTrail[i].Screen.x = scx;
      SnailTrail[i].Screen.y = scy;
    }
    */
    
  }
  
  for(i=0;i<MAXTASKPOINTS-1;i++)
  {

    if ((Task[i].Index >=0) && AATEnabled) {
      LatLon2Screen(Task[i].AATTargetLon, Task[i].AATTargetLat, &scx, &scy);
      Task[i].Target.x  = scx;
      Task[i].Target.y = scy;
    }

    if((Task[i].Index >=0) &&  (Task[i+1].Index <0))
    {
      // finish
      LatLon2Screen(Task[i].SectorEndLon, Task[i].SectorEndLat, &scx, &scy);
      
      Task[i].End.x  = scx;
      Task[i].End.y = scy;
      
      LatLon2Screen(Task[i].SectorStartLon, Task[i].SectorStartLat, &scx, &scy);      
      Task[i].Start.x  = scx;
      Task[i].Start.y = scy;
    }

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


void MapWindow::CalculateWaypointReachable(void)
{
  unsigned int i,j;
  bool intask;
  double WaypointDistance, WaypointBearing,AltitudeRequired;

  if (!WayPointList) return;

  for(i=0;i<NumberOfWayPoints;i++)
  {
    // calculate reachable for waypoints in task also
    intask = false;
    for (j=0; j<MAXTASKPOINTS; j++) {
      if (Task[j].Index == -1) break;
      if ((unsigned int)Task[j].Index == i) {
        intask = true;
        break;
      }
    }

    if(WayPointList[i].Visible || intask)
    {
      if(  ((WayPointList[i].Flags & AIRPORT) == AIRPORT) || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) )
      {
        WaypointDistance = Distance(DrawInfo.Latitude, DrawInfo.Longitude, WayPointList[i].Latitude, WayPointList[i].Longitude);

        WaypointBearing =  Bearing(DrawInfo.Latitude, DrawInfo.Longitude, WayPointList[i].Latitude, WayPointList[i].Longitude);
        AltitudeRequired = GlidePolar::MacCreadyAltitude(0.0, 
                                     // JMW was MACCREADY
          WaypointDistance,WaypointBearing, 
          DerivedDrawInfo.WindSpeed, 
          DerivedDrawInfo.WindBearing,0,0,true,0);
        AltitudeRequired = AltitudeRequired + SAFETYALTITUDEARRIVAL + WayPointList[i].Altitude ;
        AltitudeRequired = DrawInfo.Altitude - AltitudeRequired;				
        
        WayPointList[i].AltArivalAGL = AltitudeRequired;

        if(AltitudeRequired >=0){
          WayPointList[i].Reachable = TRUE;
        } else {
          WayPointList[i].Reachable = FALSE;
        }
      }				
    }
  }
}


#define NUMPOINTS 2
void MapWindow::DrawSolidLine(HDC hdc, POINT ptStart, POINT ptEnd)
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


void MapWindow::DrawCDI() {
  bool dodrawcdi = false;

  if (DerivedDrawInfo.Circling) {
    if (EnableCDICircling) {
      dodrawcdi = true;
    }
  } else {
    if (EnableCDICruise) {
      dodrawcdi = true;
    }    
  }

  if (dodrawcdi) {
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



double MapWindow::findMapScaleBarSize(RECT rc) {

  int range = rc.bottom-rc.top;
  int nbars = 0;
  int nscale = 1;
  double pixelsize = MapScale/GetMapResolutionFactor(); // km/pixel
  
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


void MapWindow::DrawMapScale2(HDC hDC, RECT rc, POINT Orig_Aircraft)
{

  if (Appearance.MapScale2 == apMs2None) return;

  HPEN hpOld   = (HPEN)SelectObject(hDC, hpMapScale);
  HPEN hpWhite = (HPEN)CreatePen(PS_SOLID, 2, RGB(0xd0,0xd0,0xd0));
  HPEN hpBlack = (HPEN)CreatePen(PS_SOLID, 2, RGB(0x30,0x30,0x30));

  double y;
  bool color = false;
  POINT Start, End={0,0};
  bool first=true;

  double barsize = findMapScaleBarSize(rc);

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


void MapWindow::DrawSpeedToFly(HDC hDC, RECT rc) {
  POINT chevron[3];

  HPEN hpOld;
  HBRUSH hbOld;

  //  TCHAR Value[10];
  int i;

  if (Appearance.DontShowSpeedToFly || !DerivedDrawInfo.Flying)
    return;

#ifndef _SIM_
  if (!(DrawInfo.AirspeedAvailable && DrawInfo.VarioAvailable)) {
    return;
  }
#else
  // cheat
  DrawInfo.IndicatedAirspeed = DrawInfo.Speed;
#endif

  hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));
  hpOld = (HPEN)SelectObject(hDC, hpBearing);

  double vdiff;
  int vsize = (rc.bottom-rc.top)/2;

  vdiff = (DerivedDrawInfo.VOpt - DrawInfo.IndicatedAirspeed)/40.0;
  // 25.0 m/s is maximum scale
  vdiff = max(-0.5,min(0.5,vdiff)); // limit it
  
  int yoffset=0;
  int hyoffset=0;
  vsize = iround(fabs(vdiff*vsize));
  int xoffset = rc.right-25;
  int ycenter = (rc.bottom+rc.top)/2;

  int k=0;

  for (k=0; k<2; k++) {

    for (i=0; i< vsize; i+= 5) {
      if (vdiff>0) {
        yoffset = i+ycenter+k;
        hyoffset = 4;
      } else {
        yoffset = -i+ycenter-k;
        hyoffset = -4;
      }
      chevron[0].x = xoffset;
      chevron[0].y = yoffset;
      chevron[1].x = xoffset+10;
      chevron[1].y = yoffset+hyoffset;
      chevron[2].x = xoffset+20;
      chevron[2].y = yoffset;
      
      Polyline(hDC, chevron, 3);
    }
    if (vdiff>0) {
      hpOld = (HPEN)SelectObject(hDC, hpSpeedSlow);
    } else {
      hpOld = (HPEN)SelectObject(hDC, hpSpeedFast);
    }
  }

  SelectObject(hDC, hpBearing);
  chevron[0].x = xoffset-3;
  chevron[0].y = ycenter;
  chevron[1].x = xoffset+3+20;
  chevron[1].y = ycenter;
  Polyline(hDC, chevron, 2);
    
  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);

}


void MapWindow::DrawFLARMTraffic(HDC hDC, RECT rc) {
  if (!DrawInfo.FLARM_Available) return;

  HPEN hpOld;
//  POINT Arrow[2];

  hpOld = (HPEN)SelectObject(hDC, hpBestCruiseTrack);

  int i;
  int scx, scy;
//  double dX, dY;
  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (_tcslen(DrawInfo.FLARM_Traffic[i].ID)>0) {
      
      // TODO: draw direction, height?
      LatLon2Screen(DrawInfo.FLARM_Traffic[i].Longitude, 
		    DrawInfo.FLARM_Traffic[i].Latitude, 
		    &scx, &scy);

      /*  Direction line disabled, not working very well

      Arrow[0].x = scx;
      Arrow[0].y = scy;

      double vmag = max(1.0,min(15.0,DrawInfo.FLARM_Traffic[i].Speed/5.0))*2;

      dX = 0; dY = vmag;

      rotate(&dX, &dY, -1*(DisplayAngle 
			   - DrawInfo.FLARM_Traffic[i].TrackBearing));

      Arrow[1].x = iround(dX+scx);  Arrow[1].y = iround(dY+scy);

      Polygon(hDC,Arrow,2);
      */

      DrawBitmapIn(hDC, scx, scy, hFLARMTraffic);

    }
  }

  SelectObject(hDC, hpOld);

}


//////////////////////
// JMW added simple code to prevent text writing over map city names
int MapWindow::nLabelBlocks;
RECT MapWindow::LabelBlockCoords[MAXLABELBLOCKS];

bool MapWindow::checkLabelBlock(RECT rc) {
  bool ok = true;

  for (int i=0; i<nLabelBlocks; i++) {
    if (CheckRectOverlap(LabelBlockCoords[i],rc)) {
      ok = false;
      continue;
    }
  }
  if (nLabelBlocks<MAXLABELBLOCKS-1) {
    LabelBlockCoords[nLabelBlocks]= rc;
    nLabelBlocks++;
  }
  return ok;
}
