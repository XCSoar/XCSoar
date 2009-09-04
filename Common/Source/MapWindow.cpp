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
#include "Components.hpp"
#include "Interface.hpp"
#include "LogFile.hpp"
#include "Protection.hpp"
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
#include "SettingsComputer.hpp"
#include "Audio/VarioSound.h"
#include "InputEvents.h"
#include "ReplayLogger.hpp"
#include "Screen/Blank.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Util.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/LabelBlock.hpp"
#include "Compatibility/gdi.h"
#include "TopologyStore.h"
#include "InfoBoxLayout.h"
#include "InfoBoxManager.h"
#include "RasterTerrain.h"
#include "Gauge/GaugeFLARM.hpp"
#include "Message.h"
#include "Calculations.h" // TODO danger! for InAATTurnSector
#include "RasterWeather.h"
#include "options.h" /* for DEBUG_VIRTUALKEYS */
#include "Defines.h" /* for DEBUG_VIRTUALKEYS */

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

//////////////////////////


MapWindow::MapWindow()
  :MapWindowProjection(),
   TargetDrag_State(0),
   TargetDrag_Latitude(0),
   TargetDrag_Longitude(0),
   BigZoom(true),
   LandableReachable(false),
   MapFullScreen(false),
   askFullScreen(false)
{

}


void MapWindow::RefreshMap() {
  MapWindowTimer::InterruptTimer();
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

void MapWindow::ReadBlackboard(const NMEA_INFO &nmea_info,
			       const DERIVED_INFO &derived_info) {
  mutexFlightData.Lock();
  MapWindowBlackboard::ReadBlackboard(nmea_info, derived_info);

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
    if (Calculated().Circling){
      DisplayMode = dmCircling;
    } else if (Calculated().FinalGlide){
      DisplayMode = dmFinalGlide;
    } else
      DisplayMode = dmCruise;
    break;
  }
  if (lastDisplayMode != DisplayMode){
    SwitchZoomClimb();
  }

  MapWindowProjection::ExchangeBlackboard(nmea_info, derived_info);
  mutexFlightData.Unlock();
}


typedef struct {
  DWORD time_last;
  bool dirty;
} MapIdleTrigger;


// This idle function allows progressive scanning of visibility etc
// 
bool MapWindow::Idle(const bool do_force) {
  bool still_dirty=false;

  StartTimer();

  static MapIdleTrigger main_idle;
  static MapIdleTrigger terrain_idle;
  static MapIdleTrigger topology_idle;
  static MapIdleTrigger rasp_idle;

  if (do_force) {
    main_idle.dirty = true;
    terrain_idle.dirty = true;
    topology_idle.dirty = true;
    rasp_idle.dirty = true;
    topology->TriggerUpdateCaches(*this);
  }

  do {

    // scan main object visibility
    if (main_idle.dirty) {
      main_idle.dirty = false;
      ScanVisibility(getSmartBounds());
      continue;
    }

    if (do_force) {
      // exit after important object visibilities are scanned
      // this ensures waypoints/airspace are visible after a significant
      // shift of the map
      return true;
    }
    
    if (topology_idle.dirty) {
      if (EnableTopology) {
	topology_idle.dirty = 
	  topology->ScanVisibility(*this, *getSmartBounds(), do_force);
      } else {
	topology_idle.dirty = false;
      }
      continue;
    }

    if (terrain_idle.dirty) {
      terrain.ServiceTerrainCenter(Basic().Latitude,
				   Basic().Longitude);
      terrain.ServiceCache();
      terrain_idle.dirty = false;
      continue;
    }

    if (rasp_idle.dirty) {
      RASP.SetViewCenter(Basic().Latitude,
			 Basic().Longitude);
      rasp_idle.dirty = false;
      continue;
    }

  } while (RenderTimeAvailable() && 
	   (still_dirty = 
	      main_idle.dirty 
	    | terrain_idle.dirty 
	    | topology_idle.dirty
	    | rasp_idle.dirty));

  return still_dirty;
}


