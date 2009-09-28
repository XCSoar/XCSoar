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
#include "Dialogs.h"
#include "Audio/VarioSound.h"
#include "InputEvents.h"
#include "Screen/Graphics.hpp"
#include "Screen/LabelBlock.hpp"
#include "Compatibility/gdi.h"
#include "TopologyStore.h"
#include "InfoBoxLayout.h"
#include "RasterTerrain.h"
#include "TerrainRenderer.h"
#include "RasterWeather.h"
#include "Gauge/GaugeCDI.hpp"

#ifdef PNA
#include "Asset.hpp"
#endif

#include <assert.h>
#include <windows.h>
#include <math.h>
#include <tchar.h>

#ifdef WINDOWSPC
#include <wingdi.h>
#endif

///////////////////////////////// Initialisation

ScreenGraphics MapGfx;

//////////////////////////


MapWindow::MapWindow()
  :MapWindowProjection(),
   cdi(NULL),
   TargetDrag_State(0),
   BigZoom(true),
   FullScreen(false)
{
  TargetDrag_Location.Latitude = 0;
  TargetDrag_Location.Longitude = 0;
  zoomclimb.CruiseMapScale = 10;
  zoomclimb.ClimbMapScale = 0.25;
  zoomclimb.last_isclimb = false;
  zoomclimb.last_targetpan = false;
}

MapWindow::~MapWindow()
{
  if (cdi != NULL)
    delete cdi;
  if (terrain_renderer != NULL)
    delete terrain_renderer;
}

void
MapWindow::set(ContainerWindow &parent,
               const RECT _MapRectSmall, const RECT _MapRectBig)
{
  MapRectSmall = _MapRectSmall;
  MapRect = MapRectBig = _MapRectBig;

  MaskedPaintWindow::set(parent, _T("XCSoarMap"), MapRect.left, MapRect.top,
                         MapRect.right - MapRect.left,
                         MapRect.bottom - MapRect.top);

  // initialise other systems
  InitialiseScaleList(SettingsMap());

  // set initial display mode
  draw_canvas.background_transparent();

  // paint draw window black to start
  draw_canvas.black_pen();
  draw_canvas.rectangle(MapRectBig.left, MapRectBig.top,
                        MapRectBig.right, MapRectBig.bottom);

  get_canvas().copy(draw_canvas);

  cdi = new GaugeCDI(parent); /* XXX better attach to "this"? */

}

void MapWindow::RefreshMap() {
  MapWindowTimer::InterruptTimer();
  drawTriggerEvent.trigger();
}


void MapWindow::StoreRestoreFullscreen(bool store) {
  /* JMW broken, will need new implementation
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
  */
}


///////////////////////////////////////////////////////////////////////////
#include "DeviceBlackboard.hpp"

void MapWindow::ReadBlackboard(const NMEA_INFO &nmea_info,
			       const DERIVED_INFO &derived_info) {
  ScopeLock protect(mutexBlackboard);
  MapWindowBlackboard::ReadBlackboard(nmea_info, derived_info);
  ReadSettingsComputer(device_blackboard.SettingsComputer());
  ReadSettingsMap(device_blackboard.SettingsMap());
}


void MapWindow::SendBlackboard(const NMEA_INFO &nmea_info,
			       const DERIVED_INFO &derived_info) {
  ScopeLock protect(mutexBlackboard);
  MapWindowProjection::ExchangeBlackboard(nmea_info, derived_info,
					  SettingsMap());
  device_blackboard.ReadMapProjection(*this);
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
  static unsigned robin = 3;

  if (do_force) {
    robin = 3;
    main_idle.dirty = true;
    terrain_idle.dirty = true;
    topology_idle.dirty = true;
    rasp_idle.dirty = true;
    topology->TriggerUpdateCaches(*this);
  }

  do {
    robin = (robin+1)%4;
    switch(robin) {
    case 0:
      // scan main object visibility
      if (main_idle.dirty) {
        main_idle.dirty = false;
        ScanVisibility(getSmartBounds());

        if (do_force) {
          // exit after important object visibilities are scanned
          // this ensures waypoints/airspace are visible after a significant
          // shift of the map
          return true;
        }

        break;
      }
    case 1:
      if (topology_idle.dirty) {
        if (SettingsMap().EnableTopology) {
          topology_idle.dirty = 
            topology->ScanVisibility(*this, *getSmartBounds(), do_force);
        } else {
          topology_idle.dirty = false;
        }
        break;
      }
    case 2:
      if (terrain_idle.dirty) {
        terrain.ServiceTerrainCenter(Basic().Location);
        terrain.ServiceCache();
        
        if (!do_force) {
          // JMW this currently isn't working with the smart bounds
          terrain_idle.dirty = false;
        }
        break;
      }
    case 3:
      if (rasp_idle.dirty) {
        RASP.SetViewCenter(Basic().Location);
        if (!do_force) {
          // JMW this currently isn't working with the smart bounds
          rasp_idle.dirty = false;
        }
        break;
      }
    default:
      break;
    }

  } while (RenderTimeAvailable() && 
	   !drawTriggerEvent.test() &&
	   (still_dirty = 
	      main_idle.dirty 
	    | terrain_idle.dirty 
	    | topology_idle.dirty
	    | rasp_idle.dirty));

  return still_dirty;
}


