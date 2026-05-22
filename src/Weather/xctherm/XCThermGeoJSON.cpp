// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermGeoJSON.hpp"
#include "LogFile.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

namespace XCThermGeoJSON {

/**
 * Minimal JSON number parser — reads a double starting at pos,
 * advances pos past the number.
 */
static double
ReadNumber(const char *&p) noexcept
{
  char *end;
  double v = std::strtod(p, &end);
  p = end;
  return v;
}

/**
 * Skip whitespace and commas.
 */
static void
SkipWS(const char *&p) noexcept
{
  while (*p == ' ' || *p == '\t' || *p == ',' || *p == '\r')
    ++p;
}

/**
 * Find the next occurrence of a key like "min": in the string.
 * Returns pointer to the character after the colon, or nullptr.
 */
static const char *
FindKey(const char *p, const char *key) noexcept
{
  const char *f = std::strstr(p, key);
  if (f == nullptr)
    return nullptr;
  f += std::strlen(key);
  while (*f == ' ' || *f == ':')
    ++f;
  return f;
}

/**
 * Parse one coordinate pair [lon, lat] starting at p.
 * p should point at the opening '['.
 * Advances p past the closing ']'.
 */
static bool
ParseCoord(const char *&p, GeoPoint &out) noexcept
{
  // find '['
  while (*p && *p != '[') ++p;
  if (*p == '\0') return false;
  ++p; // skip '['

  SkipWS(p);
  double lon = ReadNumber(p);
  SkipWS(p);
  double lat = ReadNumber(p);

  // find closing ']'
  while (*p && *p != ']') ++p;
  if (*p == ']') ++p;

  out = GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
  return true;
}

/**
 * Parse a ring: [ [lon,lat], [lon,lat], ... ]
 * p should point at the opening '[' of the ring.
 */
static bool
ParseRing(const char *&p, Ring &ring) noexcept
{
  // find opening '['
  while (*p && *p != '[') ++p;
  if (*p == '\0') return false;
  ++p; // skip ring-opening '['

  while (*p) {
    SkipWS(p);
    if (*p == ']') {
      ++p; // end of ring
      return true;
    }
    if (*p == '[') {
      GeoPoint pt;
      if (ParseCoord(p, pt))
        ring.push_back(pt);
    } else {
      ++p;
    }
  }
  return false;
}

/**
 * Parse a polygon: [ ring, ring, ... ]
 * (first ring = exterior, rest = holes)
 */
static bool
ParsePolygon(const char *&p, std::vector<Ring> &polygon) noexcept
{
  // find opening '['
  while (*p && *p != '[') ++p;
  if (*p == '\0') return false;
  ++p; // skip polygon-opening '['

  while (*p) {
    SkipWS(p);
    if (*p == ']') {
      ++p;
      return true;
    }
    if (*p == '[') {
      Ring ring;
      if (ParseRing(p, ring) && !ring.empty())
        polygon.push_back(std::move(ring));
    } else {
      ++p;
    }
  }
  return false;
}

/**
 * Parse the coordinates array of a MultiPolygon:
 * [ polygon, polygon, ... ]
 */
static bool
ParseMultiPolygonCoords(const char *&p,
                        std::vector<std::vector<Ring>> &polygons) noexcept
{
  // find "coordinates"
  const char *coords = FindKey(p, "\"coordinates\"");
  if (coords == nullptr)
    return false;
  p = coords;

  // find outer opening '['
  while (*p && *p != '[') ++p;
  if (*p == '\0') return false;
  ++p; // skip MultiPolygon-opening '['

  while (*p) {
    SkipWS(p);
    if (*p == ']') {
      ++p;
      return true;
    }
    if (*p == '[') {
      std::vector<Ring> polygon;
      if (ParsePolygon(p, polygon) && !polygon.empty())
        polygons.push_back(std::move(polygon));
    } else {
      ++p;
    }
  }
  return false;
}

/**
 * Parse one line = one Feature.
 */
static bool
ParseFeature(const char *line, WindBand &band) noexcept
{
  // Extract min/max from properties
  const char *min_ptr = FindKey(line, "\"min\"");
  const char *max_ptr = FindKey(line, "\"max\"");
  if (min_ptr == nullptr || max_ptr == nullptr)
    return false;

  band.min_ms = ReadNumber(min_ptr);
  band.max_ms = ReadNumber(max_ptr);

  // Parse geometry
  const char *p = line;
  return ParseMultiPolygonCoords(p, band.polygons);
}

ForecastLayer
Parse(std::string_view content, bool skip_neutral) noexcept
{
  ForecastLayer layer;

  const char *start = content.data();
  const char *end = start + content.size();

  while (start < end) {
    // Find next line
    const char *eol = start;
    while (eol < end && *eol != '\n')
      ++eol;

    // Skip empty lines
    if (eol > start) {
      const std::string line(start, eol - start);
      WindBand band;
      if (ParseFeature(line.c_str(), band)) {
        // Skip neutral band if requested
        if (skip_neutral && band.min_ms >= -0.2 && band.max_ms <= 0.2) {
          // don't add
        } else if (!band.polygons.empty()) {
          layer.bands.push_back(std::move(band));
        }
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
