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
#include <charconv>
#include <cmath>
#include <string_view>

#ifdef USE_GEOTIFF
#include "LibTiff.hpp"
#endif

#include <stdexcept>

using namespace GeoBitmap;

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

  auto parse = [](std::string_view value) {
    uint32_t result = 0;
    const auto [end, error] = std::from_chars(value.begin(), value.end(), result);
    if (error != std::errc{} || end != value.end())
      throw std::runtime_error("Unsupported geo image file");

    return result;
  };

  const auto zoom_value = parse(zoom_string);
  const auto x = parse(x_string);
  const auto y = parse(y_string);
  if (zoom_value == 0 || zoom_value > MAX_TILE_ZOOM)
    throw std::runtime_error("Unsupported geo image file");

  const auto tile_count = uint32_t{1} << zoom_value;
  if (x >= tile_count || y >= tile_count)
    throw std::runtime_error("Unsupported geo image file");

  const auto zoom = uint16_t(zoom_value);
  return GeoBitmap::GetGeoQuadrilateral({zoom, x, y});
}

GeoQuadrilateral
Bitmap::LoadGeoFile(Path path)
{
  if (!File::Exists(path))
    throw std::runtime_error("Geo image file not found");

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