void MapWindow::ExchangeBlackboard(void) 
{
  ReadBlackboard(device_blackboard.Basic(), device_blackboard.Calculated());
  ApplyScreenSize();
  SendBlackboard(device_blackboard.Basic(), device_blackboard.Calculated());
}

void MapWindow::DrawThreadLoop(void) {
  StartTimer();

  //  static PeriodClock mclock;
  //  printf("draw %d\n",mclock.elapsed());
  //  mclock.update();

  if (BigZoom) {
    // quickly draw zoom level on top
    DrawMapScale(get_canvas(), MapRect, true);
  }

  Render(draw_canvas, MapRect);

  // copy to canvas
  mutexBuffer.Lock();
  get_canvas().copy(draw_canvas);
  mutexBuffer.Unlock();
  update(MapRect);

  StopTimer();
}

bool
MapWindow::register_class(HINSTANCE hInstance)
{
#ifdef ENABLE_SDL
  return true;
#else /* !ENABLE_SDL */
  WNDCLASS wc;

  wc.hInstance = hInstance;
  wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = MapWindow::WndProc;
  wc.cbClsExtra = 0;
#ifdef WINDOWSPC
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
#endif /* !ENABLE_SDL */
}

#ifdef WIN32
bool
MapWindow::identify(HWND hWnd)
{
  TCHAR name[16];
  if (::GetClassName(hWnd, name, sizeof(name)) == 0)
    return false;
  return _tcscmp(name, _T("XCSoarMap"));
}
#endif /* WIN32 */

bool MapWindow::checkLabelBlock(const RECT brect) {
  return label_block.check(brect);
}

#include "Components.hpp"
#include "GlideComputer.hpp"

void MapWindow::ScanVisibility(rectObj *bounds_active) {
  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)

  glide_computer.GetSnailTrail().ScanVisibility(bounds_active);

  ScanVisibilityWaypoints(bounds_active);
  ScanVisibilityAirspace(bounds_active);
}




void MapWindow::SwitchZoomClimb(void) {
  
  bool isclimb = (DisplayMode == dmCircling);

  bool my_target_pan = SettingsMap().TargetPan;

  if (my_target_pan != zoomclimb.last_targetpan) {
    if (my_target_pan) {
      // save starting values
      if (isclimb) {
        zoomclimb.ClimbMapScale = GetMapScaleUser();
      } else {
        zoomclimb.CruiseMapScale = GetMapScaleUser();
      }
    } else {
      // restore scales
      if (isclimb) {
        RequestMapScale(zoomclimb.ClimbMapScale, SettingsMap());
      } else {
        RequestMapScale(zoomclimb.CruiseMapScale, SettingsMap());
      }
      BigZoom = true;
    }
    zoomclimb.last_targetpan = my_target_pan;
    return;
  }

  if (!my_target_pan && SettingsMap().CircleZoom) {
    if (isclimb != zoomclimb.last_isclimb) {
      if (isclimb) {
        // save cruise scale
        zoomclimb.CruiseMapScale = GetMapScaleUser();
        // switch to climb scale
        RequestMapScale(zoomclimb.ClimbMapScale, SettingsMap());
      } else {
        // leaving climb
        // save cruise scale
        zoomclimb.ClimbMapScale = GetMapScaleUser();
        RequestMapScale(zoomclimb.CruiseMapScale, SettingsMap());
        // switch to climb scale
      }
      BigZoom = true;
      zoomclimb.last_isclimb = isclimb;
    }
  }
}


void MapWindow::ApplyScreenSize() {
  FullScreen = SettingsMap().FullScreen;
  // ok, save the state.
  if (FullScreen) {
    SetMapRect(MapRectBig);
  } else {
    SetMapRect(MapRectSmall);
  }

  DisplayMode_t lastDisplayMode = DisplayMode;
  switch (SettingsMap().UserForceDisplayMode) {
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
}


bool MapWindow::draw_masked_bitmap_if_visible(Canvas &canvas,
					      Bitmap &bitmap,
					      const GEOPOINT &loc,
					      unsigned width,
					      unsigned height,
					      POINT *scin)
{
  POINT sc;
  POINT *scp = (scin!=NULL)? (scin):(&sc);

  if (LonLat2ScreenIfVisible(loc, scp)) {
    draw_masked_bitmap(canvas, bitmap, scp->x, scp->y, width, height, true);
    return true;
  }
  return false;
}


