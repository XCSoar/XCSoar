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
#include "Audio/VarioSound.h"
#include "InputEvents.h"
#include "Trigger.hpp"
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

MapWindow::MapWindow()
  :MapWindowProjection(),
   TargetDrag_State(0),
   TargetDrag_Latitude(0),
   TargetDrag_Longitude(0),
   BigZoom(true),
   LandableReachable(false),
   fpsTime0(0),
   MapFullScreen(false),
   askFullScreen(false),
   askVisibilityScan(false),
   user_asked_redraw(false)
{

}


int timestats_av = 0;
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
  mutexFlightData.Lock();
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

  mutexFlightData.Unlock();
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
  mutexTerrainData.Lock();
  SetTopologyBounds(*this, MapRect, do_force);
  mutexTerrainData.Unlock();

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


void MapWindow::DrawThreadLoop(bool first_time) {
  if (!dirtyEvent.test() && !first_time) {
    // redraw old screen, must have been a request for fast refresh
    get_canvas().copy(draw_canvas);
    return;
  }

  mutexRun.Lock(); // take control

  dirtyEvent.reset();

  UpdateInfo(&GPS_INFO, &CALCULATED_INFO);

  if (BigZoom) {
    // quickly draw zoom level on top
    DrawMapScale(get_canvas(), MapRect, true);
  }

  if (askFullScreen != MapFullScreen) {
    ToggleFullScreenStart();
  }

  if (gauge_flarm != NULL)
    gauge_flarm->Render(&DrawInfo);

  RenderMapWindow(draw_canvas, MapRect);

  if (!first_time) {
    get_canvas().copy(draw_canvas);
    update(MapRect);
  }

  UpdateTimeStats(false);
  UpdateCaches(first_time);

  mutexRun.Unlock(); // release control
}


void MapWindow::DrawThreadInitialise(void) {
  // initialise other systems
  InitialiseScaleList(); // should really be done before the thread
			 // has started, so it happens from main thread

  // set main rectangles
  MapRectBig = get_client_rect();
  MapRectSmall = MapRect;
  MapRect = MapRectSmall;

  UpdateTimeStats(true);

  // set initial display mode
  draw_canvas.background_transparent();
  mask_canvas.background_opaque();

  // paint draw window black to start
  draw_canvas.black_pen();
  draw_canvas.rectangle(MapRectBig.left, MapRectBig.top,
			MapRectBig.right, MapRectBig.bottom);

  get_canvas().copy(draw_canvas);

  ////// This is just here to give fully rendered start screen
  UpdateInfo(&GPS_INFO, &CALCULATED_INFO);
  dirtyEvent.trigger();
  UpdateTimeStats(true);
  //////

  RequestMapScale = MapScale;
  UpdateMapScale(); // first call
  ToggleFullScreenStart();
}


DWORD MapWindow::_DrawThread ()
{
  while (!globalRunningEvent.test()) {
    // wait for start
    Sleep(100);
  }

  DrawThreadInitialise();

  DrawThreadLoop(true); // first time draw

  // this is the main drawing loop

  do {
    DrawThreadLoop(false);
    drawTriggerEvent.wait(5000);
  } while (!closeTriggerEvent.test());

  mutexStart.Unlock(); // release lock
  return 0;
}


bool MapWindow::register_class(HINSTANCE hInstance, const TCHAR* szWindowClass) {

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
  wc.hCursor = NULL;
  wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = szWindowClass;

  return (RegisterClass(&wc)!= FALSE);
}

bool MapWindow::checkLabelBlock(const RECT brect) {
  return label_block.check(brect);
}


/////////////////////////////////////////

bool MapWindow::on_resize(unsigned width, unsigned height) {
  StartupStore(TEXT("on_resize %d %d\n"),width,height);

  resize(width, height);
  draw_canvas.resize(width, height);
  buffer_canvas.resize(width, height);
  mask_canvas.resize(width, height);

  SetFontInfoAll(get_canvas());

  // Signal that draw thread can run now
  mutexStart.Lock();
  window_initialised = true;
  mutexStart.Unlock(); // release lock
  return false;
}

bool MapWindow::on_create()
{
  StartupStore(TEXT("on_create\n"));

  draw_canvas.set(get_canvas());
  buffer_canvas.set(get_canvas());
  mask_canvas.set(draw_canvas);
  return true;
}

bool MapWindow::on_destroy()
{
  draw_canvas.reset();
  mask_canvas.reset();
  buffer_canvas.reset();

  PostQuitMessage (0);
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
      Screen2LatLon((int)x, (int)y, mouseMovelon, mouseMovelat);
      if (InAATTurnSector(mouseMovelon, mouseMovelat, TargetPanIndex)) {
	// update waypoints so if we drag out of the cylinder, it
	// will remain adjacent to the edge
	Task[TargetPanIndex].AATTargetLat = mouseMovelat;
	Task[TargetPanIndex].AATTargetLon = mouseMovelon;
	TargetDrag_Latitude = mouseMovelat;
	TargetDrag_Longitude = mouseMovelon;
	POINT Pos; Pos.x = x; Pos.y = y;
	DrawBitmapIn(get_canvas(), Pos, MapGfx.hBmpTarget);
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

  // TODO VNT move Screen2LatLon in LBUTTONUP after making sure we
  // really need Xstart and Ystart so we save precious
  // milliseconds waiting for BUTTONUP GetTickCount
  Screen2LatLon(x, y, Xstart, Ystart);
  XstartScreen = x;
  YstartScreen = y;

  mutexTaskData.Lock();
  if (AATEnabled && TargetPan) {
    if (ValidTaskPoint(TargetPanIndex)) {
      POINT tscreen;
      LatLon2Screen(Task[TargetPanIndex].AATTargetLon,
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
  Screen2LatLon(x, y, Xlat, Ylat);

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
	if (PopupNearestWaypointDetails(Xstart, Ystart, 500*MapScale, false)) {
	  return true;
	}
      } else {
	if (PopupInteriorAirspaceDetails(Xstart, Ystart)) {
	  return true;
	}
      }
    } else {
      if(dwInterval < AIRSPACECLICK) { // original and untouched interval
	if (PopupNearestWaypointDetails(Xstart, Ystart, 500*MapScale, false)) {
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
  InterfaceTimeoutReset();
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
  return true;
}

#include "Screen/PaintCanvas.hpp"

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
