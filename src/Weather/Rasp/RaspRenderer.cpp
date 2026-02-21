// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspRenderer.hpp"
#include "RaspCache.hpp"
#include "RaspStyle.hpp"
#include "ColorMap.hpp"
#include "Terrain/RasterMap.hpp"
#include "Terrain/TerrainSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/RawBitmap.hpp"
#include "util/StringAPI.hxx"

StaticString<96>
RaspRenderer::GetExtendedLabel() const
{
  StaticString<96> result;
  const char *label = cache.GetMapLabel();
  if (label == nullptr) {
    result.clear();
    return result;
  }

  const BrokenTime t = cache.GetLoadedTime();
  if (t.IsPlausible())
    result.Format("%s %02u:%02ulst", label, t.hour, t.minute);
  else
    result = label;

  return result;
}

[[gnu::pure]]
static const RaspStyle &
LookupWeatherTerrainStyle(const char *name)
{
  const auto *i = rasp_styles;
  while (i->name != nullptr && !StringIsEqual(i->name, name))
    ++i;

  if(i->name == nullptr) {
    // If no exact match, try matching the short codes
    // to the end of the field name
    if(StringLength(name) >= 3) {
      const char *short_name =
        &name[StringLength(name) - 3];
      i = rasp_colormaps_general;
      while (i->name != nullptr
             && !StringIsEqual(i->name, short_name))
        ++i;
    }
  }
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

  const RasterMap *map = cache.GetMap();
  if (map == nullptr)
    return false;

  if (!map->GetBounds().Overlaps(projection.GetScreenBounds()))
    /* not visible */
    return false;

  auto materialized_color_ramp =
    MaterializeColorRamp(style.color_map,
                         style.color_map_alpha,
                         style.scale, style.offset,
                         height_scale, style.do_water);

  if (materialized_color_ramp.hash != last_ramp_hash) {
    auto ramp = materialized_color_ramp.GetColorRamp();
 
    // Choose between RGB and RGBA colormap based on style and on
    // whether the rendering backend can blend per-pixel source alpha.
    const bool use_alpha = style.HasAlpha() && HaveBitmapSourceAlpha();

    if (use_alpha)
      raster_renderer.PrepareColorTableAlpha(&ramp, do_water,
                                             height_scale,
                                             interp_levels);
    else
      raster_renderer.PrepareColorTable(&ramp, do_water,
                                        height_scale,
                                        interp_levels);
    last_ramp_hash = materialized_color_ramp.hash;
  }

  raster_renderer.ScanMap(*map, projection);

  raster_renderer.GenerateImage(false, height_scale,
                                settings.contrast, settings.brightness,
                                Angle::Zero(), 0u);
  return true;
}
