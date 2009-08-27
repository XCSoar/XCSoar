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
#include "Interface.hpp"
#include "Protection.hpp"
#include "Utils.h"
#include "Utils2.h"
#include "UtilsSystem.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "McReady.h"
#include "Dialogs.h"
#include "Blackboard.hpp"
#include "Settings.hpp"
#include "SettingsTask.hpp"
#include "SettingsUser.hpp"
#include "Audio/VarioSound.h"
#include "InputEvents.h"
#include "Trigger.hpp"
#include "Screen/Blank.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Util.hpp"
#include "Screen/Fonts.hpp"
#include "Compatibility/gdi.h"
#include "TopologyStore.h"
#include "GaugeVarioAltA.h"
#include "GaugeFLARM.h"
#include "InfoBoxLayout.h"
#include "InfoBoxManager.h"
#include "RasterTerrain.h"

#include "Calculations.h" // TODO danger! for InAATTurnSector

#ifdef PNA
#include "Asset.hpp"
#endif

#include <assert.h>
#include <windows.h>
#include <math.h>
#include <tchar.h>

#if (WINDOWSPC>0)
#include <wingdi.h>
#endif

#ifndef NDEBUG
#define DRAWLOAD
#define DEBUG_VIRTUALKEYS
#endif

///////////////////////////////// Settings

DisplayTextType_t    DisplayTextType = DISPLAYNONE;
int TrailActive = true;
int VisualGlide = 0;
DisplayMode_t UserForceDisplayMode = dmNone;
DisplayMode_t DisplayMode = dmCruise;
bool EnableTrailDrift=false;
bool bAirspaceBlackOutline = false;
bool EnableCDICruise = false;
bool EnableCDICircling = false;
unsigned char DeclutterLabels = 0;
// 12 is number of airspace types
int      iAirspaceBrush[AIRSPACECLASSCOUNT] =
  {2,0,0,0,3,3,3,3,0,3,2,3,3,3};
int      iAirspaceColour[AIRSPACECLASSCOUNT] =
  {5,0,0,10,0,0,10,2,0,10,9,3,7,7};
int      iAirspaceMode[AIRSPACECLASSCOUNT] =
  {0,0,0,0,0,0,0,0,0,0,0,1,1,0};
bool AutoZoom = false;
int SnailWidthScale = 16;
int WindArrowStyle = 0;

///////////////////////////////// Initialisation

ScreenGraphics MapGfx;

///

int misc_tick_count=0;

int MapWindow::TargetDrag_State = 0;
double MapWindow::TargetDrag_Latitude = 0;
double MapWindow::TargetDrag_Longitude = 0;

///////////////////
NMEA_INFO MapWindowData::DrawInfo;
DERIVED_INFO MapWindowData::DerivedDrawInfo;

///////////////////

BOOL  MapWindow::Initialised = FALSE;
bool  MapWindow::BigZoom = true;
bool  MapWindow::LandableReachable = false;
POINT MapWindow::Groundline[NUMTERRAINSWEEPS+1];
DWORD MapWindow::fpsTime0 = 0;
bool  MapWindow::MapFullScreen = false;
bool  MapWindow::askFullScreen = false;
bool  MapWindow::askVisibilityScan = false;

/////////////////////////////////

extern void ShowMenu();

HDC MapWindow::hDCTemp;
HDC MapWindow::hDCMask;
HDC MapWindow::hdcDrawWindow;
HDC MapWindow::hdcScreen;
HBITMAP MapWindow::hMaskBitMap;
HBITMAP MapWindow::hDrawBitMap;
HBITMAP MapWindow::hDrawBitMapTmp;

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


bool MapWindow::user_asked_redraw = false;


void MapWindow::RefreshMap() {
  dirtyEvent.trigger();
  user_asked_redraw = true;
  timestats_dirty = true;
  drawTriggerEvent.trigger();
}


bool MapWindow::isMapFullScreen() {
  // SDP - Seems that RequestFullScreen
  // is always more accurate (MapFullSCreen is delayed)
  return askFullScreen;
}


