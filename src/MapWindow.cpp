/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Screen/Graphics.hpp"
#include "Waypoint/Waypoints.hpp"
#include "TopologyStore.h"
#include "RasterTerrain.h"
#include "TerrainRenderer.h"
#include "RasterWeather.h"
#include "Gauge/GaugeCDI.hpp"
#include "Protection.hpp"
#include "SnailTrail.hpp"

#include <tchar.h>

// Initialization

ScreenGraphics MapGfx;

/**
 * Constructor of the MapWindow class
 */
MapWindow::MapWindow()
  :MapWindowProjection(),
   way_points(NULL),
   topology(NULL), terrain(NULL), weather(NULL), terrain_renderer(NULL),
   airspace_database(NULL), task(NULL),
   marks(NULL), snail_trail(NULL), 
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

/**
 * Destructor of the MapWindow class
 */
MapWindow::~MapWindow()
{
  if (cdi != NULL)
    delete cdi;
  if (terrain_renderer != NULL)
    delete terrain_renderer;

  reset();
}

void
MapWindow::set(ContainerWindow &parent, const RECT _MapRectSmall,
    const RECT _MapRectBig)
{
  MapRectSmall = _MapRectSmall;
  MapRect = MapRectBig = _MapRectBig;

  MaskedPaintWindow::set(parent, _T("XCSoarMap"), MapRect.left, MapRect.top,
                         MapRect.right - MapRect.left,
                         MapRect.bottom - MapRect.top);

  // initialize other systems
  InitialiseScaleList(SettingsMap());

  cdi = new GaugeCDI(parent); /* XXX better attach to "this"? */
}

/**
 * Triggers the drawTrigger and is called by
 * the on_mouse_up event in case of panning
 */
void
MapWindow::RefreshMap()
{
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

/**
 * Copies the given basic and calculated info to the MapWindowBlackboard
 * and reads the Settings from the DeviceBlackboard.
 * @param nmea_info Basic info
 * @param derived_info Calculated info
 */
void
MapWindow::ReadBlackboard(const NMEA_INFO &nmea_info,
                          const DERIVED_INFO &derived_info,
                          const SETTINGS_COMPUTER &settings_computer,
                          const SETTINGS_MAP &settings_map)
{
  MapWindowBlackboard::ReadBlackboard(nmea_info, derived_info);
  ReadSettingsComputer(settings_computer);
  ReadSettingsMap(settings_map);
}

void
MapWindow::UpdateProjection()
{
  ApplyScreenSize();
  MapWindowProjection::ExchangeBlackboard(Calculated(), SettingsMap());
}

typedef struct {
  DWORD time_last;
  bool dirty;
} MapIdleTrigger;

/**
 * This idle function allows progressive scanning of visibility etc
 */
bool
MapWindow::Idle(const bool do_force)
{
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

    if (topology != NULL)
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
      if (topology != NULL && topology_idle.dirty) {
        if (SettingsMap().EnableTopology) {
          topology_idle.dirty =
            topology->ScanVisibility(*this, *getSmartBounds(), do_force);
        } else {
          topology_idle.dirty = false;
        }
        break;
      }
    case 2:
      if (terrain != NULL && terrain_idle.dirty) {
        terrain->ServiceTerrainCenter(Basic().Location);
        terrain->ServiceCache();

        if (!do_force) {
          // JMW this currently isn't working with the smart bounds
          terrain_idle.dirty = false;
        }
        break;
      }
    case 3:
      if (weather != NULL && rasp_idle.dirty) {
        weather->SetViewCenter(Basic().Location);
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

/**
 * Handles the drawing of the moving map and is called by the DrawThread
 */
void
MapWindow::DrawThreadLoop(void)
{
  // Start the drawing timer (for drawing time calculation)
  StartTimer();

  //  static PeriodClock mclock;
  //  printf("draw %d\n",mclock.elapsed());
  //  mclock.update();

  if (BigZoom) {
    // quickly draw zoom level on top
    DrawMapScale(get_canvas(), MapRect, true);
  }

  // Render the moving map
  Render(draw_canvas, MapRect);

  // Copy the rendered map to the drawing canvas
  invalidate();

  // Stop the drawing timer and calculate drawing time
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

void
MapWindow::set_topology(TopologyStore *_topology)
{
  topology = _topology;
}

void
MapWindow::set_terrain(RasterTerrain *_terrain)
{
  terrain = _terrain;

  if (terrain_renderer != NULL) {
    delete terrain_renderer;
    terrain_renderer = NULL;
  }
}

void
MapWindow::set_weather(RasterWeather *_weather)
{
  weather = _weather;

  if (terrain_renderer != NULL) {
    delete terrain_renderer;
    terrain_renderer = NULL;
  }
}

bool MapWindow::checkLabelBlock(const RECT brect) {
  return label_block.check(brect);
}

void MapWindow::ScanVisibility(rectObj *bounds_active) {
  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)

  if (snail_trail != NULL)
    snail_trail->ScanVisibility(bounds_active);

#ifdef OLD_TASK
  ScanVisibilityAirspace(bounds_active);
#endif
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
