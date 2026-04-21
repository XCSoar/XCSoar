// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermOverlay.hpp"

#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Color.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#include <algorithm>
#include <cmath>

namespace {

static constexpr float kZeroVerticalThreshold = 0.15f;
static constexpr int kTileEdgeSnapThreshold = 8;
static constexpr int kTileEdgeOverlapUnits = 16;

static int
ExpandTileEdgeCoordinate(int value, int extent) noexcept {
  if (value <= kTileEdgeSnapThreshold)
    return -kTileEdgeOverlapUnits;

  if (value >= extent - kTileEdgeSnapThreshold)
    return extent + kTileEdgeOverlapUnits;

  return value;
}

static Color ValueToColor(float value) {
  if (value >= 4.0f)
    return Color(150, 65, 220);   // >= +4.0
  if (value >= 3.0f)
    return Color(235, 20, 20);    // +3.0 .. +4.0
  if (value >= 2.0f)
    return Color(245, 140, 0);    // +2.0 .. +3.0
  if (value >= 1.0f)
    return Color(245, 190, 0);    // +1.0 .. +2.0
  if (value >= 0.5f)
    return Color(240, 225, 0);    // +0.5 .. +1.0
  if (value >= 0.2f)
    return Color(220, 220, 170);  // +0.2 .. +0.5
  if (value > -0.2f)
    return Color(170, 170, 145);  // -0.2 .. +0.2
  if (value > -0.5f)
    return Color(130, 120, 65);   // -0.5 .. -0.2
  if (value > -1.0f)
    return Color(110, 200, 235);  // -1.0 .. -0.5
  if (value > -2.0f)
    return Color(95, 150, 220);   // -2.0 .. -1.0
  if (value > -3.0f)
    return Color(35, 35, 220);    // -3.0 .. -2.0

  return Color(20, 20, 170);      // <= -3.0
}

static double TileXToLongitude(double x, int z) {
  return x / (double)(1 << z) * 360.0 - 180.0;
}

static double TileYToLatitude(double y, int z) {
  const double n = M_PI - 2.0 * M_PI * y / (double)(1 << z);
  return 180.0 / M_PI * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}

} // namespace

XCThermCompositeOverlay::XCThermCompositeOverlay() noexcept = default;

const char *
XCThermCompositeOverlay::GetLabel() const noexcept {
  return "XCTherm Wave";
}

bool
XCThermCompositeOverlay::IsInside(GeoPoint p) const noexcept {
  const std::lock_guard lock(mutex);

  for (const auto &tile : tiles)
    if (tile.bounds.IsInside(p))
      return true;

  return false;
}

GeoBounds
XCThermCompositeOverlay::ComputeTileBounds(XCThermTileCoord coord) noexcept {
  const GeoPoint top_left{
    Angle::Degrees(TileXToLongitude(coord.x, coord.zoom)),
    Angle::Degrees(TileYToLatitude(coord.y, coord.zoom))
  };
  const GeoPoint bottom_right{
    Angle::Degrees(TileXToLongitude(coord.x + 1, coord.zoom)),
    Angle::Degrees(TileYToLatitude(coord.y + 1, coord.zoom))
  };

  return GeoBounds(top_left, bottom_right);
}

GeoPoint
XCThermCompositeOverlay::TilePointToGeo(XCThermTileCoord coord,
                                         uint32_t extent,
                                         int x, int y) noexcept {
  const double ext = extent > 0 ? double(extent) : 4096.0;
  const double tx = double(coord.x) + double(x) / ext;
  const double ty = double(coord.y) + double(y) / ext;

  return {
    Angle::Degrees(TileYToLatitude(ty, coord.zoom)),
    Angle::Degrees(TileXToLongitude(tx, coord.zoom))
  };
}

void
XCThermCompositeOverlay::AddTile(XCThermTileCoord coord,
                                  XCThermMVT::Tile &&data) noexcept {
  const auto bounds = ComputeTileBounds(coord);

  const std::lock_guard lock(mutex);
  tiles.emplace_back(coord, std::move(data), bounds);
}

void
XCThermCompositeOverlay::Clear() noexcept {
  const std::lock_guard lock(mutex);
  tiles.clear();
}

unsigned
XCThermCompositeOverlay::GetTileCount() const noexcept {
  const std::lock_guard lock(mutex);
  return static_cast<unsigned>(tiles.size());
}

void
XCThermCompositeOverlay::Draw(Canvas &canvas,
                               const WindowProjection &projection) noexcept {
  const std::lock_guard lock(mutex);

  if (tiles.empty())
    return;

  const auto screen_bounds = projection.GetScreenBounds();

#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
#endif

  for (const auto &tile : tiles) {
    if (!tile.bounds.Overlaps(screen_bounds))
      continue;

    /* Draw polygons (wave/vertical wind areas) */
    for (const auto &poly : tile.data.polygons) {
      const float center = (poly.min_value + poly.max_value) * 0.5f;
      if (std::fabs(center) <= kZeroVerticalThreshold)
        continue;

      const Color color = ValueToColor(center);
      canvas.Select(Brush(ColorWithAlpha(color, 90)));
      canvas.SelectNullPen();

      for (const auto &ring : poly.rings) {
        if (ring.size() < 3)
          continue;

        std::vector<BulkPixelPoint> points;
        points.reserve(ring.size());

        const int extent = tile.data.extent > 0
          ? static_cast<int>(tile.data.extent) : 4096;

        for (const auto &tp : ring) {
          const int ex = ExpandTileEdgeCoordinate(tp.x, extent);
          const int ey = ExpandTileEdgeCoordinate(tp.y, extent);
          const auto gp = TilePointToGeo(tile.coord, tile.data.extent,
                                          ex, ey);
          points.push_back(projection.GeoToScreen(gp));
        }

        if (points.size() >= 3)
          canvas.DrawPolygon(points.data(),
                             static_cast<unsigned>(points.size()));
      }
    }

    /* Draw wind direction arrows (point features) */
    canvas.Select(Pen(1, COLOR_BLACK.WithAlpha(60)));
    for (const auto &p : tile.data.points) {
      if (std::fabs(p.vertical_speed) <= kZeroVerticalThreshold)
        continue;

      const auto gp = TilePointToGeo(tile.coord, tile.data.extent,
                                      p.x, p.y);
      const auto sp = projection.GeoToScreen(gp);

      const float len = std::clamp(
        std::fabs(p.vertical_speed) * 2.0f + 3.0f, 4.0f, 16.0f);
      const double rad = (p.direction_deg - 90.0) * M_PI / 180.0;
      const int dx = static_cast<int>(std::cos(rad) * len);
      const int dy = static_cast<int>(std::sin(rad) * len);

      canvas.DrawLine(sp, {sp.x + dx, sp.y + dy});
    }
  }
}
