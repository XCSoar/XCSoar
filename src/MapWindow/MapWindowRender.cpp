/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Look/MapLook.hpp"
#include "Markers/ProtectedMarkers.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Units/Units.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/MarkerRenderer.hpp"

#ifdef HAVE_NOAA
#include "Weather/NOAAStore.hpp"
#endif

void
MapWindow::RenderTerrain(Canvas &canvas)
{
  background.SetShadingAngle(render_projection, GetMapSettings().terrain,
                             Calculated());
  background.Draw(canvas, render_projection, GetMapSettings().terrain);
}

void
MapWindow::RenderTopography(Canvas &canvas)
{
  if (topography_renderer != NULL && GetMapSettings().topography_enabled)
    topography_renderer->Draw(canvas, render_projection);
}

void
MapWindow::RenderTopographyLabels(Canvas &canvas)
{
  if (topography_renderer != NULL && GetMapSettings().topography_enabled)
    topography_renderer->DrawLabels(canvas, render_projection, label_block);
}

void
MapWindow::RenderFinalGlideShading(Canvas &canvas)
{
  if (terrain != NULL &&
      Calculated().terrain_valid)
      DrawTerrainAbove(canvas);
}

void
MapWindow::RenderAirspace(Canvas &canvas)
{
  if (GetMapSettings().airspace.enable)
    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas, stencil_canvas,
#endif
                           render_projection,
                           Basic(), Calculated(),
                           GetComputerSettings().airspace,
                           GetMapSettings().airspace);
}

void
MapWindow::RenderMarkers(Canvas &canvas)
{
  if (marks != NULL &&
      render_projection.GetMapScale() <= fixed(30000))
    ::RenderMarkers(canvas, render_projection, look.marker, *marks);
}

void
MapWindow::RenderNOAAStations(Canvas &canvas)
{
#ifdef HAVE_NOAA
  if (noaa_store == NULL)
    return;

  RasterPoint pt;
  for (auto it = noaa_store->begin(), end = noaa_store->end(); it != end; ++it)
    if (it->parsed_metar_available && it->parsed_metar.location_available &&
        render_projection.GeoToScreenIfVisible(it->parsed_metar.location, pt))
      look.noaa.icon.Draw(canvas, pt);
#endif
}

void
MapWindow::RenderGlide(Canvas &canvas)
{
  // draw red cross on glide through terrain marker
  if (Calculated().terrain_valid)
    DrawGlideThroughTerrain(canvas);
}

void
MapWindow::Render(Canvas &canvas, const PixelRect &rc)
{ 
  const NMEAInfo &basic = Basic();

  render_projection = visible_projection;

  // Calculate screen position of the aircraft
  RasterPoint aircraft_pos{0,0};
  if (basic.location_available)
      aircraft_pos = render_projection.GeoToScreen(basic.location);

  // reset label over-write preventer
  label_block.reset();

  // Render terrain, groundline and topography
  draw_sw.Mark("RenderTerrain");
  RenderTerrain(canvas);

  draw_sw.Mark("RenderTopography");
  RenderTopography(canvas);

  draw_sw.Mark("RenderFinalGlideShading");
  RenderFinalGlideShading(canvas);

  // Render track bearing (ground track)
  draw_sw.Mark("DrawTrackBearing");
  DrawTrackBearing(canvas, aircraft_pos);

  // Render airspace
  draw_sw.Mark("RenderAirspace");
  RenderAirspace(canvas);

  // Render task, waypoints
  draw_sw.Mark("DrawContest");
  DrawContest(canvas);

  draw_sw.Mark("DrawTask");
  DrawTask(canvas);

  draw_sw.Mark("DrawWaypoints");
  DrawWaypoints(canvas);

  draw_sw.Mark("DrawNOAAStations");
  RenderNOAAStations(canvas);

  draw_sw.Mark("RenderMisc1");
  // Render weather/terrain max/min values
  DrawTaskOffTrackIndicator(canvas);

  // Render the snail trail
  if (basic.location_available)
    RenderTrail(canvas, aircraft_pos);

  RenderMarkers(canvas);

  // Render estimate of thermal location
  DrawThermalEstimate(canvas);

  // Render topography on top of airspace, to keep the text readable
  draw_sw.Mark("RenderTopographyLabels");
  RenderTopographyLabels(canvas);

  // Render glide through terrain range
  draw_sw.Mark("RenderGlide");
  RenderGlide(canvas);

  draw_sw.Mark("RenderMisc2");

  DrawBestCruiseTrack(canvas, aircraft_pos);

  airspace_renderer.DrawIntersections(canvas, render_projection);

  // Draw wind vector at aircraft
  if (basic.location_available)
    DrawWind(canvas, aircraft_pos, rc);

  // Draw traffic

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  DrawSkyLinesTraffic(canvas);
#endif

  DrawTeammate(canvas);

  if (basic.location_available)
    DrawFLARMTraffic(canvas, aircraft_pos);

  // Finally, draw you!
  if (basic.location_available)
    AircraftRenderer::Draw(canvas, GetMapSettings(), look.aircraft,
                           basic.attitude.heading - render_projection.GetScreenAngle(),
                           aircraft_pos);

  // Render compass
  DrawCompass(canvas, rc);
}
