/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Math/Earth.hpp"
#include "Units.hpp"

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
   marks(NULL)
#ifndef ENABLE_OPENGL
  , ui_generation(1), buffer_generation(0),
   scale_buffer(0)
#endif
{}

void
MapWindow::set(ContainerWindow &parent, const RECT &rc)
{
  WindowStyle style;
  style.enable_double_clicks();
  DoubleBufferWindow::set(parent, _T("XCSoarMap"), rc.left, rc.top,
                          rc.right - rc.left, rc.bottom - rc.top,
                          style);

  // initialize other systems
  visible_projection.Initialize(SettingsMap(), get_client_rect());

#ifndef ENABLE_OPENGL
  buffer_projection = visible_projection;
#endif
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
  UpdateMapScale(Calculated(), SettingsMap());
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
      Distance(terrain_center, visible_projection.GetGeoLocation()) < fixed(1000))
    return;

  // always service terrain even if it's not used by the map,
  // because it's used by other calculations
  RasterTerrain::ExclusiveLease lease(*terrain);
  lease->SetViewCenter(visible_projection.GetGeoLocation());
  terrain_center = visible_projection.GetGeoLocation();
}

void
MapWindow::UpdateWeather()
{
  // always service weather even if it's not used by the map,
  // because it's potentially used by other calculations

  if (weather != NULL) {
    weather->Reload((int)Basic().Time);
    weather->SetViewCenter(visible_projection.GetGeoLocation());
  }
}

#ifndef ENABLE_OPENGL
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
#endif

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

GlidePolar 
MapWindow::get_glide_polar() const
{
  return task != NULL ? task->get_glide_polar() : GlidePolar(fixed_zero);
}

bool
MapWindow::IsOriginCentered(const DisplayOrientation_t orientation,
                                      const DisplayMode_t mode)
{
  return (orientation == NORTHUP
      || (orientation == NORTHTRACK
          && mode != dmCircling)
      || ((orientation == NORTHCIRCLE
           || orientation == TRACKCIRCLE)
          && mode == dmCircling));
}

void
MapWindow::UpdateScreenAngle(const NMEA_INFO &basic,
    const DERIVED_INFO &derived, const SETTINGS_MAP &settings)
{
  if (settings.TargetPan &&
      derived.common_stats.active_taskpoint_index != settings.TargetPanIndex) {
    visible_projection.SetScreenAngle(Angle::native(fixed_zero));
    return;
  }

  if (IsOriginCentered(settings.DisplayOrientation,
                       visible_projection.GetDisplayMode())) {
    if (settings.DisplayOrientation == TRACKCIRCLE)
      visible_projection.SetScreenAngle(derived.task_stats.current_leg.
                                        solution_remaining.Vector.Bearing);
    else
      visible_projection.SetScreenAngle(Angle::native(fixed_zero));
  } else {
    // normal, glider forward
    visible_projection.SetScreenAngle(basic.TrackBearing);
  }
}

void
MapWindow::UpdateMapScale(const DERIVED_INFO &derived,
                          const SETTINGS_MAP &settings_map)
{
  static bool TargetPanLast = false;
  static fixed TargetPanUnZoom = fixed_one;

  fixed wpd;
  if (settings_map.TargetPan)
    wpd = settings_map.TargetZoomDistance;
  else
    wpd = derived.AutoZoomDistance;

  if (settings_map.TargetPan) {
    if (!TargetPanLast) { // just entered targetpan so save zoom
      TargetPanLast = true;
      TargetPanUnZoom = visible_projection.GetMapScale();
    }
    // set scale exactly so that waypoint distance is the zoom factor
    // across the screen

    wpd = max(fixed_int_constant(50), min(fixed_int_constant(160000), wpd / 4));
    visible_projection.RequestMapScale(wpd);
    return;
  }

  if (settings_map.AutoZoom && positive(wpd)) {
    fixed AutoZoomFactor =
        IsOriginCentered(settings_map.DisplayOrientation,
                         visible_projection.GetDisplayMode()) ?
        fixed(2.5) : fixed_four;

    if (wpd < AutoZoomFactor * visible_projection.GetMapScale()) {
      // waypoint is too close, so zoom in

      // set scale exactly so that waypoint distance is the zoom factor
      // across the screen
      wpd = max(fixed_int_constant(440), min(fixed_int_constant(160000),
                                             wpd / AutoZoomFactor));

      visible_projection.RequestMapScale(wpd);
    }
  } else if (TargetPanLast) {
    visible_projection.RequestMapScale(TargetPanUnZoom);
  }

  if (!settings_map.TargetPan && TargetPanLast)
    TargetPanLast = false;
}

void
MapWindow::Update(const RECT rc, const NMEA_INFO &basic,
                  const SETTINGS_MAP &settings_map)
{
  visible_projection.SetMapRect(rc);

  if (IsOriginCentered(settings_map.DisplayOrientation,
                       visible_projection.GetDisplayMode()) ||
      settings_map.EnablePan)
    visible_projection.SetScreenOrigin((rc.left + rc.right) / 2,
                                       (rc.bottom + rc.top) / 2);
  else
    visible_projection.SetScreenOrigin(
        (rc.left + rc.right) / 2,
        ((rc.top - rc.bottom) * settings_map.GliderScreenPosition / 100) + rc.bottom);

  if (settings_map.EnablePan)
    visible_projection.SetGeoLocation(settings_map.PanLocation);
  else
    // Pan is off
    visible_projection.SetGeoLocation(basic.Location);

  visible_projection.UpdateScreenBounds();
}