void MapWindow::DrawThreadLoop(void) {

  StartTimer();

  mutexFlightData.Lock();
  ReadBlackboard(device_blackboard.Basic(), device_blackboard.Calculated());
  mutexFlightData.Unlock();

  if (BigZoom) {
    // quickly draw zoom level on top
    DrawMapScale(get_canvas(), MapRect, true);
  }

  if (askFullScreen != MapFullScreen) {
    ToggleFullScreenStart();
  }

  if (gauge_flarm != NULL)
    gauge_flarm->Render(&Basic());

  Render(draw_canvas, MapRect);

  // copy to canvas
  get_canvas().copy(draw_canvas);
  update(MapRect);

  StopTimer();
}


void MapWindow::DrawThreadInitialise(void) {
  // initialise other systems
  InitialiseScaleList(); // should really be done before the thread
			 // has started, so it happens from main thread

  // set initial display mode
  draw_canvas.background_transparent();

  // paint draw window black to start
  draw_canvas.black_pen();
  draw_canvas.rectangle(MapRectBig.left, MapRectBig.top,
			MapRectBig.right, MapRectBig.bottom);

  get_canvas().copy(draw_canvas);

  ToggleFullScreenStart();
}


DWORD MapWindow::_DrawThread ()
{
  bool bounds_dirty = false;

  // wait for start
  globalRunningEvent.wait();

  DrawThreadInitialise();

  mutexRun.Lock(); // take control
  DrawThreadLoop(); // first time draw
  bounds_dirty = SmartBounds(true);
  Idle(true);
  while (Idle(false)) {};
  DrawThreadLoop(); // first time draw
  mutexRun.Unlock(); // release control

  do {
    if (drawTriggerEvent.wait(MIN_WAIT_TIME)) {
      mutexRun.Lock(); // take control
      DrawThreadLoop();
      if (SmartBounds(false)) {
	bounds_dirty = Idle(true); // this call is quick
      }
      mutexRun.Unlock(); // release control
      continue;
    } 
    if (bounds_dirty && !drawTriggerEvent.test()) {
      mutexRun.Lock(); // take control
      bounds_dirty = Idle(false);
      mutexRun.Unlock(); // release control
      continue;
    } 
  } while (!closeTriggerEvent.wait(500));

  return 0;
}


bool
MapWindow::register_class(HINSTANCE hInstance)
{

  WNDCLASS wc;

  wc.hInstance = hInstance;
  wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = MapWindow::WndProc;
  wc.cbClsExtra = 0;
#if (WINDOWSPC>0)
  wc.cbWndExtra = 0 ;
#else
  WNDCLASS dc;
  GetClassInfo(hInstance,TEXT("DIALOG"),&dc);
  wc.cbWndExtra = dc.cbWndExtra ;
#endif
  wc.hIcon = (HICON)NULL;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = _T("XCSoarMap");

  return (RegisterClass(&wc)!= FALSE);
}

bool MapWindow::checkLabelBlock(const RECT brect) {
  return label_block.check(brect);
}


/////////////////////////////////////////

bool MapWindow::on_resize(unsigned width, unsigned height) {
  MaskedPaintWindow::on_resize(width, height);

  draw_canvas.resize(width, height);
  buffer_canvas.resize(width, height);

  SetFontInfoAll(get_canvas());

  return true;
}

bool MapWindow::on_create()
{
  if (!MaskedPaintWindow::on_create())
    return false;

  draw_canvas.set(get_canvas());
  buffer_canvas.set(get_canvas());
  return true;
}

bool MapWindow::on_destroy()
{
  draw_canvas.reset();
  buffer_canvas.reset();

  MaskedPaintWindow::on_destroy();
  return true;
}

///////

static double Xstart, Ystart;
static int XstartScreen, YstartScreen;
static bool ignorenext=true;
static DWORD dwDownTime= 0L, dwUpTime= 0L, dwInterval= 0L;

bool MapWindow::on_mouse_double(int x, int y)
{
  // Added by ARH to show menu button when mapwindow is double clicked.
  //
  // VNT TODO: do not handle this event and remove CS_DBLCLKS in register class.
  // Only handle timed clicks in BUTTONDOWN with no proximity.
  //
  dwDownTime = GetTickCount();
#ifndef DISABLEAUDIO
  if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
#endif
  InputEvents::ShowMenu();
  ignorenext = true;
  return true;
}

