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
#include "Overlay.hpp"
#include "Look/MapLook.hpp"
#include "Weather/Rasp/RaspRenderer.hpp"
#include "Weather/Rasp/RaspCache.hpp"
#include "Topography/CachedTopographyRenderer.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/WaveRenderer.hpp"
#include "Operation/Operation.hpp"
#include "Tracking/SkyLines/Data.hpp"

#ifdef HAVE_NOAA
#include "Weather/NOAAStore.hpp"
#endif

void
MapWindow::RenderTrackBearing(Canvas &canvas, const PixelPoint aircraft_pos)
{
  // default rendering option assumes circling is off, so ground-relative
  DrawTrackBearing(canvas, aircraft_pos, false);
}

void
MapWindow::RenderTerrain(Canvas &canvas)
{
  background.SetShadingAngle(render_projection, GetMapSettings().terrain,
                             Calculated());
  background.Draw(canvas, render_projection, GetMapSettings().terrain);
}

inline void
MapWindow::RenderRasp(Canvas &canvas)
{
  if (rasp_store == nullptr)
    return;

  const WeatherUIState &state = GetUIState().weather;
  if (rasp_renderer && state.map != (int)rasp_renderer->GetParameter()) {
#ifndef ENABLE_OPENGL
    const ScopeLock protect(mutex);
#endif

    rasp_renderer.reset();
  }

  if (state.map < 0)
    return;

  if (!rasp_renderer) {
#ifndef ENABLE_OPENGL
    const ScopeLock protect(mutex);
#endif
    rasp_renderer.reset(new RaspRenderer(*rasp_store, state.map));
  }

  rasp_renderer->SetTime(state.time);

  {
    QuietOperationEnvironment operation;
    rasp_renderer->Update(Calculated().date_time_local, operation);
  }

  const auto &terrain_settings = GetMapSettings().terrain;
  if (rasp_renderer->Generate(render_projection, terrain_settings))
    rasp_renderer->Draw(canvas, render_projection);
}

void
MapWindow::RenderTopography(Canvas &canvas)
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->Draw(canvas, render_projection);
}

void
MapWindow::RenderTopographyLabels(Canvas &canvas)
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->DrawLabels(canvas, render_projection, label_block);
}

inline void
MapWindow::RenderOverlays(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  if (overlay)
    overlay->Draw(canvas, render_projection);
#endif
}

void
MapWindow::RenderFinalGlideShading(Canvas &canvas)
{
  if (terrain != nullptr &&
      Calculated().terrain_valid)
      DrawTerrainAbove(canvas);
}

void
MapWindow::RenderAirspace(Canvas &canvas)
{
  if (GetMapSettings().airspace.enable) {
    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas,
#endif
                           render_projection,
                           Basic(), Calculated(),
                           GetComputerSettings().airspace,
                           GetMapSettings().airspace);

    airspace_label_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                                 buffer_canvas,
#endif
                                 render_projection,
                                 Basic(), Calculated(),
                                 GetComputerSettings().airspace,
                                 GetMapSettings().airspace);
  }
}

void
MapWindow::RenderNOAAStations(Canvas &canvas)
{
#ifdef HAVE_NOAA
  if (noaa_store == nullptr)
    return;

  PixelPoint pt;
  for (auto it = noaa_store->begin(), end = noaa_store->end(); it != end; ++it)
    if (it->parsed_metar_available && it->parsed_metar.location_available &&
        render_projection.GeoToScreenIfVisible(it->parsed_metar.location, pt))
      look.noaa.icon.Draw(canvas, pt);
#endif
}

