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

#ifdef ENABLE_OPENGL
#include "Geo/GeoBounds.hpp"

/**
 * Checks if the size difference of any dimension is more than a factor
 * of two. Used to decide whether the cached RASP image has to be
 * re-rendered after zooming in, analog to terrain rendering.
 */
[[gnu::pure]]
static bool
IsLargeSizeDifference(const GeoBounds &a, const GeoBounds &b) noexcept
{
  return a.GetWidth().Native() > 2 * b.GetWidth().Native() ||
    a.GetHeight().Native() > 2 * b.GetHeight().Native();
}
#endif

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

RaspFieldValue
RaspRenderer::GetValueAt(GeoPoint p) const noexcept
{
  const RasterMap *map = cache.GetMap();
  if (map == nullptr)
    return {0, UnitGroup::NONE, false};

  const TerrainHeight h = map->GetInterpolatedHeight(p);
  if (h.IsSpecial())
    /* invalid or water: no field data here */
    return {0, UnitGroup::NONE, false};

  const auto &style = LookupWeatherTerrainStyle(cache.GetMapName());

  /* the raw raster value is the rendering-domain value; invert the
     colour transform to recover the colormap value-space, then map it
     to the canonical system unit */
  const double colormap_value = (h.GetValue() - style.offset) / style.scale;
  return {style.ToSystemValue(colormap_value), style.unit_group, true};
}

bool
RaspRenderer::Generate(const WindowProjection &projection,
                       const TerrainRendererSettings &settings,
                       ContourDensity contour_density)
{
  const RasterMap *map = cache.GetMap();
  if (map == nullptr)
    return false;

  if (!map->GetBounds().Overlaps(projection.GetScreenBounds()))
    /* not visible */
    return false;

  /* Reuse the previously rendered image if nothing relevant changed.
     This early-out is placed before the (comparatively expensive)
     MaterializeColorRamp / ScanMap / GenerateImage below. */
#ifdef ENABLE_OPENGL
  const GeoBounds &old_bounds = raster_renderer.GetBounds();
  GeoBounds new_bounds = projection.GetScreenBounds();
  new_bounds.IntersectWith(map->GetBounds());   // already known to overlap

  if (old_bounds.IsValid() && old_bounds.IsInside(new_bounds) &&
      !IsLargeSizeDifference(old_bounds, new_bounds) &&
      map == last_map &&
      contour_density == last_contour_density &&
      settings.contrast == last_contrast &&
      settings.brightness == last_brightness &&
      !raster_renderer.UpdateQuantisation()) {
    /* The existing image is suitable for reuse. With contours, only
       reuse without a zoom change; otherwise reuse as a fast preview
       but re-render the higher quality views (q=2 and q=1). */
    if (contour_density == ContourDensity::OFF ||
        raster_renderer.GetQuantisationPixels() > 2 ||
        projection.GetScale() == last_projection_scale)
      return true;
  }
#else
  if (compare_projection.Compare(projection) &&
      map == last_map &&
      contour_density == last_contour_density &&
      settings.contrast == last_contrast &&
      settings.brightness == last_brightness)
    /* no change since previous frame */
    return true;

  compare_projection = CompareProjection(projection);
#endif

  const auto &style = LookupWeatherTerrainStyle(cache.GetMapName());
  const bool do_water = style.do_water;
  const unsigned height_scale = style.height_scale;

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
                                             RASP_INTERP_LEVELS);
    else
      raster_renderer.PrepareColorTable(&ramp, do_water,
                                        height_scale,
                                        RASP_INTERP_LEVELS);
    last_ramp_hash = materialized_color_ramp.hash;
  }

  raster_renderer.ScanMap(*map, projection);

  const unsigned contour_spacing =
    ContourSpacing(contour_density, height_scale);

  raster_renderer.GenerateImage(false, height_scale,
                                settings.contrast, settings.brightness,
                                Angle::Zero(), contour_spacing);

  last_map = map;
  last_contour_density = contour_density;
  last_contrast = settings.contrast;
  last_brightness = settings.brightness;
#ifdef ENABLE_OPENGL
  last_projection_scale = projection.GetScale();
#endif
  return true;
}
