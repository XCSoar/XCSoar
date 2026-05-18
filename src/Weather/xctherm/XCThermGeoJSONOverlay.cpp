// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermGeoJSONOverlay.hpp"

#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#include "LogFile.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

void
XCThermGeoJSONOverlay::SetForecast(
    XCThermGeoJSON::ForecastLayer &&_forecast,
    const char *_label) noexcept
{
  const std::lock_guard lock{mutex};
  forecast = std::move(_forecast);
  label = _label != nullptr ? _label : "XCTherm";
  LogFmt("xctherm overlay: loaded {} bands, {} polygons",
         forecast.bands.size(), forecast.TotalPolygons());
}

bool
XCThermGeoJSONOverlay::HasData() const noexcept
{
  const std::lock_guard lock{mutex};
  return !forecast.IsEmpty();
}

const char *
XCThermGeoJSONOverlay::GetLabel() const noexcept
{
  return "XCTherm";
}

bool
XCThermGeoJSONOverlay::IsInside([[maybe_unused]] GeoPoint p) const noexcept
{
  /* For now, always return true — the forecast covers a large region.
     A bounding-box check would be a future optimization. */
  return HasData();
}

Color
XCThermGeoJSONOverlay::WindToColor(double min_ms, double max_ms) noexcept
{
  /*
   * Official XCTherm color scale (from legend):
   *
   *  < -3.0 m/s   dark navy     (extreme sink)
   *   -3.0 m/s    blue          #0000C8
   *   -2.0 m/s    bright cyan   #00D0FF
   *   -1.0 m/s    sky blue      #60E0FF
   *   -0.5 m/s    light blue    #A0E8FF
   *   -0.2 m/s    pale blue     #C8F0FF (neutral sink edge)
   *   +0.2 m/s    cream/beige   #F0F0C0 (neutral lift edge)
   *   +0.5 m/s    yellow        #FFFF00
   *   +1.0 m/s    gold/amber    #FFD000
   *   +2.0 m/s    orange        #FFA000
   *   +3.0 m/s    red-orange    #FF4000
   *   +4.0 m/s    red           #FF0000
   *   > +4.0 m/s  purple        #A020F0 (extreme lift)
   */

  const double mid = (min_ms + max_ms) / 2.0;

  if (mid <= -3.0)  return Color(0x00, 0x00, 0xC8);  // blue
  if (mid <= -2.0)  return Color(0x00, 0xD0, 0xFF);  // bright cyan
  if (mid <= -1.0)  return Color(0x60, 0xE0, 0xFF);  // sky blue
  if (mid <= -0.5)  return Color(0xA0, 0xE8, 0xFF);  // light blue
  if (mid <= -0.2)  return Color(0xC8, 0xF0, 0xFF);  // pale blue
  if (mid <= +0.2)  return Color(0xF0, 0xF0, 0xC0);  // cream/beige
  if (mid <= +0.5)  return Color(0xFF, 0xFF, 0x00);  // yellow
  if (mid <= +1.0)  return Color(0xFF, 0xD0, 0x00);  // gold/amber
  if (mid <= +2.0)  return Color(0xFF, 0xA0, 0x00);  // orange
  if (mid <= +3.0)  return Color(0xFF, 0x40, 0x00);  // red-orange
  if (mid <= +4.0)  return Color(0xFF, 0x00, 0x00);  // red
  return Color(0xA0, 0x20, 0xF0);                    // purple
}

void
XCThermGeoJSONOverlay::Draw(Canvas &canvas,
                             const WindowProjection &projection) noexcept
{
  const std::lock_guard lock{mutex};

  if (forecast.IsEmpty())
    return;

  /* Enable alpha blending for the entire overlay draw */
  const ScopeAlphaBlend alpha_blend;

  /* Temporary buffer for screen-space polygon points.
     We reuse this across polygons to avoid reallocation. */
  std::vector<BulkPixelPoint> screen_points;
  screen_points.reserve(256);

  const auto screen_rect = projection.GetScreenRect();

  for (const auto &band : forecast.bands) {
    /* Set color for this wind band.
     * Alpha ramps down with intensity so the underlying map stays
     * readable through the stronger lift/sink colors (which would
     * otherwise saturate). |mid| = 0  → 140, |mid| = 4 → 68. */
    const Color color = WindToColor(band.min_ms, band.max_ms);
    const double abs_mid = std::abs((band.min_ms + band.max_ms) / 2.0);
    int alpha = (int)std::lround(140.0 - 18.0 * abs_mid);
    if (alpha < 60) alpha = 60;
    if (alpha > 150) alpha = 150;
    const Color fill_color = ColorWithAlpha(color, (uint8_t)alpha);

    Brush brush(fill_color);
    canvas.Select(brush);
    canvas.SelectNullPen();

    for (const auto &polygon : band.polygons) {
      /* We only draw the exterior ring (ring 0).
         Holes are rare in weather contour data. */
      if (polygon.empty())
        continue;

      const auto &ring = polygon[0];
      if (ring.size() < 3)
        continue;

      /* Quick bounding-box visibility check */
      GeoPoint bb_min = ring[0], bb_max = ring[0];
      for (const auto &pt : ring) {
        if (pt.longitude < bb_min.longitude) bb_min.longitude = pt.longitude;
        if (pt.latitude < bb_min.latitude) bb_min.latitude = pt.latitude;
        if (pt.longitude > bb_max.longitude) bb_max.longitude = pt.longitude;
        if (pt.latitude > bb_max.latitude) bb_max.latitude = pt.latitude;
      }

      /* Check if bounding box intersects the screen */
      auto tl = projection.GeoToScreen(GeoPoint(bb_min.longitude, bb_max.latitude));
      auto br = projection.GeoToScreen(GeoPoint(bb_max.longitude, bb_min.latitude));

      if (br.x < screen_rect.left || tl.x > screen_rect.right ||
          br.y < screen_rect.top || tl.y > screen_rect.bottom)
        continue;

      /* Project all points to screen coordinates */
      screen_points.clear();
      for (const auto &pt : ring) {
        auto sp = projection.GeoToScreen(pt);
        screen_points.push_back(BulkPixelPoint{sp.x, sp.y});
      }

      canvas.DrawPolygon(screen_points.data(),
                         (unsigned)screen_points.size());
    }
  }
}
