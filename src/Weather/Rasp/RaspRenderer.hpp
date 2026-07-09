// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RaspCache.hpp"
#include "Terrain/RasterRenderer.hpp"
#include "Weather/WeatherUIState.hpp"
#include "Units/Group.hpp"
#include "time/BrokenTime.hpp"
#include "util/StaticString.hxx"

#include <cstdint>

#ifndef ENABLE_OPENGL
#include "Projection/CompareProjection.hpp"
#endif

struct TerrainRendererSettings;
struct GeoPoint;

/**
 * The value of a RASP field sampled at a geographic point.
 */
struct RaspFieldValue {
  /**
   * The field value in the canonical system unit of #unit_group
   * (only meaningful if #available).
   */
  double value;

  /**
   * The physical quantity, for formatting in user units.
   * UnitGroup::NONE means the value is unitless.
   */
  UnitGroup unit_group;

  /**
   * Is field data available at the sampled point?  False outside the
   * field bounds or where the data is invalid.
   */
  bool available;
};

class RaspRenderer {
  RaspCache cache;

  RasterRenderer raster_renderer;

#ifndef ENABLE_OPENGL
  CompareProjection compare_projection;
#endif

  uint32_t last_ramp_hash = 0;

  /**
   * The #RasterMap that was rendered in the previous Generate() call.
   * Used as a cheap "data changed" signal: RaspCache::Reload() swaps in
   * a fresh RasterMap whenever data changes, so pointer identity is reliable.
   */
  const RasterMap *last_map = nullptr;

  ContourDensity last_contour_density = ContourDensity::OFF;

  /**
   * The contrast/brightness settings passed to GenerateImage() in the
   * previous Generate() call.  GenerateImage() bakes these into the
   * rendered pixels, so the cached image may only be reused while they
   * are unchanged.
   */
  short last_contrast = -1, last_brightness = -1;

#ifdef ENABLE_OPENGL
  double last_projection_scale = 0;
#endif

public:
  RaspRenderer(const RaspStore &_store, unsigned parameter)
    :cache(_store, parameter) {
#ifdef ENABLE_OPENGL
    /* Quantization limited to legacy value of 2 until further
       optimizations in place. */
    raster_renderer.SetMinQuantisationPixels(2);
#endif
  }

  /**
   * Flush the cache.
   */
  void Flush() {
#ifdef ENABLE_OPENGL
    raster_renderer.Invalidate();
#else
    compare_projection.Clear();
#endif
    last_map = nullptr;
  }

  unsigned GetParameter() const {
    return cache.GetParameter();
  }

  /**
   * Returns the human-readable name for the current RASP map, or
   * nullptr if no RASP map is enabled.
   */
  [[gnu::pure]]
  const char *GetLabel() const {
    return cache.GetMapLabel();
  }

  /**
   * Returns a description that is the label with additional information
   * appended, e.g. "Rain 12:45lst".
   */
  [[gnu::pure]]
  StaticString<96> GetExtendedLabel() const;

  [[gnu::pure]]
  bool IsInside(GeoPoint p) const {
    return cache.IsInside(p);
  }

  /**
   * Sample the current RASP field at a geographic point, returning
   * the value in the canonical system unit of its #UnitGroup.
   */
  [[gnu::pure]]
  RaspFieldValue GetValueAt(GeoPoint p) const noexcept;

  void SetTime(BrokenTime t) {
    cache.SetTime(t);
  }

  void Update(BrokenTime time_local, OperationEnvironment &operation) {
    cache.Reload(time_local, operation);
  }

  /**
   * @param contour_density density of contour lines to draw;
   * ContourDensity::OFF disables them
   * @return true if an image has been renderered and Draw() may be
   * called
   */
  bool Generate(const WindowProjection &projection,
                const TerrainRendererSettings &settings,
                ContourDensity contour_density = ContourDensity::OFF);

  void Draw(Canvas &canvas, const WindowProjection &projection,
            float alpha=1.0f) const {
    raster_renderer.Draw(canvas, projection, true, alpha);
  }
};
