// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MbTilesOverlay.hpp"

#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/BufferCanvas.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace EDL {

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
  const double rad = limited * M_PI / 180.;
  const double scale = double(1u << zoom);
  const double mercator = std::asinh(std::tan(rad));
  return (1. - mercator / M_PI) / 2. * scale;
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
  const double mercator = M_PI * (1. - 2. * double(row) / scale);
  return std::atan(std::sinh(mercator)) * 180. / M_PI;
}

MbTilesOverlay::MbTilesOverlay(Path path, std::string _label, uint8_t _alpha)
  :database(path), label(std::move(_label)), alpha(_alpha)
{
  cached_zoom = database.GetMetadata().max_zoom;
}

unsigned
MbTilesOverlay::SelectZoom(const WindowProjection &projection) const noexcept
{
  const auto &screen_bounds = projection.GetScreenBounds();
  const double longitude_width = std::max(screen_bounds.GetWidth().Degrees(), 0.01);
  const double target = std::log2(double(projection.GetScreenSize().width) * 360. /
                                  (256. * longitude_width));
  const unsigned rounded = unsigned(std::clamp<int>(int(std::lround(target)),
                                                    database.GetMetadata().min_zoom,
                                                    database.GetMetadata().max_zoom));
  return rounded;
}

Bitmap
MbTilesOverlay::LoadTile(unsigned zoom, unsigned column, unsigned row)
{
  const unsigned scale = 1u << zoom;
  const unsigned tms_row = scale - 1u - row;
  auto raw_tile = database.LoadTile(zoom, column, tms_row);

  Bitmap bitmap;
  if (!bitmap.Load(std::span{raw_tile}))
    throw std::runtime_error("Failed to decode MBTiles tile image");
  return bitmap;
}

bool
MbTilesOverlay::IsInside(GeoPoint p) const noexcept
{
  const auto &bounds = database.GetMetadata().bounds;
  return !bounds.IsValid() || bounds.IsInside(p);
}

void
MbTilesOverlay::Draw(Canvas &canvas, const WindowProjection &projection) noexcept
{
  const auto &screen_bounds = projection.GetScreenBounds();
  if (database.GetMetadata().bounds.IsValid() &&
      !database.GetMetadata().bounds.Overlaps(screen_bounds))
    return;

  const unsigned zoom = SelectZoom(projection);
  if (zoom != cached_zoom) {
    cache.clear();
    cached_zoom = zoom;
  }

  const unsigned scale = 1u << zoom;
  const double west = screen_bounds.GetWest().Degrees();
  const double east = screen_bounds.GetEast().Degrees();
  const double north = screen_bounds.GetNorth().Degrees();
  const double south = screen_bounds.GetSouth().Degrees();

  const int min_column = std::max(0, int(std::floor(LongitudeToTileX(west, zoom))));
  const int max_column = std::min(int(scale) - 1, int(std::floor(LongitudeToTileX(east, zoom))));
  const int min_row = std::max(0, int(std::floor(LatitudeToTileY(north, zoom))));
  const int max_row = std::min(int(scale) - 1, int(std::floor(LatitudeToTileY(south, zoom))));

  for (int row = min_row; row <= max_row; ++row) {
    for (int column = min_column; column <= max_column; ++column) {
      const TileKey key{zoom, unsigned(column), unsigned(row)};
      auto [it, inserted] = cache.emplace(key, Bitmap{});
      if (inserted) {
        try {
          it->second = LoadTile(zoom, column, row);
        } catch (const std::exception &) {
          cache.erase(it);
          continue;
        }
      }

      if (!it->second.IsDefined())
        continue;

      const GeoPoint north_west{Angle::Degrees(TileXToLongitude(column, zoom)),
                                Angle::Degrees(TileYToLatitude(row, zoom))};
      const GeoPoint south_east{Angle::Degrees(TileXToLongitude(column + 1, zoom)),
                                Angle::Degrees(TileYToLatitude(row + 1, zoom))};

      const auto p1 = projection.GeoToScreen(north_west);
      const auto p2 = projection.GeoToScreen(south_east);
      const int left = std::min(p1.x, p2.x);
      const int right = std::max(p1.x, p2.x);
      const int top = std::min(p1.y, p2.y);
      const int bottom = std::max(p1.y, p2.y);

      if (right <= left || bottom <= top)
        continue;

      const PixelPoint dest_position{left, top};
      const PixelSize dest_size{unsigned(right - left), unsigned(bottom - top)};

#ifdef ENABLE_OPENGL
      canvas.Stretch(dest_position, dest_size, it->second);
#else
      BufferCanvas buffer;
      buffer.Create(canvas, it->second.GetSize());
      buffer.Copy(it->second);
      canvas.AlphaBlend(dest_position, dest_size,
                        buffer, {0, 0}, it->second.GetSize(),
                        alpha);
#endif
    }
  }
}

} // namespace EDL
