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

#include "MapWindow.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/WindowCanvas.hpp"
#include "Screen/Layout.hpp"
#include "Topology/TopologyStore.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Gauge/GaugeCDI.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Math/Earth.hpp"

#include <tchar.h>

/**
 * Constructor of the MapWindow class
 */
MapWindow::MapWindow()
  :way_points(NULL),
   topology(NULL),
   terrain(NULL),
   terrain_center(Angle::native(fixed_zero), Angle::native(fixed_zero)),
   weather(NULL),
   airspace_database(NULL), airspace_warnings(NULL), task(NULL),
   marks(NULL), 
   cdi(NULL),
   ui_generation(1), buffer_generation(0),
   scale_buffer(0),
   TargetDrag_Location(GeoPoint(Angle::native(fixed_zero),
                                Angle::native(fixed_zero))),
   TargetDrag_State(0)
{
  zoomclimb.CruiseMapScale = fixed_ten;
  zoomclimb.ClimbMapScale = fixed(0.25);
  zoomclimb.last_isclimb = false;
  zoomclimb.last_targetpan = false;
}

/**
 * Destructor of the MapWindow class
 */
MapWindow::~MapWindow()
{
  delete cdi;
}

void
MapWindow::set(ContainerWindow &parent, const RECT &rc)
{
  WindowStyle style;
  style.enable_double_clicks();
  DoubleBufferWindow::set(parent, _T("XCSoarMap"), rc.left, rc.top,
                          rc.right - rc.left, rc.bottom - rc.top,
                          style);

  // initialize other systems
  visible_projection.InitialiseScaleList(SettingsMap(), get_client_rect());
  buffer_projection = visible_projection;

  cdi = new GaugeCDI(parent); /* XXX better attach to "this"? */
}

/**
 * Copies the given basic and calculated info to the MapWindowBlackboard
 * and reads the Settings from the DeviceBlackboard.
 * @param nmea_info Basic info
 * @param derived_info Calculated info
 * @param settings_computer Computer settings to exchange
 * @param settings_map Map settings to exchange
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
  visible_projection.ExchangeBlackboard(Calculated(), SettingsMap());
}

void
MapWindow::UpdateTopology()
{
  if (topology != NULL && SettingsMap().EnableTopology)
    topology->ScanVisibility(visible_projection);
}

void
MapWindow::UpdateTerrain()
{
  if (terrain == NULL ||
      Distance(terrain_center, visible_projection.GetPanLocation()) < fixed(1000))
    return;

  // always service terrain even if it's not used by the map,
  // because it's used by other calculations
  RasterTerrain::ExclusiveLease lease(*terrain);
  lease->SetViewCenter(visible_projection.GetPanLocation());
  terrain_center = visible_projection.GetPanLocation();
}

void
MapWindow::UpdateWeather()
{
  // always service weather even if it's not used by the map,
  // because it's potentially used by other calculations

  if (weather != NULL) {
    weather->Reload((int)Basic().Time);
    weather->SetViewCenter(visible_projection.GetPanLocation());
  }
}

/**
 * Handles the drawing of the moving map and is called by the DrawThread
 */
void
MapWindow::DrawThreadLoop(void)
{
  unsigned render_generation = ui_generation;

  // Start the drawing timer (for drawing time calculation)
  StartTimer();

  //  static PeriodClock mclock;
  //  printf("draw %d\n",mclock.elapsed());
  //  mclock.update();

  // Render the moving map
  Render(get_canvas(), get_client_rect());

  // Stop the drawing timer and calculate drawing time
  StopTimer();

  /* save the generation number which was active when rendering had
     begun */
  buffer_projection = render_projection;
  buffer_generation = render_generation;

  flip();
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
  wc.cbWndExtra = 0 ;
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
  return !_tcscmp(name, _T("XCSoarMap"));
}
#endif /* WIN32 */

void
MapWindow::set_topology(TopologyStore *_topology)
{
  topology = _topology;
  m_background.reset();
}

void
MapWindow::set_terrain(RasterTerrain *_terrain)
{
  terrain = _terrain;
  terrain_center = GeoPoint(Angle::native(fixed_zero),
                            Angle::native(fixed_zero));
  m_background.set_terrain(_terrain);
}

void
MapWindow::set_weather(RasterWeather *_weather)
{
  weather = _weather;
  m_background.set_weather(_weather);
}

void
MapWindow::SwitchZoomClimb(void)
{
  bool isclimb = (visible_projection.GetDisplayMode() == dmCircling);

  bool my_target_pan = SettingsMap().TargetPan;

  if (my_target_pan != zoomclimb.last_targetpan) {
    if (my_target_pan) {
      // save starting values
      if (isclimb)
        zoomclimb.ClimbMapScale = visible_projection.GetMapScaleUser();
      else
        zoomclimb.CruiseMapScale = visible_projection.GetMapScaleUser();
    } else {
      // restore scales
      if (isclimb)
        visible_projection.RequestMapScale(zoomclimb.ClimbMapScale, SettingsMap());
      else
        visible_projection.RequestMapScale(zoomclimb.CruiseMapScale, SettingsMap());
    }
    zoomclimb.last_targetpan = my_target_pan;
    return;
  }

  if (!my_target_pan && SettingsMap().CircleZoom) {
    if (isclimb != zoomclimb.last_isclimb) {
      if (isclimb) {
        // save cruise scale
        zoomclimb.CruiseMapScale = visible_projection.GetMapScaleUser();
        // switch to climb scale
        visible_projection.RequestMapScale(zoomclimb.ClimbMapScale, SettingsMap());
      } else {
        // leaving climb
        // save cruise scale
        zoomclimb.ClimbMapScale = visible_projection.GetMapScaleUser();
        visible_projection.RequestMapScale(zoomclimb.CruiseMapScale, SettingsMap());
        // switch to climb scale
      }

      zoomclimb.last_isclimb = isclimb;
    }
  }
}

static DisplayMode_t
ApplyUserForceDisplayMode(DisplayMode_t current,
                          const SETTINGS_MAP &settings_map,
                          const DERIVED_INFO &derived_info)
{
  if (settings_map.UserForceDisplayMode != dmNone)
    return settings_map.UserForceDisplayMode;
  else if (derived_info.Circling)
    return dmCircling;
  else if (derived_info.task_stats.flight_mode_final_glide)
    return dmFinalGlide;
  else
    return dmCruise;
}

void
MapWindow::ApplyScreenSize()
{
  DisplayMode_t lastDisplayMode = visible_projection.GetDisplayMode();
  DisplayMode_t newDisplayMode =
    ApplyUserForceDisplayMode(lastDisplayMode, SettingsMap(), Calculated());

  if (newDisplayMode != lastDisplayMode) {
    visible_projection.SetDisplayMode(newDisplayMode);
    SwitchZoomClimb();
  }
}


GlidePolar 
MapWindow::get_glide_polar() const
{
  return task != NULL ? task->get_glide_polar() : GlidePolar(fixed_zero);
}