void MapWindow::StoreRestoreFullscreen(bool store) {
  static bool oldfullscreen = 0;
  static bool SuperPan = false;
  if (store) {
    // pan not active on entry, save fullscreen status
    SuperPan = true;
    oldfullscreen = MapWindow::isMapFullScreen();
  } else {
    if (SuperPan) {
      // pan is active, need to restore
      if (!oldfullscreen) {
        // change it if necessary
        askFullScreen = false;
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

      MapGfx.Destroy();

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
      LockTaskData();
      if (AATEnabled && TargetPan && (TargetDrag_State>0)) {
	// target follows "finger" so easier to drop near edge of
	// sector
        if (TargetDrag_State == 1) {
          POINT Pos;
          double mouseMovelon, mouseMovelat;
          Pos.x = LOWORD(lParam);
          Pos.y = HIWORD(lParam);

          Screen2LatLon((int)Pos.x, (int)Pos.y, mouseMovelon, mouseMovelat);
          if (InAATTurnSector(mouseMovelon, mouseMovelat, TargetPanIndex)) {
            // update waypoints so if we drag out of the cylinder, it
            // will remain adjacent to the edge
            Task[TargetPanIndex].AATTargetLat = mouseMovelat;
            Task[TargetPanIndex].AATTargetLon = mouseMovelon;
            TargetDrag_Latitude = mouseMovelat;
            TargetDrag_Longitude = mouseMovelon;
            DrawBitmapIn(hdcScreen, Pos, MapGfx.hBmpTarget);
          }
        }
      }
      UnlockTaskData();
      break;

    case WM_LBUTTONDOWN:
      #ifdef DEBUG_DBLCLK
      DoStatusMessage(_T("BUTTONDOWN MapWindow"));
      #endif
      ResetDisplayTimeOut();
      dwDownTime = GetTickCount();
      if (ignorenext) break;
      XstartScreen = LOWORD(lParam); YstartScreen = HIWORD(lParam);
      // TODO VNT move Screen2LatLon in LBUTTONUP after making sure we
      // really need Xstart and Ystart so we save precious
      // milliseconds waiting for BUTTONUP GetTickCount
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
      bool my_target_pan;
      dwUpTime = GetTickCount();
      dwInterval=dwUpTime-dwDownTime;
      dwDownTime=0; // do it once forever
/*
	TCHAR buf[80];
	wsprintf(buf,_T("Interval %ldms"),dwInterval);
        DoStatusMessage(buf);
*/

      LockTaskData();
      my_target_pan = TargetPan;
      UnlockTaskData();

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

      // Caution, timed clicks from PC with a mouse are different
      // from real touchscreen devices

      if ((VirtualKeys==(VirtualKeys_t)vkEnabled) &&
	  (distance<50) && (dwInterval>= DOUBLECLICKINTERVAL)) {
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

      if (AATEnabled && my_target_pan && (TargetDrag_State>0)) {
	LockTaskData();
	TargetDrag_State = 2;
        if (InAATTurnSector(Xlat, Ylat, TargetPanIndex)) {
            // if release mouse out of sector, don't update w/ bad coords
	  TargetDrag_Latitude = Ylat;
	  TargetDrag_Longitude = Xlat;
        }
	UnlockTaskData();
	break;
      } else if (!my_target_pan && EnablePan && (distance>36)) {
	// TODO FIX should be IBLSCALE 36 instead?
	PanLongitude += Xstart-Xlat;
	PanLatitude  += Ystart-Ylat;
	RefreshMap();
	break;
      }
#ifdef _SIM_
      else if (!my_target_pan && (distance>IBLSCALE(36))) {
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
      if (!my_target_pan) {
	if ( InfoFocus>=0) { //
	  DefocusInfoBox();
	  SetFocus(hWnd);
#ifndef DISABLEAUDIO
	  if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
#endif
	  break;
	}
	if (VirtualKeys==(VirtualKeys_t)vkEnabled) {
	  if(dwInterval < VKSHORTCLICK) {
	    //100ms is NOT enough for a short click since GetTickCount
	    //is OEM custom!
	    if (PopupNearestWaypointDetails(Xstart, Ystart, 500*MapScale, false)) {
	      break;
	    }
	  } else {
	    if (PopupInteriorAirspaceDetails(Xstart, Ystart)) {
	      break;
	    }
	  }
	} else {
	  if(dwInterval < AIRSPACECLICK) { // original and untouched interval
	    if (PopupNearestWaypointDetails(Xstart, Ystart, 500*MapScale, false)) {
	      break;
	    }
	  } else {
	    if (PopupInteriorAirspaceDetails(Xstart, Ystart)) {
	      break;
	    }
	  }
	} // VK enabled
      } // !TargetPan

      break;
      /*
	case WM_PAINT:
	if ((hWnd == hWndMapWindow) && (ProgramStarted==3)) {
	// drawTriggerEvent.trigger();

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
	}
      else if ( GlobalModelType == MODELTYPE_PNA_PN6000 )
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
      else if ( GlobalModelType == MODELTYPE_PNA_NOKIA_500 )
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
      else if ( GlobalModelType == MODELTYPE_PNA_MEDION_P5 )
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
      if (InputEvents::processKey(wParam)) {
	return TRUE; // don't go to default handler
      }
      // break; unreachable!
    }

  return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}


bool MapWindow::RenderTimeAvailable() {
  DWORD fpsTime = ::GetTickCount();

  if (dirtyEvent.test())
    return false;

  if (fpsTime-timestamp_newdata<700) {
    // it's been less than 700 ms since last data
    // was posted
    return true;
  } else {
    return false;
  }
}



void MapWindow::UpdateInfo(NMEA_INFO *nmea_info,
                           DERIVED_INFO *derived_info) {
  LockFlightData();
  memcpy(&DrawInfo,nmea_info,sizeof(NMEA_INFO));
  memcpy(&DerivedDrawInfo,derived_info,sizeof(DERIVED_INFO));
  UpdateMapScale(); // done here to avoid double latency due to locks

  DisplayMode_t lastDisplayMode = DisplayMode;
  switch (UserForceDisplayMode) {
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
    if (DerivedDrawInfo.Circling){
      DisplayMode = dmCircling;
    } else if (DerivedDrawInfo.FinalGlide){
      DisplayMode = dmFinalGlide;
    } else
      DisplayMode = dmCruise;
    break;
  }
  if (lastDisplayMode != DisplayMode){
    SwitchZoomClimb();
  }

  UnlockFlightData();
}


void MapWindow::UpdateCaches(const bool force) {
  // map was dirtied while we were drawing, so skip slow process
  // (unless we haven't done it for 2000 ms)
  DWORD fpsTimeThis = ::GetTickCount();
  static double lastTime = 0;
  static DWORD fpsTimeMapCenter = 0;
  bool do_force = force | askVisibilityScan;

  askVisibilityScan = false; // reset

  // have some time, do shape file cache update if necessary
  LockTerrainDataGraphics();
  SetTopologyBounds(MapRect, do_force);
  UnlockTerrainDataGraphics();

  // JMW experimental jpeg2000 rendering/tile management
  // Must do this even if terrain is not displayed, because
  // raster terrain is used by terrain footprint etc.
  if (lastTime>DrawInfo.Time) {
    lastTime = DrawInfo.Time;
  }

  if (do_force || (fpsTimeThis - fpsTimeMapCenter > 5000)) {

    fpsTimeThis = fpsTimeMapCenter;
    RasterTerrain::ServiceTerrainCenter(DrawInfo.Latitude,
                                        DrawInfo.Longitude);
  }

  fpsTimeThis = ::GetTickCount();
  static DWORD fpsTimeLast_terrain=0;

  if (RenderTimeAvailable() ||
      (fpsTimeThis-fpsTimeLast_terrain>5000) || do_force) {
    // have some time, do graphics terrain cache update if necessary
    if (EnableTerrain) {
      fpsTimeLast_terrain = fpsTimeThis;
      RasterTerrain::ServiceCache();
    }
  }
}


extern MapWindow hWndMapWindow; // TODO try to avoid this

void MapWindow::DrawThreadLoop(bool first_time) {

  if (!dirtyEvent.test() && !first_time) {
    // redraw old screen, must have been a request for fast refresh
    BitBlt(hdcScreen, 0, 0, MapRectBig.right-MapRectBig.left,
	   MapRectBig.bottom-MapRectBig.top,
	   hdcDrawWindow, 0, 0, SRCCOPY);
    return;
  } else {
    dirtyEvent.reset();
  }

  UpdateInfo(&GPS_INFO, &CALCULATED_INFO);

  if (BigZoom) {
    // quickly draw zoom level on top
    DrawMapScale(hdcScreen, MapRect, true);
  }

  if (askFullScreen != MapFullScreen) {
    ToggleFullScreenStart();
  }

  GaugeFLARM::Render(&DrawInfo);

  RenderMapWindow(hdcDrawWindow, MapRect);

  if (!first_time) {
    BitBlt(hdcScreen, 0, 0,
	   MapRectBig.right-MapRectBig.left,
	   MapRectBig.bottom-MapRectBig.top,
	   hdcDrawWindow, 0, 0, SRCCOPY);
    HWND hWndMapWindow = ::hWndMapWindow;
    InvalidateRect(hWndMapWindow, &MapRect, false);
  }
  UpdateTimeStats(false);
  // we do caching after screen update, to minimise perceived delay
  UpdateCaches(first_time);
}


void MapWindow::DrawThreadInitialise(void) {
  HWND hWndMapWindow = ::hWndMapWindow;

  // initialise other systems
  InitialiseScaleList(); // should really be done before the thread
			 // has started, so it happens from main thread

  LabelBlockReset();

  // set main rectangles
  GetClientRect(hWndMapWindow, &MapRectBig);
  MapRectSmall = MapRect;
  MapRect = MapRectSmall;

  UpdateTimeStats(true);

  // set initial display mode
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
  dirtyEvent.trigger();
  UpdateTimeStats(true);
  //////

  RequestMapScale = MapScale;
  UpdateMapScale(); // first call
}


DWORD MapWindow::DrawThread (LPVOID lpvoid)
{
  while (!globalRunningEvent.test() || !Initialised) {
    // wait for start
    Sleep(100);
  }

  DrawThreadInitialise();

  DrawThreadLoop(true); // first time draw
  // mode advance
  if (ProgramStarted==psInitDone) {
    ProgramStarted = psFirstDrawDone;
    GaugeVario::Show(!MapFullScreen);
  }

  do {

    if (!THREADRUNNING) {
      Sleep(100); // wait around if suspended
      continue;
    }
    DrawThreadLoop(false);
    drawTriggerEvent.wait(5000);

  } while (!closeTriggerEvent.test());

  return 0;
}

