// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermGeoJSON.hpp"
#include "LogFile.hpp"
#include "io/FileReader.hxx"

#include <boost/json.hpp>

#include <exception>
#include <span>
#include <string>

namespace XCThermGeoJSON {

std::string
ReadFile(Path path) noexcept
{
  try {
    FileReader reader(path);
    const uint64_t size = reader.GetSize();
    if (size == 0 || size > 32 * 1024 * 1024)
      return {};

    std::string content(size, '\0');
    const std::size_t nbytes = reader.Read(std::span{
      reinterpret_cast<std::byte *>(content.data()), size});
    content.resize(nbytes);
    return content;
  } catch (...) {
    return {};
  }
}

ForecastLayer
ParseFile(Path path, bool skip_neutral) noexcept
{
  return Parse(ReadFile(path), skip_neutral);
}

static bool
TryParseCoord(const boost::json::value &v, GeoPoint &out) noexcept
{
  if (!v.is_array())
    return false;

  const auto &arr = v.as_array();
  if (arr.size() < 2 || !arr[0].is_number() || !arr[1].is_number())
    return false;

  out = GeoPoint{Angle::Degrees(arr[0].as_double()),
                 Angle::Degrees(arr[1].as_double())};
  return out.Check();
}

static bool
ParseRing(const boost::json::array &ring_array, Ring &ring) noexcept
{
  ring.clear();
  ring.reserve(ring_array.size());

  for (const auto &pt_v : ring_array) {
    GeoPoint pt;
    if (TryParseCoord(pt_v, pt))
      ring.push_back(pt);
  }

  return ring.size() >= 3;
}

static bool
ParsePolygon(const boost::json::array &polygon_array,
             std::vector<Ring> &polygon) noexcept
{
  polygon.clear();

  for (const auto &ring_v : polygon_array) {
    if (!ring_v.is_array())
      continue;

    Ring ring;
    if (ParseRing(ring_v.as_array(), ring))
      polygon.push_back(std::move(ring));
  }

  return !polygon.empty();
}

static bool
ParseMultiPolygon(const boost::json::object &geometry,
                  std::vector<std::vector<Ring>> &polygons) noexcept
{
  const auto *type = geometry.if_contains("type");
  if (type == nullptr || !type->is_string() ||
      type->as_string() != "MultiPolygon")
    return false;

  const auto *coordinates = geometry.if_contains("coordinates");
  if (coordinates == nullptr || !coordinates->is_array())
    return false;

  polygons.clear();

  for (const auto &polygon_v : coordinates->as_array()) {
    if (!polygon_v.is_array())
      continue;

    std::vector<Ring> polygon;
    if (ParsePolygon(polygon_v.as_array(), polygon))
      polygons.push_back(std::move(polygon));
  }

  return !polygons.empty();
}

static bool
ParseFeature(const boost::json::value &jv, WindBand &band) noexcept
{
  if (!jv.is_object())
    return false;

  const auto &feature = jv.as_object();

  const auto *properties = feature.if_contains("properties");
  if (properties == nullptr || !properties->is_object())
    return false;

  const auto &props = properties->as_object();
  const auto *min_v = props.if_contains("min");
  const auto *max_v = props.if_contains("max");
  if (min_v == nullptr || max_v == nullptr ||
      !min_v->is_number() || !max_v->is_number())
    return false;

  band.min_ms = min_v->as_double();
  band.max_ms = max_v->as_double();

  const auto *geometry = feature.if_contains("geometry");
  if (geometry == nullptr || !geometry->is_object())
    return false;

  return ParseMultiPolygon(geometry->as_object(), band.polygons);
}

ForecastLayer
Parse(std::string_view content, bool skip_neutral) noexcept
{
  ForecastLayer layer;

  const char *start = content.data();
  const char *end = start + content.size();

  while (start < end) {
    const char *eol = start;
    while (eol < end && *eol != '\n')
      ++eol;

    if (eol > start) {
      try {
        const std::string line(start, eol - start);
        const boost::json::value jv = boost::json::parse(line);

        WindBand band;
        if (ParseFeature(jv, band)) {
          if (skip_neutral && band.min_ms >= -0.2 && band.max_ms <= 0.2) {
            /* skip neutral band */
          } else if (!band.polygons.empty()) {
            layer.bands.push_back(std::move(band));
          }
        }
      } catch (const std::exception &e) {
        LogFmt("xctherm geojson: skip line: {}", e.what());
      }
    }

    if (eol >= end)
      break;
    start = eol + 1;
  }

  LogFmt("xctherm geojson: parsed {} bands, {} polygons, {} coords",
         layer.bands.size(), layer.TotalPolygons(), layer.TotalCoords());

  return layer;
}

} // namespace XCThermGeoJSON
