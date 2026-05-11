// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "GeoBitmap.hpp"
#include "UncompressedImage.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/Quadrilateral.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string_view>

#ifdef USE_GEOTIFF
#include "LibTiff.hpp"
#endif

#include <stdexcept>

using namespace GeoBitmap;

static int
LonToTileX(double lon, int zoom) noexcept
{
  return (int)std::floor((lon + 180.0) / 360.0 * (1 << zoom));
}

static int
LatToTileY(double lat, int zoom) noexcept
{
  const double latitude_radians = lat * M_PI / 180.0;
  return (int)std::floor((1.0 - std::asinh(std::tan(latitude_radians)) / M_PI) /
                         2.0 * (1 << zoom));
}

static double
TileXToLon(int x, int zoom) noexcept
{
  return x / (double)(1 << zoom) * 360.0 - 180.0;
}

static double
TileYToLat(int y, int zoom) noexcept
{
  const double n = M_PI - 2.0 * M_PI * y / (double)(1 << zoom);
  return 180.0 / M_PI * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}

TileData
GeoBitmap::GetTile(const GeoBounds &bounds, uint16_t zoom) noexcept
{
  return {
    zoom,
    (uint16_t)LonToTileX(bounds.GetCenter().longitude.Degrees(), zoom),
    (uint16_t)LatToTileY(bounds.GetCenter().latitude.Degrees(), zoom),
  };
}

TileData
GeoBitmap::GetTile(const MapWindowProjection &projection,
                   uint16_t zoom_min, uint16_t zoom_max) noexcept
{
  constexpr double earth_circumference = 42e6;

  const double diagonal = projection.GetScreenDistanceMeters();
  const double ratio = earth_circumference / diagonal;
  const auto zoom = (uint16_t)std::clamp((int)std::floor(std::log2(ratio)) + 1,
                                         (int)zoom_min, (int)zoom_max);
  return GetTile(projection.GetScreenBounds(), zoom);
}

GeoQuadrilateral
GeoBitmap::GetGeoQuadrilateral(const TileData &tile) noexcept
{
  GeoQuadrilateral bounds;

  bounds.top_left.longitude = Angle::Degrees(TileXToLon(tile.x, tile.zoom));
  bounds.top_left.latitude = Angle::Degrees(TileYToLat(tile.y, tile.zoom));
  bounds.bottom_left.longitude = bounds.top_left.longitude;
  bounds.bottom_left.latitude = Angle::Degrees(TileYToLat(tile.y + 1, tile.zoom));

  bounds.top_right.longitude = Angle::Degrees(TileXToLon(tile.x + 1, tile.zoom));
  bounds.top_right.latitude = bounds.top_left.latitude;
  bounds.bottom_right.longitude = bounds.top_right.longitude;
  bounds.bottom_right.latitude = bounds.bottom_left.latitude;

  return bounds;
}

GeoBounds
GeoBitmap::GetBounds(const TileData &tile) noexcept
{
  return GetGeoQuadrilateral(tile).GetBounds();
}

static GeoQuadrilateral
ParseTileBounds(std::string_view name)
{
  if (name.starts_with("satellite-"))
    name.remove_prefix(std::char_traits<char>::length("satellite-"));
  else if (name.starts_with("rain-"))
    name.remove_prefix(std::char_traits<char>::length("rain-"));

  auto next = [](std::string_view &remaining) {
    const auto split = remaining.find('-');
    const auto value = remaining.substr(0, split);
    if (split == std::string_view::npos)
      remaining = {};
    else
      remaining.remove_prefix(split + 1);
    return value;
  };

  std::string_view remaining = name;
  const auto zoom_string = next(remaining);
  const auto x_string = next(remaining);
  const auto y_string = next(remaining);
  if (zoom_string.empty() || x_string.empty() || y_string.empty())
    throw std::runtime_error("Unsupported geo image file");

  const auto zoom = (uint16_t)std::strtoul(std::string(zoom_string).c_str(), nullptr, 10);
  const auto x = (uint16_t)std::strtoul(std::string(x_string).c_str(), nullptr, 10);
  const auto y = (uint16_t)std::strtoul(std::string(y_string).c_str(), nullptr, 10);
  return GeoBitmap::GetGeoQuadrilateral({zoom, x, y});
}

GeoQuadrilateral
Bitmap::LoadGeoFile(Path path)
{
#ifdef USE_GEOTIFF
  /* Smallest useful GeoTIFF is far larger; skip obvious truncation before LibTiff. */
  static constexpr unsigned kMinTiffGeoFileSize = 100;

  if (path.EndsWithIgnoreCase(".tif") ||
      path.EndsWithIgnoreCase(".tiff")) {
    if (File::GetSize(path) < kMinTiffGeoFileSize)
      return {};

    auto result = LoadGeoTiff(path);
    if (!Load(std::move(result.first)))
      throw std::runtime_error("Failed to use geo image file");

    assert(IsDefined());

    return result.second;
  }
#endif

  if (path.EndsWithIgnoreCase(".jpg") ||
      path.EndsWithIgnoreCase(".jpeg") ||
      path.EndsWithIgnoreCase(".jfif") ||
      path.EndsWithIgnoreCase(".png")) {
    if (!LoadFile(path))
      throw std::runtime_error("Failed to use geo image file");

    assert(IsDefined());

    const auto base = path.GetBase();
    if (base == nullptr)
      throw std::runtime_error("Unsupported geo image file");

    return ParseTileBounds(base.c_str());
  }

  throw std::runtime_error("Unsupported geo image file");
}
