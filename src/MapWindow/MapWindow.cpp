/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Screen/Layout.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Units/Units.hpp"
#include "Operation.hpp"

#include <tchar.h>

/**
 * Constructor of the MapWindow class
 */
MapWindow::MapWindow(const MapLook &_look,
                     const WaypointLook &_waypoint_look,
                     const AirspaceLook &_airspace_look,
                     const TrailLook &_trail_look,
                     const TaskLook &_task_look,
                     const AircraftLook &_aircraft_look,
                     const TrafficLook &_traffic_look,
                     const MarkerLook &_marker_look)
  :look(_look),
   follow_mode(FOLLOW_SELF),
   way_points(NULL),
   topography(NULL), topography_renderer(NULL),
   terrain(NULL),
   terrain_radius(fixed_zero),
   weather(NULL),
   task_look(_task_look),
   aircraft_look(_aircraft_look),
   traffic_look(_traffic_look),
   marker_look(_marker_look),
   way_point_renderer(NULL, _waypoint_look),
   airspace_renderer(_airspace_look),
   trail_renderer(_trail_look),
   task(NULL), route_planner(NULL), glide_computer(NULL),
   marks(NULL),
   compass_visible(true)
#ifndef ENABLE_OPENGL
   , ui_generation(1), buffer_generation(0),
   scale_buffer(0)
#endif
{}

MapWindow::~MapWindow()
{
  delete topography_renderer;
}

void
MapWindow::set(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle style;
  style.enable_double_clicks();
  DoubleBufferWindow::set(parent, rc.left, rc.top,
                          rc.right - rc.left, rc.bottom - rc.top,
                          style);

  // initialize other systems
  visible_projection.SetMapScale(fixed_int_constant(5000));
  visible_projection.SetScreenOrigin((rc.left + rc.right) / 2,
                                     (rc.bottom + rc.top) / 2);
  visible_projection.UpdateScreenBounds();

#ifndef ENABLE_OPENGL
  buffer_projection = visible_projection;
#endif
}

void
MapWindow::ReadBlackboard(const MoreData &nmea_info,
                          const DerivedInfo &derived_info)
{
  MapWindowBlackboard::ReadBlackboard(nmea_info, derived_info);
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
MapWindow::ReadBlackboard(const MoreData &nmea_info,
                          const DerivedInfo &derived_info,
                          const SETTINGS_COMPUTER &settings_computer,
                          const SETTINGS_MAP &settings_map)
{
  MapWindowBlackboard::ReadBlackboard(nmea_info, derived_info);
  ReadSettingsComputer(settings_computer);
  ReadSettingsMap(settings_map);
}

unsigned
MapWindow::UpdateTopography(unsigned max_update)
{
  if (topography != NULL && SettingsMap().topography_enabled)
    return topography->ScanVisibility(visible_projection, max_update);
  else
    return 0;
}

bool
MapWindow::UpdateTerrain()
{
  if (terrain == NULL)
    return false;

  GeoPoint location = visible_projection.GetGeoScreenCenter();
  fixed radius = visible_projection.GetScreenWidthMeters() / 2;
  if (terrain_radius >= radius &&
      terrain_center.Distance(location) < fixed(1000))
    return false;

  // always service terrain even if it's not used by the map,
  // because it's used by other calculations
  RasterTerrain::ExclusiveLease lease(*terrain);
  lease->SetViewCenter(location, radius);
  if (lease->IsDirty())
    terrain_radius = fixed_zero;
  else {
    terrain_radius = radius;
    terrain_center = location;
  }

  return lease->IsDirty();
}

bool
MapWindow::UpdateWeather()
{
  // always service weather even if it's not used by the map,
  // because it's potentially used by other calculations

  if (weather == NULL)
    return false;

  QuietOperationEnvironment operation;
  weather->Reload((int)Basic().time, operation);
  weather->SetViewCenter(visible_projection.GetGeoScreenCenter(),
                         visible_projection.GetScreenWidthMeters() / 2);
  return weather->IsDirty();
}

/**
 * Handles the drawing of the moving map and is called by the DrawThread
 */
void
MapWindow::on_paint_buffer(Canvas &canvas)
{
#ifndef ENABLE_OPENGL
  unsigned render_generation = ui_generation;

  // Start the drawing timer (for drawing time calculation)
  StartTimer();
#endif

  // Render the moving map
  Render(canvas, get_client_rect());
  draw_sw.Finish();

#ifndef ENABLE_OPENGL
  /* save the generation number which was active when rendering had
     begun */
  buffer_projection = render_projection;
  buffer_generation = render_generation;
#endif
}

void
MapWindow::set_topography(TopographyStore *_topography)
{
  topography = _topography;

  delete topography_renderer;
  topography_renderer = topography != NULL
    ? new TopographyRenderer(*topography)
    : NULL;
}

void
MapWindow::set_terrain(RasterTerrain *_terrain)
{
  terrain = _terrain;
  terrain_center = GeoPoint(Angle::Zero(),
                            Angle::Zero());
  m_background.set_terrain(_terrain);
}

void
MapWindow::set_weather(RasterWeather *_weather)
{
  weather = _weather;
  m_background.set_weather(_weather);
}

void
MapWindow::SetMapScale(const fixed x)
{
  visible_projection.SetMapScale(x);
}
