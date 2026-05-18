// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ImageZoomView.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm>
#include <cmath>

namespace ImageZoomView {

namespace {

static constexpr int zoom_factors[] = {1, 2, 4, 8, 16, 32};

[[gnu::const]]
static double
GetImageScale(const PixelSize canvas_size, const PixelSize bitmap_size,
              const int zoom_level) noexcept
{
  if (bitmap_size.width == 0 || bitmap_size.height == 0 ||
      canvas_size.width == 0 || canvas_size.height == 0)
    return 1.0;

  const double fit_scale = std::min(
    static_cast<double>(canvas_size.width) / bitmap_size.width,
    static_cast<double>(canvas_size.height) / bitmap_size.height);
  return fit_scale *
         zoom_factors[std::clamp(zoom_level, 0, max_zoom_level)];
}

[[gnu::const]]
static double
FocalOnAxis(const double bmp_size, const double canvas_size,
              const double old_scale, const int old_zoom,
              const double view_pos) noexcept
{
  if (old_zoom == 0 || bmp_size * old_scale <= canvas_size)
    return bmp_size * 0.5;

  return view_pos + (canvas_size * 0.5) / old_scale;
}

static void
RepositionAxis(const double bmp_size, const double canvas_size,
               const double new_scale, const double focal,
               double &view_pos) noexcept
{
  if (bmp_size * new_scale <= canvas_size) {
    view_pos = 0;
    return;
  }

  const double visible = canvas_size / new_scale;
  view_pos = focal - (canvas_size * 0.5) / new_scale;
  view_pos = std::clamp(view_pos, 0.0, std::max(0.0, bmp_size - visible));
}

} // namespace

void
AdjustImageViewOnZoomChange(const int old_zoom, const int new_zoom,
                            PixelPoint &view_pos,
                            const PixelSize canvas_size,
                            const PixelSize bitmap_size) noexcept
{
  if (bitmap_size.width == 0 || bitmap_size.height == 0 ||
      canvas_size.width == 0 || canvas_size.height == 0)
    return;

  if (new_zoom == 0) {
    view_pos = {0, 0};
    return;
  }

  const double old_scale = GetImageScale(canvas_size, bitmap_size, old_zoom);
  const double new_scale = GetImageScale(canvas_size, bitmap_size, new_zoom);

  const double bmp_w = bitmap_size.width;
  const double bmp_h = bitmap_size.height;

  double view_x = view_pos.x;
  double view_y = view_pos.y;

  const double focal_x =
    FocalOnAxis(bmp_w, canvas_size.width, old_scale, old_zoom, view_x);
  const double focal_y =
    FocalOnAxis(bmp_h, canvas_size.height, old_scale, old_zoom, view_y);

  RepositionAxis(bmp_w, canvas_size.width, new_scale, focal_x, view_x);
  RepositionAxis(bmp_h, canvas_size.height, new_scale, focal_y, view_y);

  view_pos.x = int(std::lround(view_x));
  view_pos.y = int(std::lround(view_y));
}

void
PaintZoomedBitmap(Canvas &canvas, const Bitmap &bitmap, const int zoom,
                  PixelPoint &view_pos, PixelPoint &pending_offset) noexcept
{
  PixelSize img_size = bitmap.GetSize();
  if (img_size.width == 0 || img_size.height == 0 ||
      canvas.GetWidth() == 0 || canvas.GetHeight() == 0)
    return;

  const double scale = GetImageScale(
    {unsigned(canvas.GetWidth()), unsigned(canvas.GetHeight())},
    img_size, zoom);

  PixelPoint screen_pos;
  PixelSize screen_size;

  const double scaled_width = img_size.width * scale;
  if (scaled_width <= canvas.GetWidth()) {
    view_pos.x = 0;
    screen_pos.x = (canvas.GetWidth() - int(scaled_width)) / 2;
    screen_size.width = unsigned(scaled_width);
  } else {
    const double visible_width = canvas.GetWidth() / scale;
    view_pos.x = zoom == 0
      ? int((img_size.width - visible_width) / 2)
      : int(view_pos.x + pending_offset.x / scale);
    view_pos.x = std::clamp(view_pos.x, 0,
                            int(img_size.width - visible_width));
    img_size.width = unsigned(visible_width);
    screen_pos.x = 0;
    screen_size.width = unsigned(canvas.GetWidth());
  }

  const double scaled_height = img_size.height * scale;
  if (scaled_height <= canvas.GetHeight()) {
    view_pos.y = 0;
    screen_pos.y = (canvas.GetHeight() - int(scaled_height)) / 2;
    screen_size.height = unsigned(scaled_height);
  } else {
    const double visible_height = canvas.GetHeight() / scale;
    view_pos.y = zoom == 0
      ? int((img_size.height - visible_height) / 2)
      : int(view_pos.y + pending_offset.y / scale);
    view_pos.y = std::clamp(view_pos.y, 0,
                            int(img_size.height - visible_height));
    img_size.height = unsigned(visible_height);
    screen_pos.y = 0;
    screen_size.height = unsigned(canvas.GetHeight());
  }

  pending_offset = {};
  canvas.Stretch(screen_pos, screen_size, bitmap, view_pos, img_size);
}

} // namespace ImageZoomView
