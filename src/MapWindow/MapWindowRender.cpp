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
#include "ProtectedMarkers.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Units/Units.hpp"
#include "Renderer/AircraftRenderer.hpp"

void
MapWindow::RenderTerrain(Canvas &canvas)
{
  if (SettingsMap().terrain.slope_shading == sstWind)
    m_background.sun_from_wind(render_projection, Calculated().wind);
  else
    m_background.set_sun_angle(render_projection,
                               (Basic().location_available &&
                                SettingsMap().terrain.slope_shading == sstSun) ?
                               Calculated().sun_azimuth :
                               Angle::Degrees(fixed(-45.0)));

  m_background.Draw(canvas, render_projection, SettingsMap().terrain);
}

void
MapWindow::RenderTopography(Canvas &canvas)
{
  if (topography_renderer != NULL && SettingsMap().EnableTopography)
    topography_renderer->Draw(canvas, render_projection);
}

void
MapWindow::RenderTopographyLabels(Canvas &canvas)
{
  if (topography_renderer != NULL && SettingsMap().EnableTopography)
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
  if (SettingsMap().airspace.enable)
    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas, stencil_canvas,
#endif
                           render_projection,
                           Basic(), Calculated(),
                           SettingsComputer(), SettingsMap());
}

void
MapWindow::RenderMarkers(Canvas &canvas)
{
  if (marks != NULL &&
      render_projection.GetMapScale() <= fixed_int_constant(30000))
    marks->Draw(canvas, render_projection, marker_look);
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
  render_projection = visible_projection;

  // Calculate screen position of the aircraft
  const RasterPoint aircraft_pos =
      render_projection.GeoToScreen(Basic().location);

  // reset label over-write preventer
  label_block.reset();

  // Render terrain, groundline and topography
  draw_sw.Mark(_T("RenderTerrain"));
  RenderTerrain(canvas);

  draw_sw.Mark(_T("RenderTopography"));
  RenderTopography(canvas);

  draw_sw.Mark(_T("RenderFinalGlideShading"));
  RenderFinalGlideShading(canvas);

  // Render track bearing (ground track)
  draw_sw.Mark(_T("DrawTrackBearing"));
  DrawTrackBearing(canvas, aircraft_pos);

  // Render airspace
  draw_sw.Mark(_T("RenderAirspace"));
  RenderAirspace(canvas);

  // Render task, waypoints
  draw_sw.Mark(_T("DrawTask"));
  DrawTask(canvas);

  draw_sw.Mark(_T("DrawWaypoints"));
  DrawWaypoints(canvas);

  draw_sw.Mark(_T("RenderMisc1"));
  // Render weather/terrain max/min values
  if (!m_background.DrawSpotHeights(canvas, label_block))
    DrawTaskOffTrackIndicator(canvas);

  // Render the snail trail
  RenderTrail(canvas, aircraft_pos);

  RenderMarkers(canvas);

  // Render estimate of thermal location
  DrawThermalEstimate(canvas);

  // Render topography on top of airspace, to keep the text readable
  draw_sw.Mark(_T("RenderTopographyLabels"));
  RenderTopographyLabels(canvas);

  // Render glide through terrain range
  draw_sw.Mark(_T("RenderGlide"));
  RenderGlide(canvas);

  draw_sw.Mark(_T("RenderMisc2"));

  DrawBestCruiseTrack(canvas, aircraft_pos);

  airspace_renderer.DrawIntersections(canvas, render_projection);

  // Draw wind vector at aircraft
  DrawWind(canvas, aircraft_pos, rc);

  // Draw traffic
  DrawTeammate(canvas);
  DrawFLARMTraffic(canvas, aircraft_pos);

  // Finally, draw you!
  if (Basic().connected)
    AircraftRenderer::Draw(canvas, SettingsMap(), aircraft_look,
                           Calculated().heading - render_projection.GetScreenAngle(),
                           aircraft_pos);

  // Render compass
  DrawCompass(canvas, rc);
}
