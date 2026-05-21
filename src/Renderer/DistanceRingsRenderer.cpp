// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DistanceRingsRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Look/MapLook.hpp"
#include "util/StringFormat.hpp"

#include <span>

static void
DrawRingLabel(Canvas &canvas, const char *text, PixelPoint p) noexcept
{
  canvas.SetBackgroundTransparent();

  // 8-direction white halo to visually cut the ring line behind the text
  canvas.SetTextColor(COLOR_WHITE);
  static constexpr int kOffset = 2;
  canvas.DrawText({p.x - kOffset, p.y - kOffset}, text);
  canvas.DrawText({p.x,           p.y - kOffset}, text);
  canvas.DrawText({p.x + kOffset, p.y - kOffset}, text);
  canvas.DrawText({p.x - kOffset, p.y           }, text);
  canvas.DrawText({p.x + kOffset, p.y           }, text);
  canvas.DrawText({p.x - kOffset, p.y + kOffset}, text);
  canvas.DrawText({p.x,           p.y + kOffset}, text);
  canvas.DrawText({p.x + kOffset, p.y + kOffset}, text);

  canvas.SetTextColor(COLOR_BLACK);
  canvas.DrawText(p, text);
}

static void
FormatRingDistance(char *buf, size_t buf_size, double distance_m) noexcept
{
  double km = distance_m / 1000.0;
  if (km == (double)(int)km)
    StringFormat(buf, buf_size, "%d km", (int)km);
  else
    StringFormat(buf, buf_size, "%.1f km", km);
}

// Ring radius presets in meters for each zoom level tier
static constexpr double kRingsVeryNarrow[] = {
  1000, 2000, 3000, 4000, 5000, 7500, 10000,
};
static constexpr double kRingsNarrow[] = {
  2500, 5000, 7500, 10000, 15000, 20000, 25000, 30000, 40000, 50000, 100000,
};
static constexpr double kRingsMedium[] = {
  5000, 10000, 15000, 20000, 25000, 30000, 40000, 50000, 100000,
};
static constexpr double kRingsWide[] = {
  10000, 20000, 30000, 40000, 50000, 75000, 100000,
};
static constexpr double kRingsVeryWide[] = {
  50000, 100000,
};

static std::span<const double>
SelectRingSet(double screen_distance_m) noexcept
{
  if (screen_distance_m <= 8000.0) // 1km steps
    return kRingsVeryNarrow;
  if (screen_distance_m <= 25000.0) // 2.5km steps
    return kRingsNarrow;
  if (screen_distance_m <= 50000.0) // 5km steps
    return kRingsMedium;
  if (screen_distance_m <= 150000.0) // 10km steps
    return kRingsWide;
  return kRingsVeryWide; // 50km steps
}

void
DrawDistanceRings(Canvas &canvas,
                  const WindowProjection &projection,
                  const GeoPoint &aircraft_pos,
                  const MapLook &look) noexcept
{
  if (!projection.IsValid() || !aircraft_pos.IsValid())
    return;

  const auto rings = SelectRingSet(projection.GetScreenDistanceMeters());
  const PixelPoint aircraft_px = projection.GeoToScreen(aircraft_pos);
  const PixelRect screen_rect = projection.GetScreenRect();

  canvas.Select(look.distance_rings_pen);
  canvas.SelectHollowBrush();

  const Font &font = *look.overlay.overlay_font;
  canvas.Select(font);
  const int font_height = (int)font.GetHeight();

  for (double radius_m : rings) {
    const unsigned radius_px = projection.GeoToScreenDistance(radius_m);

    // Skip rings too small to be useful
    if (radius_px < 20)
      continue;

    canvas.DrawCircle(aircraft_px, radius_px);

    // Label at topmost point of the ring on screen
    const int label_center_x = aircraft_px.x;
    const int label_center_y = aircraft_px.y - (int)radius_px;

    char buf[16];
    FormatRingDistance(buf, sizeof(buf), radius_m);
    const PixelSize text_size = canvas.CalcTextSize(buf);

    const PixelPoint label_pos{
      label_center_x - (int)text_size.width / 2,
      label_center_y - font_height / 2,
    };

    // Only draw label if it's within screen bounds
    if (label_pos.x >= screen_rect.left &&
        label_pos.x + (int)text_size.width <= screen_rect.right &&
        label_pos.y >= screen_rect.top &&
        label_pos.y + font_height <= screen_rect.bottom) {
      DrawRingLabel(canvas, buf, label_pos);
    }
  }
}
