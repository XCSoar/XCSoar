// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermGeoJSONOverlay.hpp"
#include "lib/fmt/ToBuffer.hxx"

#include <cstring>
#include <string>
#include "XCThermAPI.hpp"

#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif
#include "LogFile.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <vector>

void
XCThermGeoJSONOverlay::SetForecast(
    XCThermGeoJSON::ForecastLayer &&_forecast,
    const char *_label,
    const char *_parameter,
    unsigned _forecast_utc) noexcept
{
  const std::lock_guard lock{mutex};
  forecast = std::move(_forecast);
  label = _label != nullptr ? _label : "XCTherm";
  parameter = _parameter != nullptr ? _parameter : "";
  forecast_utc = _forecast_utc;
  LogFmt("xctherm overlay: loaded {} bands, {} polygons",
         forecast.bands.size(), forecast.TotalPolygons());
}

bool
XCThermGeoJSONOverlay::HasData() const noexcept
{
  const std::lock_guard lock{mutex};
  return !forecast.IsEmpty();
}

bool
XCThermGeoJSONOverlay::GetClimbAt(GeoPoint p, double &out_min_ms,
                                  double &out_max_ms) const noexcept
{
  const std::lock_guard lock{mutex};
  /* Pure geometry lives in XCThermGeoJSON::FindBandAtPoint so it can be
     unit-tested without dragging in the overlay's UI dependencies. */
  return XCThermGeoJSON::FindBandAtPoint(forecast, p, out_min_ms, out_max_ms);
}

bool
XCThermGeoJSONOverlay::FormatPointInfo(GeoPoint p, char *buffer,
                                       std::size_t size) const noexcept
{
  if (buffer == nullptr || size == 0)
    return false;

  /* Snapshot the metadata we need under the lock, then format without
     holding it (snprintf + API call must not nest the mutex). */
  std::string label_copy, parameter_copy;
  unsigned hour;
  {
    const std::lock_guard lock{mutex};
    if (forecast.IsEmpty())
      return false;
    label_copy = label;
    parameter_copy = parameter;
    hour = forecast_utc;
  }

  /* Climb value at the tapped location.
     The source data is contoured into bands server-side, so the finest
     value available is the band the point falls in; we report its
     midpoint as the representative number. The neutral band
     (−0.2…+0.2 m/s) is dropped at parse time, so a point inside no band
     is neutral → 0.0. Open-ended edge bands carry a ±1000 sentinel
     bound; for those the finite edge is the only meaningful figure. */
  double min_ms = 0, max_ms = 0;
  std::string climb;
  if (GetClimbAt(p, min_ms, max_ms)) {
    double value;
    if (min_ms <= -100.0)
      value = max_ms;
    else if (max_ms >= 100.0)
      value = min_ms;
    else
      value = (min_ms + max_ms) / 2;
    climb = std::string(FmtBuffer<32>("{:+.1f} m/s", value).c_str());
  } else {
    climb = "0.0 m/s";
  }

  std::string meta;
  if (!parameter_copy.empty()) {
    const auto summary =
      XCThermAPI::Instance().GetCachedLayerSummary(parameter_copy);
    if (summary.latest_run_date.size() == 8 &&
        summary.latest_run_hour.size() == 2) {
      const std::string &d = summary.latest_run_date;
      meta += std::string(FmtBuffer<32>(" | run {}-{}-{} {}Z",
                                        d.substr(0, 4), d.substr(4, 2),
                                        d.substr(6, 2),
                                        summary.latest_run_hour).c_str());
    }
    if (summary.latest_downloaded_at > 0) {
      const std::time_t t = (std::time_t)summary.latest_downloaded_at;
      std::tm *lt = std::localtime(&t);
      char tbuf[8];
      if (lt != nullptr && std::strftime(tbuf, sizeof(tbuf), "%H:%M", lt) > 0)
        meta += std::string(FmtBuffer<16>(" | dl {}", tbuf).c_str());
    }
  }

  const auto line = FmtBuffer<256>("{} @ {} ({:02}Z){}",
                                   climb, label_copy, hour, meta);
  std::strncpy(buffer, line.c_str(), size - 1);
  buffer[size - 1] = '\0';
  return true;
}

const char *
XCThermGeoJSONOverlay::GetLabel() const noexcept
{
  return "XCTherm";
}

bool
XCThermGeoJSONOverlay::IsInside(GeoPoint p) const noexcept
{
  const std::lock_guard lock{mutex};
  /* Only claim the location if the forecast actually covers it — so a
     tap far outside the model domain doesn't surface a bogus XCTherm
     map item reading "0.0 m/s". */
  return !forecast.IsEmpty() && forecast.bounds.IsValid() &&
         forecast.bounds.IsInside(p);
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

#ifdef ENABLE_OPENGL
  /* Enable alpha blending for the entire overlay draw */
  const ScopeAlphaBlend alpha_blend;
#endif

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
