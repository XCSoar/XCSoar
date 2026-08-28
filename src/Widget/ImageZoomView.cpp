// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ImageZoomView.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm>

namespace ImageZoomView {

namespace {

[[gnu::const]]
static double
GetImageScale(const PixelSize canvas_size, const PixelSize bitmap_size,
              const double zoom_factor) noexcept
{
  if (bitmap_size.width == 0 || bitmap_size.height == 0 ||
      canvas_size.width == 0 || canvas_size.height == 0)
    return 1.0;

  const double fit_scale = std::min(
    static_cast<double>(canvas_size.width) / bitmap_size.width,
    static_cast<double>(canvas_size.height) / bitmap_size.height);
  return fit_scale * ClampZoomFactor(zoom_factor);
}

[[gnu::const]]
static double
CanvasToBitmapAxis(const double bmp_size, const double canvas_size,
                   const double scale, const double view_pos,
                   const double anchor) noexcept
{
  const double bitmap_pos = bmp_size * scale <= canvas_size
    /* the whole image is visible, centred in the canvas */
    ? (anchor - (canvas_size - bmp_size * scale) * 0.5) / scale
    : view_pos + anchor / scale;

  return std::clamp(bitmap_pos, 0.0, bmp_size);
}

static void
MoveViewAxis(const double bmp_size, const double canvas_size,
             const double scale, const double bitmap_pos,
             const double anchor, double &view_pos) noexcept
{
  if (bmp_size * scale <= canvas_size) {
    view_pos = 0;
    return;
  }

  const double visible = canvas_size / scale;
  view_pos = std::clamp(bitmap_pos - anchor / scale, 0.0,
                        std::max(0.0, bmp_size - visible));
}

} // namespace

DoublePoint2D
CanvasToBitmap(const PixelPoint p, const DoublePoint2D view_pos,
               const PixelSize canvas_size, const PixelSize bitmap_size,
               const double zoom_factor) noexcept
{
  const double scale = GetImageScale(canvas_size, bitmap_size, zoom_factor);

  return {
    CanvasToBitmapAxis(bitmap_size.width, canvas_size.width, scale,
                       view_pos.x, p.x),
    CanvasToBitmapAxis(bitmap_size.height, canvas_size.height, scale,
                       view_pos.y, p.y),
  };
}

void
MoveViewTo(const DoublePoint2D bitmap_pos, const PixelPoint anchor,
           DoublePoint2D &view_pos,
           const PixelSize canvas_size, const PixelSize bitmap_size,
           const double zoom_factor) noexcept
{
  if (bitmap_size.width == 0 || bitmap_size.height == 0 ||
      canvas_size.width == 0 || canvas_size.height == 0)
    return;

  const double scale = GetImageScale(canvas_size, bitmap_size, zoom_factor);

  MoveViewAxis(bitmap_size.width, canvas_size.width, scale,
               bitmap_pos.x, anchor.x, view_pos.x);
  MoveViewAxis(bitmap_size.height, canvas_size.height, scale,
               bitmap_pos.y, anchor.y, view_pos.y);
}

void
AdjustImageViewOnZoomChange(const double old_zoom_factor,
                            const double new_zoom_factor,
                            DoublePoint2D &view_pos,
                            const PixelSize canvas_size,
                            const PixelSize bitmap_size) noexcept
{
  if (bitmap_size.width == 0 || bitmap_size.height == 0 ||
      canvas_size.width == 0 || canvas_size.height == 0)
    return;

  if (IsFitZoomFactor(new_zoom_factor)) {
    view_pos = {0, 0};
    return;
  }

  const double old_scale = GetImageScale(canvas_size, bitmap_size,
                                         old_zoom_factor);
  const double new_scale = GetImageScale(canvas_size, bitmap_size,
                                         new_zoom_factor);

  const double bmp_w = bitmap_size.width;
  const double bmp_h = bitmap_size.height;
  const double centre_x = canvas_size.width * 0.5;
  const double centre_y = canvas_size.height * 0.5;

  const double focal_x =
    CanvasToBitmapAxis(bmp_w, canvas_size.width, old_scale, view_pos.x,
                       centre_x);
  const double focal_y =
    CanvasToBitmapAxis(bmp_h, canvas_size.height, old_scale, view_pos.y,
                       centre_y);

  MoveViewAxis(bmp_w, canvas_size.width, new_scale, focal_x, centre_x,
               view_pos.x);
  MoveViewAxis(bmp_h, canvas_size.height, new_scale, focal_y, centre_y,
               view_pos.y);
}

void
PaintZoomedBitmap(Canvas &canvas, const Bitmap &bitmap,
                  const double zoom_factor, DoublePoint2D &view_pos,
                  PixelPoint &pending_offset) noexcept
{
  const PixelSize bmp_size = bitmap.GetSize();
  const PixelSize canvas_size{unsigned(canvas.GetWidth()),
                              unsigned(canvas.GetHeight())};
  if (bmp_size.width == 0 || bmp_size.height == 0 ||
      canvas_size.width == 0 || canvas_size.height == 0)
    return;

  const double scale = GetImageScale(canvas_size, bmp_size, zoom_factor);

  PixelPoint screen_pos, src_pos;
  PixelSize screen_size, src_size;

  const double scaled_width = bmp_size.width * scale;
  if (scaled_width <= canvas_size.width) {
    view_pos.x = 0;
    src_pos.x = 0;
    src_size.width = bmp_size.width;
    screen_pos.x = (int(canvas_size.width) - int(scaled_width)) / 2;
    screen_size.width = unsigned(scaled_width);
  } else {
    const double visible_width = canvas_size.width / scale;
    view_pos.x = IsFitZoomFactor(zoom_factor)
      ? (bmp_size.width - visible_width) / 2
      : view_pos.x + pending_offset.x / scale;
    view_pos.x = std::clamp(view_pos.x, 0.0, bmp_size.width - visible_width);
    src_pos.x = int(view_pos.x);
    src_size.width = unsigned(visible_width);
    screen_pos.x = 0;
    screen_size.width = canvas_size.width;
  }

  const double scaled_height = bmp_size.height * scale;
  if (scaled_height <= canvas_size.height) {
    view_pos.y = 0;
    src_pos.y = 0;
    src_size.height = bmp_size.height;
    screen_pos.y = (int(canvas_size.height) - int(scaled_height)) / 2;
    screen_size.height = unsigned(scaled_height);
  } else {
    const double visible_height = canvas_size.height / scale;
    view_pos.y = IsFitZoomFactor(zoom_factor)
      ? (bmp_size.height - visible_height) / 2
      : view_pos.y + pending_offset.y / scale;
    view_pos.y = std::clamp(view_pos.y, 0.0,
                            bmp_size.height - visible_height);
    src_pos.y = int(view_pos.y);
    src_size.height = unsigned(visible_height);
    screen_pos.y = 0;
    screen_size.height = canvas_size.height;
  }

  pending_offset = {};
  canvas.Stretch(screen_pos, screen_size, bitmap, src_pos, src_size);
}

} // namespace ImageZoomView
