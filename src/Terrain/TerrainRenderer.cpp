// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/TerrainRenderer.hpp"
#include "Terrain/TerrainColor.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Ramp.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/event/Idle.hpp"

#include <cassert>


// map scale is approximately 2 points on the grid
// therefore, want one to one mapping if mapscale is 0.5
// there are approx 30 pixels in mapscale
// 240/QUANTISATION_PIXELS resolution = 6 pixels per terrain
// (mapscale/30)  km/pixels
//        0.250   km/terrain
// (0.25*30/mapscale) pixels/terrain
//  mapscale/(0.25*30)
//  mapscale/7.5 terrain units/pixel
//
// this is for TerrainInfo.StepSize = 0.0025;
TerrainRenderer::TerrainRenderer(const RasterTerrain &_terrain)
  :terrain(_terrain)
{
  settings.SetDefaults();
}

#ifdef ENABLE_OPENGL
/**
 * Checks if the size difference of any dimension is more than a
 * factor of two.  This is used to check whether the terrain has to be
 * redrawn after zooming in.
 */
static bool
IsLargeSizeDifference(const GeoBounds &a, const GeoBounds &b)
{
  assert(a.IsValid());
  assert(b.IsValid());

  return a.GetWidth().Native() > 2 * b.GetWidth().Native() ||
    a.GetHeight().Native() > 2 * b.GetHeight().Native();
}
#endif

bool
TerrainRenderer::Generate(const WindowProjection &map_projection,
                          const Angle sunazimuth)
{
#ifdef ENABLE_OPENGL
  /* Call once — the helper mutates quantisation_pixels. */
  const bool quantisation_improved = raster_renderer.UpdateQuantisation();
  raster_renderer.SetSunFromAzimuth(sunazimuth, settings.brightness,
                                    settings.contrast);
  const bool sun_ok = raster_renderer.IsShaderHillshade() ||
    sunazimuth.CompareRoughly(last_sun_azimuth);
#else
  constexpr bool quantisation_improved = false;
  const bool sun_ok = sunazimuth.CompareRoughly(last_sun_azimuth);
#endif

  const bool panning = !IsUserIdle(750);

  const unsigned height_scale = 4;
  const double screen_pixel_size =
    1.0 / map_projection.GetScale();
  const double dpi_factor =
    Layout::ScalePenWidth(1024u) / 1024.0;
  const double contour_pixel_size = screen_pixel_size * dpi_factor *
    std::max(1u, raster_renderer.GetQuantisationPixels() / 2u);
  last_contour_spacing = ContourSpacing(settings.contours, height_scale,
                                        contour_pixel_size);
#ifdef ENABLE_OPENGL
  raster_renderer.SetContourSpacing(last_contour_spacing);
#endif

  /* Exact same view: reuse without consulting overscan bounds.
     Near the map edge, overscan is clipped so old_bounds.IsInside()
     can fail even when the projection is unchanged.  Use
     CompareExact (not tolerant Compare) so a tiny pan cannot skip
     the IsInside coverage check.
     While a finger is down, ignore a newer tile serial.  Each
     arrival would otherwise rescan the whole height matrix on the
     UI thread, and a pan hitch repeats until the finger lifts. */
#ifdef ENABLE_OPENGL
  const bool tiles_current =
    terrain_serial == terrain.GetSerial() || panning;
#else
  const bool tiles_current = terrain_serial == terrain.GetSerial();
#endif
  const bool image_current =
    settings.contours == Contours::OFF ||
#ifdef ENABLE_OPENGL
    raster_renderer.IsShaderHillshade() ||
    raster_renderer.GetQuantisationPixels() > 2 ||
#endif
    map_projection.GetScale() == last_projection_scale;
  if (!quantisation_improved &&
      compare_projection.CompareExact(map_projection) &&
      tiles_current &&
      sun_ok &&
      image_current)
    return true;

#ifdef ENABLE_OPENGL
  const GeoBounds &old_bounds = raster_renderer.GetBounds();
  GeoBounds new_bounds = map_projection.GetScreenBounds();
  assert(new_bounds.IsValid());

  {
    RasterTerrain::Lease map(terrain);
    if (!new_bounds.IntersectWith(map->GetBounds()))
      /* map is outside of visible screen area */
      return false;
  }

  /* While the finger is down, ScanMap slides the picture and reads
     only the strip that entered from the direction of the pan.
     This reuse is for a still view whose samples are already
     current. */
  if (!panning &&
      !quantisation_improved &&
      old_bounds.IsValid() && old_bounds.IsInside(new_bounds) &&
      !IsLargeSizeDifference(old_bounds, new_bounds) &&
      tiles_current &&
      sun_ok &&
      image_current) {
    /* CPU contours need a rebuild when zoom changes the interval.
       The shader updates contour_div as a uniform. */
    compare_projection = CompareProjection(map_projection);
    return true;
  }

#endif

  compare_projection = CompareProjection(map_projection);

  last_sun_azimuth = sunazimuth;

  const bool do_water = true;
  const int interp_levels = 2;
  const bool is_terrain = true;
  const bool do_shading = is_terrain &&
                          settings.slope_shading != SlopeShading::OFF;

  const ColorRamp *const color_ramp = &terrain_ramps[settings.ramp];
  if (color_ramp != last_color_ramp) {
    raster_renderer.PrepareColorTable(color_ramp, do_water,
                                      height_scale, interp_levels);
    last_color_ramp = color_ramp;
  }

  {
    RasterTerrain::Lease map(terrain);
    raster_renderer.ScanMap(map, map_projection, !tiles_current);
  }

  /* A pan only reads the strip that entered the view.  A tile that
     finishes loading during that pan still has to be scanned through
     the middle once the map is still, so keep the old serial until
     then. */
  if (terrain_serial == terrain.GetSerial() || !panning)
    terrain_serial = terrain.GetSerial();

#ifdef ENABLE_OPENGL
  if (raster_renderer.IsShaderHillshade() &&
      !raster_renderer.IsHeightMatrixChanged())
    return true;
#endif

  raster_renderer.GenerateImage(do_shading, height_scale,
                                settings.contrast, settings.brightness,
                                sunazimuth,
                                last_contour_spacing);

  last_projection_scale = map_projection.GetScale();

  return true;
}
