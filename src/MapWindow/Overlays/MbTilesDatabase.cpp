// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MbTilesDatabase.hpp"

#include "LogFile.hpp"
#include "Math/Angle.hpp"
#include "ui/canvas/custom/LibPNG.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"
#include "util/NumberParser.hpp"
#include "util/StringSplit.hxx"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string_view>

namespace MbTiles {

static constexpr double MAX_MERCATOR_LATITUDE = 85.05112878;

static constexpr double
ClampLatitude(double latitude) noexcept
{
  return std::clamp(latitude, -MAX_MERCATOR_LATITUDE, MAX_MERCATOR_LATITUDE);
}

static constexpr double
LongitudeToTileX(double longitude, unsigned zoom) noexcept
{
  const double scale = double(1u << zoom);
  return (longitude + 180.) / 360. * scale;
}

static double
LatitudeToTileY(double latitude, unsigned zoom) noexcept
{
  const double limited = ClampLatitude(latitude);
  const double rad = limited * std::numbers::pi_v<double> / 180.;
  const double scale = double(1u << zoom);
  const double mercator = std::asinh(std::tan(rad));
  return (1. - mercator / std::numbers::pi_v<double>) / 2. * scale;
}

static constexpr double
TileXToLongitude(unsigned column, unsigned zoom) noexcept
{
  const double scale = double(1u << zoom);
  return (double(column) / scale) * 360. - 180.;
}

static double
TileYToLatitude(unsigned row, unsigned zoom) noexcept
{
  const double scale = double(1u << zoom);
  const double mercator = std::numbers::pi_v<double> *
    (1. - 2. * double(row) / scale);
  return std::atan(std::sinh(mercator)) * 180. /
    std::numbers::pi_v<double>;
}

static bool
ParseExact(std::string_view value, double &out) noexcept
{
  char *end = nullptr;
  double parsed = ParseDouble(value.data(), &end);
  if (end == value.data() + value.size()) {
    out = parsed;
    return true;
  }
  return false;
}

static bool
ParseExact(std::string_view value, unsigned &out) noexcept
{
  char *end = nullptr;
  unsigned parsed = ParseUnsigned(value.data(), &end);
  if (end == value.data() + value.size()) {
    out = parsed;
    return true;
  }
  return false;
}

static GeoBounds
ParseBounds(std::string_view value) noexcept
{
  if (value.empty())
    return GeoBounds::Invalid();

  const auto [west_string, rest1] = Split(value, ',');
  if (rest1.empty())
    return GeoBounds::Invalid();

  const auto [south_string, rest2] = Split(rest1, ',');
  if (rest2.empty())
    return GeoBounds::Invalid();

  const auto [east_string, north_string] = Split(rest2, ',');
  if (north_string.empty())
    return GeoBounds::Invalid();

  double west, south, east, north;
  if (!ParseExact(west_string, west) ||
      !ParseExact(south_string, south) ||
      !ParseExact(east_string, east) ||
      !ParseExact(north_string, north))
    return GeoBounds::Invalid();

  return GeoBounds({Angle::Degrees(west), Angle::Degrees(north)},
                   {Angle::Degrees(east), Angle::Degrees(south)});
}

TileKey
TileKey::FromGeoPoint(GeoPoint point, unsigned zoom) noexcept
{
  const unsigned scale = 1u << zoom;
  const int raw_column =
    int(std::floor(LongitudeToTileX(point.longitude.Degrees(), zoom)));
  const int raw_xyz_row =
    int(std::floor(LatitudeToTileY(point.latitude.Degrees(), zoom)));
  const unsigned column = std::clamp(raw_column, 0, int(scale - 1u));
  const unsigned xyz_row = std::clamp(raw_xyz_row, 0, int(scale - 1u));

  return {
    zoom,
    column,
    scale - 1u - xyz_row,
  };
}

GeoPoint
TileKey::GetNorthWest() const noexcept
{
  const unsigned scale = 1u << zoom;
  const unsigned xyz_row = scale - 1u - row;

  return {
    Angle::Degrees(TileXToLongitude(column, zoom)),
    Angle::Degrees(TileYToLatitude(xyz_row, zoom)),
  };
}

GeoPoint
TileKey::GetNorthEast() const noexcept
{
  const unsigned scale = 1u << zoom;
  const unsigned xyz_row = scale - 1u - row;

  return {
    Angle::Degrees(TileXToLongitude(column + 1, zoom)),
    Angle::Degrees(TileYToLatitude(xyz_row, zoom)),
  };
}

GeoPoint
TileKey::GetSouthWest() const noexcept
{
  const unsigned scale = 1u << zoom;
  const unsigned xyz_row = scale - 1u - row;

  return {
    Angle::Degrees(TileXToLongitude(column, zoom)),
    Angle::Degrees(TileYToLatitude(xyz_row + 1, zoom)),
  };
}

GeoPoint
TileKey::GetSouthEast() const noexcept
{
  const unsigned scale = 1u << zoom;
  const unsigned xyz_row = scale - 1u - row;

  return {
    Angle::Degrees(TileXToLongitude(column + 1, zoom)),
    Angle::Degrees(TileYToLatitude(xyz_row + 1, zoom)),
  };
}

Database::Database(Path path)
  :db(path)
{
  {
    auto stmt = db.CreateStatement("SELECT name, value FROM metadata");
    while (stmt.StepRow()) {
      const auto name = stmt.GetTextColumn(0);
      const auto value = stmt.GetTextColumn(1);
      if (name.empty() || value.empty())
        continue;

      if (name == "format")
        metadata.format = value.data();
      else if (name == "bounds")
        metadata.bounds = ParseBounds(value);
      else if (name == "minzoom")
        ParseExact(value, metadata.min_zoom);
      else if (name == "maxzoom")
        ParseExact(value, metadata.max_zoom);
    }
  }

  if (metadata.max_zoom == 0 || metadata.min_zoom == 0) {
    auto stmt = db.CreateStatement("SELECT MIN(zoom_level), MAX(zoom_level) "
                                   "FROM tiles");
    if (stmt.StepRow()) {
      if (metadata.min_zoom == 0)
        metadata.min_zoom = stmt.GetIntColumn(0);

      if (metadata.max_zoom == 0)
        metadata.max_zoom = stmt.GetIntColumn(1);
    }
  }
}

bool
Database::HasTile(TileKey key) const
{
  auto stmt = db.CreateStatement("SELECT 1 FROM tiles "
                                 "WHERE zoom_level=? AND tile_column=? AND tile_row=?");
  stmt.BindInt(1, key.zoom);
  stmt.BindInt(2, key.column);
  stmt.BindInt(3, key.row);
  return stmt.StepRow();
}

Bitmap
Database::LoadTile(TileKey key) const
{
  auto stmt = db.CreateStatement("SELECT tile_data FROM tiles "
                                 "WHERE zoom_level=? AND tile_column=? AND tile_row=?");
  stmt.BindInt(1, key.zoom);
  stmt.BindInt(2, key.column);
  stmt.BindInt(3, key.row);

  if (!stmt.StepRow())
    throw std::runtime_error("MBTiles tile not found");

  const auto *data = stmt.GetBlobColumn(0);
  const int size = stmt.GetBytesColumn(0);
  const std::span raw_tile{data, std::size_t(size)};

  Bitmap bitmap;
  UncompressedImage uncompressed;
  try {
    uncompressed = LoadPNG(raw_tile);
  } catch (const std::exception &) {
    LogFmt("Failed to decode MBTiles tile image: z={} x={} y={}",
           key.zoom, key.column, key.row);
    throw;
  }

  if (!uncompressed.IsDefined() || !bitmap.Load(std::move(uncompressed))) {
    /* MbTilesOverlay::Draw() catches this and skips only the failing
       tile, so a broken image does not stop XCSoar in flight. */
    LogFmt("Failed to load MBTiles tile bitmap: z={} x={} y={}",
           key.zoom, key.column, key.row);
    throw std::runtime_error("Failed to decode MBTiles tile image");
  }

  return bitmap;
}

} // namespace MbTiles

