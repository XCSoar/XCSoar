// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermGeoJSON.hpp"
#include "LogFile.hpp"

#include <boost/json.hpp>

#include <cstdlib>
#include <string_view>

namespace XCThermGeoJSON {

/**
 * Parse one [lon, lat] coordinate pair into a GeoPoint.
 * @return false if @p arr doesn't hold two numeric elements.
 */
static bool
ParseCoord(const boost::json::array &arr, GeoPoint &out) noexcept
{
  if (arr.size() < 2)
    return false;
  try {
    const double lon = arr.at(0).to_number<double>();
    const double lat = arr.at(1).to_number<double>();
    out = GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
    return true;
  } catch (...) {
    return false;
  }
}

/**
 * Parse a ring [[lon,lat], [lon,lat], …] into a Ring.
 */
static void
ParseRing(const boost::json::array &arr, Ring &ring)
{
  ring.reserve(arr.size());
  for (const auto &pt_val : arr) {
    if (!pt_val.is_array())
      continue;
    GeoPoint pt;
    if (ParseCoord(pt_val.as_array(), pt))
      ring.push_back(pt);
  }
}

/**
 * Parse one polygon — array of rings; ring 0 = exterior, rest = holes.
 */
static void
ParsePolygon(const boost::json::array &arr, std::vector<Ring> &polygon)
{
  polygon.reserve(arr.size());
  for (const auto &ring_val : arr) {
    if (!ring_val.is_array())
      continue;
    Ring ring;
    ParseRing(ring_val.as_array(), ring);
    if (!ring.empty())
      polygon.push_back(std::move(ring));
  }
}

/**
 * Parse the "coordinates" of a MultiPolygon geometry into @p polygons.
 */
static void
ParseMultiPolygonCoords(const boost::json::array &arr,
                        std::vector<std::vector<Ring>> &polygons)
{
  polygons.reserve(arr.size());
  for (const auto &poly_val : arr) {
    if (!poly_val.is_array())
      continue;
    std::vector<Ring> polygon;
    ParsePolygon(poly_val.as_array(), polygon);
    if (!polygon.empty())
      polygons.push_back(std::move(polygon));
  }
}

/**
 * Parse one Feature (one line of the GeoJSON stream).
 *
 * Expected shape:
 *   { "type": "Feature",
 *     "properties": { "min": <num>, "max": <num> },
 *     "geometry": { "type": "MultiPolygon",
 *                   "coordinates": [[[[lon,lat],...]]]} }
 *
 * Errors swallow into @return false — server occasionally serves
 * Features without geometry on the edges of a tile and we just skip
 * those.
 */
static bool
ParseFeature(std::string_view line, WindBand &band) noexcept
{
  try {
    boost::json::value root = boost::json::parse(line);
    if (!root.is_object())
      return false;
    const auto &feature = root.as_object();

    /* properties.min / properties.max — band bounds in m/s */
    auto it_props = feature.find("properties");
    if (it_props == feature.end() || !it_props->value().is_object())
      return false;
    const auto &props = it_props->value().as_object();

    auto it_min = props.find("min");
    auto it_max = props.find("max");
    if (it_min == props.end() || it_max == props.end())
      return false;
    band.min_ms = it_min->value().to_number<double>();
    band.max_ms = it_max->value().to_number<double>();

    /* geometry.coordinates */
    auto it_geom = feature.find("geometry");
    if (it_geom == feature.end() || !it_geom->value().is_object())
      return false;
    const auto &geom = it_geom->value().as_object();

    auto it_coords = geom.find("coordinates");
    if (it_coords == geom.end() || !it_coords->value().is_array())
      return false;

    ParseMultiPolygonCoords(it_coords->value().as_array(), band.polygons);
    return true;
  } catch (const std::exception &) {
    /* Malformed line — skip silently (server sometimes ends a tile
       with a partially-buffered Feature). */
    return false;
  }
}

ForecastLayer
Parse(std::string_view content, bool skip_neutral) noexcept
{
  ForecastLayer layer;

  /* The server streams one Feature per line (NDJSON style). Iterate
     over lines, parse each as JSON, append to the layer's bands. */
  std::size_t pos = 0;
  while (pos < content.size()) {
    const std::size_t eol = content.find('\n', pos);
    const std::size_t end = (eol == std::string_view::npos)
      ? content.size() : eol;

    if (end > pos) {
      const std::string_view line = content.substr(pos, end - pos);

      WindBand band;
      if (ParseFeature(line, band)) {
        const bool is_neutral =
          band.min_ms >= -0.2 && band.max_ms <= 0.2;
        if ((!skip_neutral || !is_neutral) && !band.polygons.empty()) {
          /* Grow the layer's geographic extent over this band's
             exterior rings, so callers can cheaply test coverage. */
          for (const auto &polygon : band.polygons)
            if (!polygon.empty())
              for (const auto &pt : polygon[0])
                layer.bounds.Extend(pt);

          /* push_back can throw bad_alloc; the function is noexcept
             on the .hpp, so we keep allocations modest by reserving
             nothing extra and trust that std::terminate on OOM is
             acceptable here (consistent with the rest of the file). */
          layer.bands.push_back(std::move(band));
        }
      }
    }

    if (eol == std::string_view::npos)
      break;
    pos = eol + 1;
  }

  LogFmt("xctherm geojson: parsed {} bands, {} polygons, {} coords",
         layer.bands.size(), layer.TotalPolygons(), layer.TotalCoords());

  return layer;
}

/* FindBandAtPoint() + its PointInRing() helper live in
   XCThermGeoQuery.cpp so they link without boost::json and stay
   unit-testable. */

} // namespace XCThermGeoJSON
