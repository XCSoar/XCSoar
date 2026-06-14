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
#include <limits>
#include <map>
#include <mutex>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string_view>

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

static constexpr unsigned MAX_TILE_ZOOM =
  std::numeric_limits<unsigned>::digits - 2;

static void
NormaliseZoom(unsigned &min_zoom, unsigned &max_zoom) noexcept
{
  if (min_zoom > MAX_TILE_ZOOM)
    min_zoom = MAX_TILE_ZOOM;

  if (max_zoom > MAX_TILE_ZOOM)
    max_zoom = MAX_TILE_ZOOM;

  if (min_zoom > max_zoom) {
    if (max_zoom > 0)
      std::swap(min_zoom, max_zoom);
    else if (min_zoom > 0)
      max_zoom = min_zoom;
  }

  if (min_zoom == 0 && max_zoom == 0) {
    min_zoom = 1;
    max_zoom = 1;
  } else if (min_zoom == 0)
    min_zoom = max_zoom;
  else if (max_zoom == 0)
    max_zoom = min_zoom;
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

MbTilesDatabase::MbTilesDatabase(Path path)
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
    auto stmt = db.CreateStatement("SELECT MIN(zoom_level), MAX(zoom_level) FROM tiles");
    if (stmt.StepRow()) {
      if (metadata.min_zoom == 0) {
        const int min_int = stmt.GetIntColumn(0);
        if (min_int >= 0)
          metadata.min_zoom = unsigned(min_int);
      }

      if (metadata.max_zoom == 0) {
        const int max_int = stmt.GetIntColumn(1);
        if (max_int >= 0)
          metadata.max_zoom = unsigned(max_int);
      }
    }
  }

  NormaliseZoom(metadata.min_zoom, metadata.max_zoom);
}

bool
MbTilesDatabase::HasTileUnlocked(TileKey key) const
{
  auto stmt = db.CreateStatement("SELECT 1 FROM tiles "
                                 "WHERE zoom_level=? AND tile_column=? AND tile_row=?");
  stmt.BindInt(1, key.zoom);
  stmt.BindInt(2, key.column);
  stmt.BindInt(3, key.row);
  return stmt.StepRow();
}

bool
MbTilesDatabase::HasTile(TileKey key) const
{
  const std::lock_guard lock{mutex};
  return HasTileUnlocked(key);
}

static UncompressedImage
LoadUncompressedTile(const SqliteDatabase &db, TileKey key)
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

  try {
    UncompressedImage uncompressed = LoadPNG(raw_tile);
    if (!uncompressed.IsDefined())
      throw std::runtime_error("Failed to decode MBTiles tile image");
    return uncompressed;
  } catch (const std::exception &) {
    LogFmt("Failed to decode MBTiles tile image: z={} x={} y={}",
           key.zoom, key.column, key.row);
    throw;
  }
}

static bool
ReadRgbaPixel(const UncompressedImage &image, unsigned x, unsigned y,
              Rgba8 &pixel)
{
  if (image.GetFormat() != UncompressedImage::Format::RGBA)
    return false;

  const unsigned width = image.GetWidth();
  const unsigned height = image.GetHeight();
  if (width == 0 || height == 0 || x >= width || y >= height)
    return false;

  const auto *row = static_cast<const uint8_t *>(image.GetData()) +
    std::size_t(y) * image.GetPitch();
  const unsigned offset = x * 4u;
  pixel.r = row[offset];
  pixel.g = row[offset + 1];
  pixel.b = row[offset + 2];
  pixel.a = row[offset + 3];
  return true;
}

static bool
ComputePixelCoords(GeoPoint p, TileKey key, unsigned width, unsigned height,
                   unsigned &x, unsigned &y) noexcept
{
  const GeoPoint north_west = key.GetNorthWest();
  const GeoPoint south_east = key.GetSouthEast();
  const Angle lon_span = south_east.longitude - north_west.longitude;
  const Angle lat_span = north_west.latitude - south_east.latitude;
  if (lon_span == Angle::Zero() || lat_span == Angle::Zero())
    return false;

  if (width == 0 || height == 0)
    return false;

  const double fx = std::clamp((p.longitude - north_west.longitude) / lon_span,
                               0., 1.);
  const double fy = std::clamp((north_west.latitude - p.latitude) / lat_span,
                               0., 1.);

  x = std::min(unsigned(fx * double(width)), width - 1u);
  y = std::min(unsigned(fy * double(height)), height - 1u);
  return true;
}

static bool
SampleRgbaInImage(GeoPoint p, TileKey key, const UncompressedImage &image,
                  Rgba8 &pixel) noexcept
{
  unsigned x = 0, y = 0;
  if (!ComputePixelCoords(p, key, image.GetWidth(), image.GetHeight(), x, y))
    return false;

  if (!ReadRgbaPixel(image, x, y, pixel))
    return false;

  if (pixel.r == 0 && pixel.g == 0 && pixel.b == 0 && pixel.a == 0)
    return false;

  return true;
}

bool
MbTilesDatabase::SampleRgbaAtGeo(GeoPoint p, Rgba8 &pixel) const noexcept
{
  try {
    const std::lock_guard lock{mutex};
    const TileKey key = TileKey::FromGeoPoint(p, metadata.max_zoom);
    if (!HasTileUnlocked(key))
      return false;

    UncompressedImage image;
    try {
      image = LoadUncompressedTile(db, key);
    } catch (const std::exception &) {
      return false;
    }

    if (image.GetFormat() != UncompressedImage::Format::RGBA)
      return false;

    return SampleRgbaInImage(p, key, image, pixel);
  } catch (const std::exception &) {
    return false;
  }
}

Bitmap
MbTilesDatabase::LoadTile(TileKey key) const
{
  const std::lock_guard lock{mutex};
  UncompressedImage uncompressed = LoadUncompressedTile(db, key);

  Bitmap bitmap;
  if (!bitmap.Load(std::move(uncompressed))) {
    /* MbTilesOverlay::Draw() catches this and skips only the failing
       tile, so a broken image does not stop XCSoar in flight. */
    LogFmt("Failed to load MBTiles tile bitmap: z={} x={} y={}",
           key.zoom, key.column, key.row);
    throw std::runtime_error("Failed to decode MBTiles tile image");
  }

  return bitmap;
}