inline void
MapWindow::DrawWaves(Canvas &canvas)
{
  const WaveRenderer renderer(look.wave);

#ifdef HAVE_SKYLINES_TRACKING
  if (skylines_data != nullptr) {
    ScopeLock protect(skylines_data->mutex);
    renderer.Draw(canvas, render_projection, *skylines_data);
  }
#endif

  renderer.Draw(canvas, render_projection, Calculated().wave);
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

  // reset label over-write preventer
  label_block.reset();

  render_projection = visible_projection;

  if (!render_projection.IsValid()) {
    canvas.ClearWhite();
    return;
  }

  // Calculate screen position of the aircraft
  PixelPoint aircraft_pos{0,0};
  if (basic.location_available)
      aircraft_pos = render_projection.GeoToScreen(basic.location);

  // General layout principles:
  // - lower elevation drawn on bottom layers
  // - increasing elevation drawn above
  // - increasing importance drawn above
  // - attempt to not obscure text

  //////////////////////////////////////////////// items on ground

  // Render terrain, groundline and topography
  draw_sw.Mark("RenderTerrain");
  RenderTerrain(canvas);

  draw_sw.Mark("RenderRasp");
  RenderRasp(canvas);

  draw_sw.Mark("RenderTopography");
  RenderTopography(canvas);

  draw_sw.Mark("RenderOverlays");
  RenderOverlays(canvas);

  draw_sw.Mark("DrawNOAAStations");
  RenderNOAAStations(canvas);

  //////////////////////////////////////////////// glide range info

  draw_sw.Mark("RenderFinalGlideShading");
  RenderFinalGlideShading(canvas);

  //////////////////////////////////////////////// airspace

  // Render airspace
  draw_sw.Mark("RenderAirspace");
  RenderAirspace(canvas);

  //////////////////////////////////////////////// task

  // Render task, waypoints
  draw_sw.Mark("DrawContest");
  DrawContest(canvas);

  draw_sw.Mark("DrawTask");
  DrawTask(canvas);

  draw_sw.Mark("DrawWaypoints");
  DrawWaypoints(canvas);

  //////////////////////////////////////////////// aircraft level items
  // Render the snail trail
  if (basic.location_available)
    RenderTrail(canvas, aircraft_pos);

  DrawWaves(canvas);

  // Render estimate of thermal location
  DrawThermalEstimate(canvas);

  //////////////////////////////////////////////// text items
  // Render topography on top of airspace, to keep the text readable
  draw_sw.Mark("RenderTopographyLabels");
  RenderTopographyLabels(canvas);

  //////////////////////////////////////////////// navigation overlays
  // Render glide through terrain range
  draw_sw.Mark("RenderGlide");
  RenderGlide(canvas);

  draw_sw.Mark("RenderMisc1");
  // Render weather/terrain max/min values
  DrawTaskOffTrackIndicator(canvas);

  // Render track bearing (projected track ground/air relative)
  draw_sw.Mark("DrawTrackBearing");
  RenderTrackBearing(canvas, aircraft_pos);

  draw_sw.Mark("RenderMisc2");
  DrawBestCruiseTrack(canvas, aircraft_pos);

  // Draw wind vector at aircraft
  if (basic.location_available)
    DrawWind(canvas, aircraft_pos, rc);

  // Render compass
  DrawCompass(canvas, rc);

  //////////////////////////////////////////////// traffic
  // Draw traffic

#ifdef HAVE_SKYLINES_TRACKING
  DrawSkyLinesTraffic(canvas);
#endif

  if (basic.location_available)
      DrawGLinkTraffic(canvas, aircraft_pos);

  DrawTeammate(canvas);

  if (basic.location_available)
    DrawFLARMTraffic(canvas, aircraft_pos);

  //////////////////////////////////////////////// own aircraft
  // Finally, draw you!
  if (basic.location_available)
    AircraftRenderer::Draw(canvas, GetMapSettings(), look.aircraft,
                           basic.attitude.heading - render_projection.GetScreenAngle(),
                           aircraft_pos);

  //////////////////////////////////////////////// important overlays
  // Draw intersections on top of aircraft
  airspace_renderer.DrawIntersections(canvas, render_projection);
}
