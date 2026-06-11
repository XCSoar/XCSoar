// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Pure geometry query over a parsed XCTherm forecast layer, split out
 * from XCThermGeoJSON.cpp so it links without boost::json / LogFile and
 * can be unit-tested in isolation (see test/src/TestXCThermBandQuery).
 */

#include "XCThermGeoJSON.hpp"

#include <cmath>
#include <cstddef>

namespace XCThermGeoJSON {

/**
 * Ray-casting point-in-polygon test on a single ring, in geographic
 * coordinates (longitude = x, latitude = y). Good enough at forecast
 * scale; we don't correct for meridian convergence.
 */
[[gnu::pure]]
static bool
PointInRing(const Ring &ring, double x, double y) noexcept
{
  bool inside = false;
  const std::size_t n = ring.size();
  if (n < 3)
    return false;
  for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
    const double xi = ring[i].longitude.Degrees();
    const double yi = ring[i].latitude.Degrees();
    const double xj = ring[j].longitude.Degrees();
    const double yj = ring[j].latitude.Degrees();
    if (((yi > y) != (yj > y)) &&
        (x < (xj - xi) * (y - yi) / (yj - yi) + xi))
      inside = !inside;
  }
  return inside;
}

bool
FindBandAtPoint(const ForecastLayer &layer, GeoPoint p,
                double &out_min_ms, double &out_max_ms) noexcept
{
  if (layer.IsEmpty())
    return false;

  const double x = p.longitude.Degrees();
  const double y = p.latitude.Degrees();

  bool found = false;
  double best_abs_mid = -1.0;

  for (const auto &band : layer.bands) {
    const double mid = (band.min_ms + band.max_ms) / 2.0;
    const double abs_mid = std::abs(mid);
    /* Skip bands that can't beat the current best — cheap early-out
       before the expensive polygon scan. */
    if (found && abs_mid <= best_abs_mid)
      continue;

    for (const auto &polygon : band.polygons) {
      if (polygon.empty() || polygon[0].size() < 3)
        continue;
      /* Exterior ring only; holes are absent in this data (matches
         the Draw path). */
      if (PointInRing(polygon[0], x, y)) {
        out_min_ms = band.min_ms;
        out_max_ms = band.max_ms;
        best_abs_mid = abs_mid;
        found = true;
        break;  /* this band matched; move to next band */
      }
    }
  }

  return found;
}

} // namespace XCThermGeoJSON