bool MapWindow::on_mouse_move(int x, int y)
{
  mutexTaskData.Lock();
  if (AATEnabled && TargetPan && (TargetDrag_State>0)) {
    // target follows "finger" so easier to drop near edge of
    // sector
    if (TargetDrag_State == 1) {
      double mouseMovelon, mouseMovelat;
      Screen2LonLat((int)x, (int)y, mouseMovelon, mouseMovelat);
      if (InAATTurnSector(mouseMovelon, mouseMovelat, TargetPanIndex)) {
	// update waypoints so if we drag out of the cylinder, it
	// will remain adjacent to the edge
	Task[TargetPanIndex].AATTargetLat = mouseMovelat;
	Task[TargetPanIndex].AATTargetLon = mouseMovelon;
	TargetDrag_Latitude = mouseMovelat;
	TargetDrag_Longitude = mouseMovelon;
	draw_masked_bitmap(get_canvas(), MapGfx.hBmpTarget, x, y, 10, 10, true);
      }
    }
  }
  mutexTaskData.Unlock();
  return true;
}

bool MapWindow::on_mouse_down(int x, int y)
{
  ResetDisplayTimeOut();
  dwDownTime = GetTickCount();
  if (ignorenext) return true;

  // TODO VNT move Screen2LonLat in LBUTTONUP after making sure we
  // really need Xstart and Ystart so we save precious
  // milliseconds waiting for BUTTONUP GetTickCount
  Screen2LonLat(x, y, Xstart, Ystart);
  XstartScreen = x;
  YstartScreen = y;

  mutexTaskData.Lock();
  if (AATEnabled && TargetPan) {
    if (ValidTaskPoint(TargetPanIndex)) {
      POINT tscreen;
      LonLat2Screen(Task[TargetPanIndex].AATTargetLon,
		    Task[TargetPanIndex].AATTargetLat,
		    tscreen);
      double distance = isqrt4((long)((XstartScreen-tscreen.x)
			       *(XstartScreen-tscreen.x)+
			       (YstartScreen-tscreen.y)
			       *(YstartScreen-tscreen.y)))
	/InfoBoxLayout::scale;
      
      if (distance<10) {
	TargetDrag_State = 1;
      }
    }
  }
  mutexTaskData.Unlock();
  return true;
}




