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
#include "Settings.hpp"
#include "SettingsUser.hpp"
#include "Audio/VarioSound.h"
#include "InputEvents.h"
#include "Screen/Graphics.hpp"
#include "Screen/Util.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/LabelBlock.hpp"
#include "Compatibility/gdi.h"
#include "TopologyStore.h"
#include "InfoBoxLayout.h"
#include "RasterTerrain.h"
#include "Message.h"
#include "Calculations.h" // TODO danger! for InAATTurnSector
#include "RasterWeather.h"
#include "options.h" /* for DEBUG_VIRTUALKEYS */
#include "Defines.h" /* for DEBUG_VIRTUALKEYS */
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

///////////////////////////////// Settings

DisplayMode_t DisplayMode = dmCruise;
// 12 is number of airspace types
int      iAirspaceBrush[AIRSPACECLASSCOUNT] =
  {2,0,0,0,3,3,3,3,0,3,2,3,3,3};
int      iAirspaceColour[AIRSPACECLASSCOUNT] =
  {5,0,0,10,0,0,10,2,0,10,9,3,7,7};
int      iAirspaceMode[AIRSPACECLASSCOUNT] =
  {0,0,0,0,0,0,0,0,0,0,0,1,1,0};


///////////////////////////////// Initialisation

ScreenGraphics MapGfx;

//////////////////////////


MapWindow::MapWindow()
  :MapWindowProjection(),
   cdi(NULL),
   TargetDrag_State(0),
   TargetDrag_Latitude(0),
   TargetDrag_Longitude(0),
   BigZoom(true),
   FullScreen(false)
{

}

MapWindow::~MapWindow()
{
  if (cdi != NULL)
    delete cdi;
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

void MapWindow::ReadBlackboard(const NMEA_INFO &nmea_info,
			       const DERIVED_INFO &derived_info) {
  ScopeLock protect(mutexFlightData);
  MapWindowBlackboard::ReadBlackboard(nmea_info, derived_info);
  ReadSettingsComputer(device_blackboard.SettingsComputer());
  ReadSettingsMap(device_blackboard.SettingsMap());
  ApplyScreenSize();

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
      if (SettingsMap().EnableTopology) {
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
      
      if (!do_force) {
	// JMW this currently isn't working with the smart bounds
	terrain_idle.dirty = false;
      }
      continue;
    }

    if (rasp_idle.dirty) {
      RASP.SetViewCenter(Basic().Latitude,
			 Basic().Longitude);
      if (!do_force) {
	// JMW this currently isn't working with the smart bounds
	rasp_idle.dirty = false;
      }
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


void MapWindow::ExchangeBlackboard(void) 
{
  ReadBlackboard(device_blackboard.Basic(), device_blackboard.Calculated());
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

void MapWindow::on_paint(Canvas& _canvas) {
  mutexBuffer.Lock();
  _canvas.copy(draw_canvas);
  mutexBuffer.Unlock();
}

bool
MapWindow::on_setfocus()
{
  MaskedPaintWindow::on_setfocus();

  return true;
}

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
