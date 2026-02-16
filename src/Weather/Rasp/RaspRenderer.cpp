// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspRenderer.hpp"
#include "RaspCache.hpp"
#include "RaspStyle.hpp"
#include "Terrain/RasterMap.hpp"
#include "Terrain/TerrainSettings.hpp"
#include "ui/canvas/Ramp.hpp"
#include "Projection/WindowProjection.hpp"
#include "util/StringAPI.hxx"

[[gnu::pure]]
static const RaspStyle &
LookupWeatherTerrainStyle(const char *name)
{
  const auto *i = rasp_styles;
  while (i->name != nullptr && !StringIsEqual(i->name, name))
    ++i;

  return *i;
}

bool
RaspRenderer::Generate(const WindowProjection &projection,
                       const TerrainRendererSettings &settings)
{
  const auto &style = LookupWeatherTerrainStyle(cache.GetMapName());
  const bool do_water = style.do_water;
  const unsigned height_scale = style.height_scale;
  const int interp_levels = 5;
  const ColorRamp *color_ramp = style.color_ramp;

  const RasterMap *map = cache.GetMap();
  if (map == nullptr)
    return false;

  if (!map->GetBounds().Overlaps(projection.GetScreenBounds()))
    /* not visible */
    return false;

  if (color_ramp != last_color_ramp) {
    raster_renderer.PrepareColorTable(color_ramp, do_water,
                                      height_scale, interp_levels);
    last_color_ramp = color_ramp;
  }

  raster_renderer.ScanMap(*map, projection);

  raster_renderer.GenerateImage(false, height_scale,
                                settings.contrast, settings.brightness,
                                Angle::Zero(), false);
  return true;
}