bool MapWindow::on_mouse_up(int x, int y)
{
  if (ignorenext||dwDownTime==0) {
    ignorenext=false;
    return true;
  }
  RECT rc = MapRect;
  bool my_target_pan;
  dwUpTime = GetTickCount();
  dwInterval=dwUpTime-dwDownTime;
  dwDownTime=0; // do it once forever

  mutexTaskData.Lock();
  my_target_pan = TargetPan;
  mutexTaskData.Unlock();

  InfoBoxManager::Defocus();

  if (dwInterval == 0) {
#ifdef DEBUG_VIRTUALKEYS
    Message::AddMessage(_T("dwInterval==0 impossible!"));
#endif
    return true; // should be impossible
  }

  double distance = isqrt4((long)((XstartScreen-x)*(XstartScreen-x)+
			   (YstartScreen-y)*(YstartScreen-y)))
    /InfoBoxLayout::scale;

#ifdef DEBUG_VIRTUALKEYS
  TCHAR buf[80]; char sbuf[80];
  sprintf(sbuf,"%.0f",distance);
  _stprintf(buf,_T("XY=%d,%d dist=%S Up=%ld Down=%ld Int=%ld"),
	    x,y,sbuf,dwUpTime,dwDownTime,dwInterval);
  Message::AddMessage(buf);
#endif

  // Caution, timed clicks from PC with a mouse are different
  // from real touchscreen devices
  
  if ((distance<50) 
       && (VirtualKeys==(VirtualKeys_t)vkEnabled) 
       && (dwInterval>= DOUBLECLICKINTERVAL)) {
    unsigned wParam=ProcessVirtualKey(x,y,dwInterval,0);
    if (wParam==0) {
#ifdef DEBUG_VIRTUALKEYS
      Message::AddMessage(_T("E02 INVALID Virtual Key!"));
#endif
      return true;
    }
    //    dwDownTime= 0L;
    //    InputEvents::processKey(wParam);
    //    return;
  }

  double Xlat, Ylat;
  Screen2LonLat(x, y, Xlat, Ylat);

  if (AATEnabled && my_target_pan && (TargetDrag_State>0)) {
    mutexTaskData.Lock();
    TargetDrag_State = 2;
    if (InAATTurnSector(Xlat, Ylat, TargetPanIndex)) {
      // if release mouse out of sector, don't update w/ bad coords
      TargetDrag_Latitude = Ylat;
      TargetDrag_Longitude = Xlat;
    }
    mutexTaskData.Unlock();
    return true;
  }
 
  if (!my_target_pan && EnablePan && (distance>IBLSCALE(36))) {
    PanLongitude += Xstart-Xlat;
    PanLatitude  += Ystart-Ylat;
    RefreshMap();
    return true;
  }

#ifdef _SIM_
  if (!ReplayLogger::IsEnabled() && !my_target_pan && (distance>IBLSCALE(36))) {
    // This drag moves the aircraft (changes speed and direction)
    double newbearing;
    double oldbearing = XCSoarInterface::Basic().TrackBearing;
    double minspeed = 1.1*GlidePolar::Vminsink;
    DistanceBearing(Ystart, Xstart, Ylat, Xlat, NULL, &newbearing);
    if ((fabs(AngleLimit180(newbearing-oldbearing))<30)
	|| (XCSoarInterface::Basic().Speed<minspeed)) {

      device_blackboard.SetSpeed(min(100.0,max(minspeed,distance/3)));
    }
    device_blackboard.SetTrackBearing(newbearing);
    // change bearing without changing speed if direction change > 30
    // 20080815 JMW prevent dragging to stop glider
    
    // JMW trigger recalcs immediately
    TriggerGPSUpdate();
    return true;
  }
#endif

  if (!my_target_pan) {
    if (InfoBoxManager::Defocus()) {
      return false;
    }
    if (VirtualKeys==(VirtualKeys_t)vkEnabled) {
      if(dwInterval < VKSHORTCLICK) {
	//100ms is NOT enough for a short click since GetTickCount
	//is OEM custom!
	if (PopupNearestWaypointDetails(Xstart, Ystart, 
					DistancePixelsToMeters(IBLSCALE(10)), false)) {
	  return true;
	}
      } else {
	if (PopupInteriorAirspaceDetails(Xstart, Ystart)) {
	  return true;
	}
      }
    } else {
      if(dwInterval < AIRSPACECLICK) { // original and untouched interval
	if (PopupNearestWaypointDetails(Xstart, Ystart, 
					DistancePixelsToMeters(IBLSCALE(10)), false)) {
	  return true;
	}
      } else {
	if (PopupInteriorAirspaceDetails(Xstart, Ystart)) {
	  return true;
	}
      }
    } // VK enabled
  } // !TargetPan

  return false;
}


bool MapWindow::on_key_down(unsigned key_code)
{
  // VENTA-TODO careful here, keyup no more trapped for PNA.
  // Forbidden usage of keypress timing.
  
  ResetDisplayTimeOut();
  XCSoarInterface::InterfaceTimeoutReset();
  key_code = TranscodeKey(key_code);
#if defined(GNAV)
  if (key_code == 0xF5){
    SignalShutdown(false);
    return true;
  }
#endif
  dwDownTime= 0L;
  if (InputEvents::processKey(key_code)) {
    return true; // don't go to default handler
  }

  return MaskedPaintWindow::on_key_down(key_code);
}

void MapWindow::on_paint(Canvas& _canvas) {
  _canvas.copy(draw_canvas);
}

//////////////////////////
//

DWORD MapWindow::DrawThread (LPVOID lpvoid)
{
  MapWindow *mw = (MapWindow *)lpvoid;
  mw->_DrawThread();
}


//////////////////////////////


bool MapWindow::draw_masked_bitmap_if_visible(Canvas &canvas,
					      Bitmap &bitmap,
					      const double &lon,
					      const double &lat,
					      unsigned width,
					      unsigned height,
					      POINT *scin)
{
  POINT sc;
  POINT *scp = (scin!=NULL)? (scin):(&sc);

  if (LonLat2ScreenIfVisible(lon, lat, scp)) {
    draw_masked_bitmap(canvas, bitmap, scp->x, scp->y, width, height, true);
    return true;
  }
  return false;
}
