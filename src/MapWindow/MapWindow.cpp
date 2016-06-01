/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "OverlayBitmap.hpp"
#include "Look/MapLook.hpp"
#include "Topography/CachedTopographyRenderer.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Weather/Rasp/RaspRenderer.hpp"
#include "Computer/GlideComputer.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

/**
 * Constructor of the MapWindow class
 */
MapWindow::MapWindow(const MapLook &_look,
                     const TrafficLook &_traffic_look)
  :look(_look),
   traffic_look(_traffic_look),
   waypoint_renderer(nullptr, look.waypoint),
   airspace_renderer(look.airspace),
   airspace_label_renderer(look.airspace),
   trail_renderer(look.trail) {}

MapWindow::~MapWindow()
{
  Destroy();

  delete topography_renderer;
}

#ifdef ENABLE_OPENGL

void
MapWindow::SetOverlay(std::unique_ptr<MapOverlay> &&_overlay)
{
  overlay = std::move(_overlay);
}

#endif

void
MapWindow::SetGlideComputer(GlideComputer *_gc)
{
  glide_computer = _gc;
  airspace_renderer.SetAirspaceWarnings(glide_computer != nullptr
                                        ? &glide_computer->GetAirspaceWarnings()
                                        : nullptr);
}

void
MapWindow::FlushCaches()
{
  background.Flush();
  if (rasp_renderer)
    rasp_renderer->Flush();
  airspace_renderer.Flush();
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
                          const ComputerSettings &settings_computer,
                          const MapSettings &settings_map)
{
  MapWindowBlackboard::ReadBlackboard(nmea_info, derived_info);
  ReadComputerSettings(settings_computer);
  ReadMapSettings(settings_map);
}

unsigned
MapWindow::UpdateTopography(unsigned max_update)
{
  if (topography != nullptr && GetMapSettings().topography_enabled)
    return topography->ScanVisibility(visible_projection, max_update);
  else
    return 0;
}

bool
MapWindow::UpdateTerrain()
{
  if (terrain == nullptr)
    return false;

  GeoPoint location = visible_projection.GetGeoScreenCenter();
  auto radius = visible_projection.GetScreenWidthMeters() / 2;

  // always service terrain even if it's not used by the map,
  // because it's used by other calculations
  return terrain->UpdateTiles(location, radius);
}

/**
 * Handles the drawing of the moving map and is called by the DrawThread
 */
void
MapWindow::OnPaintBuffer(Canvas &canvas)
{
#ifndef ENABLE_OPENGL
  unsigned render_generation = ui_generation;
#endif

#ifdef ENABLE_OPENGL
  GLCanvasScissor scissor(canvas);
#endif

  // Render the moving map
  Render(canvas, GetClientRect());
  draw_sw.Finish();

#ifndef ENABLE_OPENGL
  /* save the generation number which was active when rendering had
     begun */
  buffer_projection = render_projection;
  buffer_generation = render_generation;
#endif
}

void
MapWindow::SetTopography(TopographyStore *_topography)
{
  topography = _topography;

  delete topography_renderer;
  topography_renderer = topography != nullptr
    ? new CachedTopographyRenderer(*topography, look.topography)
    : nullptr;
}

void
MapWindow::SetTerrain(RasterTerrain *_terrain)
{
  terrain = _terrain;
  background.SetTerrain(_terrain);
}

void
MapWindow::SetRasp(const std::shared_ptr<RaspStore> &_rasp_store)
{
  rasp_renderer.reset();
  rasp_store = _rasp_store;
}
