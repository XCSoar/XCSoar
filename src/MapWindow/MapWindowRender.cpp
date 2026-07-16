// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "Overlay.hpp"
#include "Look/MapLook.hpp"
#include "Weather/Rasp/RaspRenderer.hpp"
#include "Weather/Rasp/RaspCache.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#ifdef HAVE_HTTP
#include "DataGlobals.hpp"
#include "Weather/SkySight/SkySightManager.hpp"
#endif
#include "Topography/CachedTopographyRenderer.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/WaveRenderer.hpp"
#include "Operation/Operation.hpp"
#include "Tracking/SkyLines/Data.hpp"

#ifdef HAVE_NOAA
#include "Weather/NOAAStore.hpp"
#endif

void
MapWindow::RenderTrackBearing(Canvas &canvas,
                              const PixelPoint aircraft_pos) noexcept
{
  // default rendering option assumes circling is off, so ground-relative
  DrawTrackBearing(canvas, aircraft_pos, false);
}

inline void
MapWindow::RenderTerrain(Canvas &canvas) noexcept
{
  background.SetShadingAngle(render_projection, GetMapSettings().terrain,
                             Calculated());
  background.Draw(canvas, render_projection, GetMapSettings().terrain);
}

inline void
MapWindow::RenderRasp(Canvas &canvas) noexcept
{
  if (rasp_store == nullptr)
    return;

  const WeatherUIState &state = GetUIState().weather;
  if (rasp_renderer &&
      (state.map < 0 ||
       unsigned(state.map) >= rasp_store->GetItemCount() ||
       state.map != (int)rasp_renderer->GetParameter())) {
#ifndef ENABLE_OPENGL
    const std::lock_guard lock{mutex};
#endif

    rasp_renderer.reset();
  }

  if (state.map < 0 ||
      unsigned(state.map) >= rasp_store->GetItemCount())
    return;

  BrokenTime auto_local_time = BrokenTime::Invalid();
  if (state.time_auto_advance) {
    const BrokenDateTime &utc = Basic().date_time_utc;
    if (utc.IsPlausible()) {
      const auto quarter = utc.ToLocal().FloorToQuarterHour();
      auto_local_time = BrokenTime(quarter.hour, quarter.minute);
    }
  }

  if (!rasp_store->HasSelectedTimeData(unsigned(state.map),
                                       state.time_auto_advance,
                                       state.time, auto_local_time))
    return;

  if (!rasp_renderer) {
#ifndef ENABLE_OPENGL
    const std::lock_guard lock{mutex};
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

inline void
MapWindow::RenderTopography(Canvas &canvas) noexcept
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->Draw(canvas, render_projection);
}

inline void
MapWindow::RenderTopographyLabels(Canvas &canvas) noexcept
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->DrawLabels(canvas, render_projection, label_block);
}

inline void
MapWindow::RenderOverlays([[maybe_unused]] Canvas &canvas) noexcept
{
#ifdef ENABLE_OPENGL
#if defined(HAVE_HTTP)
  for (const auto &i : overlay)
    if (i)
      i->Draw(canvas, render_projection);
#else
  if (overlay)
    overlay->Draw(canvas, render_projection);
#endif
#endif
}

inline void
MapWindow::RenderFinalGlideShading(Canvas &canvas) noexcept
{
  if (terrain != nullptr &&
      Calculated().terrain_valid)
      DrawTerrainAbove(canvas);
}

inline void
MapWindow::RenderAirspace(Canvas &canvas) noexcept
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
                                 render_projection,
                                 Basic(), Calculated(),
                                 GetComputerSettings().airspace,
                                 GetMapSettings().airspace,
                                 &label_block);
  }
}

inline void
MapWindow::RenderNOAAStations(Canvas &canvas) noexcept
{
#ifdef HAVE_NOAA
  if (noaa_store == nullptr)
    return;

  for (auto it = noaa_store->begin(), end = noaa_store->end(); it != end; ++it)
    if (it->parsed_metar_available && it->parsed_metar.location_available)
      if (auto pt = render_projection.GeoToScreenIfVisible(it->parsed_metar.location))
        look.noaa.icon.Draw(canvas, *pt);
#else
  (void)canvas;
#endif
}

inline void
MapWindow::DrawWaves(Canvas &canvas) noexcept
{
  const WaveRenderer renderer(look.wave);

#ifdef HAVE_SKYLINES_TRACKING
  if (skylines_data != nullptr) {
    const std::lock_guard lock{skylines_data->mutex};
    renderer.Draw(canvas, render_projection, *skylines_data);
  }
#endif

  renderer.Draw(canvas, render_projection, Calculated().wave);
}

inline void
MapWindow::RenderGlide(Canvas &canvas) noexcept
{
  // draw red cross on glide through terrain marker
  if (Calculated().terrain_valid)
    DrawGlideThroughTerrain(canvas);
}

void
MapWindow::Render(Canvas &canvas, const PixelRect &rc) noexcept
{
  const NMEAInfo &basic = Basic();

  // reset label over-write preventer
  label_block.reset();

#ifndef ENABLE_OPENGL
  {
    const std::lock_guard lock{frame_projection_mutex};
    render_projection = published_projection;
  }
#else
  render_projection = visible_projection;
#endif

  if (!render_projection.IsValid() ||
      !render_projection.GetScreenBounds().IsValid()) {
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

#ifdef HAVE_HTTP
  if (auto skysight = DataGlobals::GetSkySight())
    skysight->Render();
#endif

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

  //////////////////////////////////////////////// distance rings

  draw_sw.Mark("DrawDistanceRings");
  DrawDistanceRings(canvas);

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

  // Render track bearing (projected track ground/air relative)
  draw_sw.Mark("DrawTrackBearing");
  RenderTrackBearing(canvas, aircraft_pos);
  
  // Render Detour cost markers
  draw_sw.Mark("RenderMisc1");
  DrawTaskOffTrackIndicator(canvas);

  // Draw the Turn Back Marker (TBM) on the track line
  DrawTurnBackMarker(canvas);

  draw_sw.Mark("RenderMisc2");
  DrawBestCruiseTrack(canvas, aircraft_pos);

  // Draw wind vector at aircraft
  if (basic.location_available)
    DrawWind(canvas, aircraft_pos, rc);

  // Render compass
  DrawCompass(canvas, rc);

  //////////////////////////////////////////////// traffic
  // Draw traffic

  DrawGLinkTraffic(canvas);

  DrawTeammate(canvas);

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
