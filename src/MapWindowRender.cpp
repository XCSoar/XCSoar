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
#include "Marks.hpp"
#include "Topology/TopologyRenderer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Units.hpp"

void
MapWindow::RenderTerrain(Canvas &canvas)
{
  m_background.sun_from_wind(render_projection, Basic().wind);
  m_background.Draw(canvas, render_projection, SettingsMap());
}

void
MapWindow::RenderTopology(Canvas &canvas)
{
  if (topology_renderer != NULL && SettingsMap().EnableTopology)
    topology_renderer->Draw(canvas, render_projection);
}

void
MapWindow::RenderTopologyLabels(Canvas &canvas)
{
  if (topology_renderer != NULL && SettingsMap().EnableTopology)
    topology_renderer->DrawLabels(canvas, render_projection, label_block,
                                  SettingsMap());
}

void
MapWindow::RenderFinalGlideShading(Canvas &canvas)
{
  if (terrain != NULL &&
      SettingsComputer().FinalGlideTerrain == 2 &&
      Calculated().TerrainValid)
      DrawTerrainAbove(canvas);
}

void
MapWindow::RenderAirspace(Canvas &canvas)
{
  if (SettingsMap().EnableAirspace)
    DrawAirspace(canvas);
}

void
MapWindow::RenderMarks(Canvas &canvas)
{
  if (marks != NULL &&
      render_projection.GetMapScale() <= fixed_int_constant(30000))
    marks->Draw(canvas, render_projection);
}

void
MapWindow::RenderGlide(Canvas &canvas)
{
  // draw red cross on glide through terrain marker
  if (Calculated().TerrainValid)
    DrawGlideThroughTerrain(canvas);
}

void
MapWindow::RenderAirborne(Canvas &canvas, const RECT &rc,
                          const RasterPoint aircraft_pos)
{
  // Draw wind vector at aircraft
  if (!SettingsMap().EnablePan || SettingsMap().TargetPan)
    DrawWind(canvas, render_projection.GetScreenOrigin(), rc);

  // Draw traffic
  DrawTeammate(canvas);
  DrawFLARMTraffic(canvas, aircraft_pos);

  // Finally, draw you!
  if (Basic().gps.Connected)
    DrawAircraft(canvas, aircraft_pos);
}

void
MapWindow::RenderSymbology_lower(Canvas &canvas,
                                 const RasterPoint aircraft_pos)
{
  if (Basic().gps.Connected)
    DrawBestCruiseTrack(canvas, aircraft_pos);

  DrawAirspaceIntersections(canvas);
}

void
MapWindow::Render(Canvas &canvas, const RECT &rc)
{ 
  Update();
  render_projection = visible_projection;

  // Calculate screen position of the aircraft
  const RasterPoint aircraft_pos =
      render_projection.GeoToScreen(Basic().Location);

  // Calculate screen positions of the final glide groundline
  CalculateScreenPositionsGroundline();

  // reset label over-write preventer
  label_block.reset();

  // Render terrain, groundline and topology
  RenderTerrain(canvas);
  RenderTopology(canvas);
  RenderFinalGlideShading(canvas);

  // Render airspace
  RenderAirspace(canvas);

  // Render the snail trail
  /// @todo trail should be drawn above task shaded sections
  DrawTrail(canvas, aircraft_pos);

  DrawThermalEstimate(canvas);

  // Render task, waypoints and marks
  DrawTask(canvas);
  DrawWaypoints(canvas);
  DrawTaskOffTrackIndicator(canvas);
  RenderMarks(canvas);

  // Render topology on top of airspace, to keep the text readable
  RenderTopologyLabels(canvas);

  // Render glide through terrain range
  RenderGlide(canvas);

  // Render weather/terrain max/min values
  m_background.DrawSpotHeights(canvas, label_block);

  // Render lower symbology
  RenderSymbology_lower(canvas, aircraft_pos);

  // Render aircraft symbol (and FLARM traffic)
  RenderAirborne(canvas, rc, aircraft_pos);
  
  // Render compass
  DrawCompass(canvas, rc);
}
