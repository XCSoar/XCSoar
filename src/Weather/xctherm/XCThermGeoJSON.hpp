// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace XCThermGeoJSON {

/**
 * A single polygon ring (closed ring of GeoPoints).
 */
using Ring = std::vector<GeoPoint>;

/**
 * One wind-speed band (e.g. +0.2 → +0.5 m/s) containing
 * many polygons.
 */
struct WindBand {
  double min_ms;  // minimum vertical wind m/s
  double max_ms;  // maximum vertical wind m/s

  /**
   * Each polygon is a vector of rings.
   * Ring 0 = exterior, rings 1..n = holes (rare in weather data).
   */
  std::vector<std::vector<Ring>> polygons;

  /** Total number of coordinate points across all polygons. */
  [[gnu::pure]]
  unsigned TotalCoords() const noexcept {
    unsigned n = 0;
    for (const auto &poly : polygons)
      for (const auto &ring : poly)
        n += ring.size();
    return n;
  }
};

/**
 * A complete forecast layer parsed from one GeoJSON file.
 */
struct ForecastLayer {
  /** All wind bands in this layer */
  std::vector<WindBand> bands;

  /** Metadata (optional, populated if available) */
  std::string layer_name;   // e.g. "vertical_wind_5000amsl"
  std::string model;        // e.g. "ICON-CH"
  std::string valid_time;   // ISO timestamp if known

  /** Is there any data? */
  bool IsEmpty() const noexcept { return bands.empty(); }

  /** Total number of polygons across all bands */
  [[gnu::pure]]
  unsigned TotalPolygons() const noexcept {
    unsigned n = 0;
    for (const auto &b : bands)
      n += b.polygons.size();
    return n;
  }

  /** Total number of coordinates across all bands */
  [[gnu::pure]]
  unsigned TotalCoords() const noexcept {
    unsigned n = 0;
    for (const auto &b : bands)
      n += b.TotalCoords();
    return n;
  }
};

/**
 * Parse a newline-delimited GeoJSON string (one Feature per line).
 *
 * Each Feature must have:
 *   properties: { "min": <float>, "max": <float> }
 *   geometry: { "type": "MultiPolygon", "coordinates": [...] }
 *
 * @param content The full file content
 * @param skip_neutral If true, skip the band where
 *   min >= -0.2 && max <= 0.2 (saves ~48% of data)
 * @return Parsed forecast layer
 */
ForecastLayer
Parse(std::string_view content, bool skip_neutral = true) noexcept;

} // namespace XCThermGeoJSON
