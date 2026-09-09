// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ContourOverlay.hpp"
#include "SkySightLimits.hpp"

#include "Projection/WindowProjection.hpp"
#include "system/Path.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

#ifdef USE_GEOTIFF

namespace {

/**
 * How many screen pixels one raster pixel should cover.  The map
 * magnifies the patch with bilinear filtering, so keeping a raster pixel
 * near this size leaves band edges smooth without rendering more than the
 * display can show.
 */
constexpr double SCREEN_PIXELS_PER_RASTER_PIXEL = 2;

/**
 * How far beyond the screen the patch reaches, as a fraction of the
 * screen.  Small pans then reuse the rendered patch instead of
 * re-contouring on every frame.
 */
constexpr double WINDOW_MARGIN = 0.25;

[[gnu::pure]] bool
Contains(const SkySight::ContourWindow &outer,
         const SkySight::ContourWindow &inner) noexcept
{
  return outer.upsample == inner.upsample &&
    outer.x <= inner.x && outer.y <= inner.y &&
    outer.x + outer.width >= inner.x + inner.width &&
    outer.y + outer.height >= inner.y + inner.height;
}

} // namespace

SkySightContourOverlay::SkySightContourOverlay(
  Path path, const std::map<float, SkySight::LegendColor> &legend)
  :source(SkySight::ReadFieldImage(path)),
   palette(legend),
   bounds(source.GetBounds()),
   label((path.GetBase() != nullptr ? path.GetBase() : path).c_str())
{
  if (palette.empty())
    throw std::runtime_error("SkySight layer has no legend");
}

bool
SkySightContourOverlay::IsInside(GeoPoint p) const noexcept
{
  return bounds.IsValid() && bounds.IsInside(p);
}

SkySight::ContourWindow
SkySightContourOverlay::SelectWindow(
  const WindowProjection &projection) const noexcept
{
  SkySight::ContourWindow window;

  auto screen_bounds = projection.GetScreenBounds();
  if (!screen_bounds.IsValid())
    return window;

  screen_bounds = screen_bounds.Scale(1 + 2 * WINDOW_MARGIN);
  if (!screen_bounds.Overlaps(bounds))
    return window;

  const auto north_west = source.ProjectClamped(screen_bounds.GetNorthWest());
  const auto south_east = source.ProjectClamped(screen_bounds.GetSouthEast());
  if (south_east.x < north_west.x || south_east.y < north_west.y)
    return window;

  window.x = unsigned(north_west.x);
  window.y = unsigned(north_west.y);
  window.width = unsigned(south_east.x) - window.x + 1;
  window.height = unsigned(south_east.y) - window.y + 1;

  /* Match the raster to the display: one raster pixel per couple of
     screen pixels, so the contours are as sharp as the screen can show
     and no sharper. */
  const double visible_degrees =
    std::max(projection.GetScreenBounds().GetWidth().Degrees(), 1e-6);
  const double samples_across = visible_degrees / source.longitude_step;
  const double screen_pixels_per_sample =
    double(projection.GetScreenSize().width) / std::max(samples_across, 1e-6);

  const auto wanted = long(std::lround(screen_pixels_per_sample /
                                       SCREEN_PIXELS_PER_RASTER_PIXEL));
  window.upsample =
    unsigned(std::clamp(wanted, 1L, long(SkySight::MAX_CONTOUR_UPSAMPLE)));

  /* ...and never beyond what the raster budget allows. */
  window.upsample = std::min(window.upsample,
                             SkySight::ChooseContourUpsample(
                               window.width, window.height,
                               SkySight::MAX_CONTOUR_RASTER_AXIS,
                               SkySight::MAX_CONTOUR_RASTER_CELLS));
  return window;
}

bool
SkySightContourOverlay::Render(SkySight::ContourWindow window) noexcept
try {
  const SkySight::ContourRasterizer rasterizer{source.field, palette, window};

  const unsigned width = rasterizer.GetWidth();
  const unsigned height = rasterizer.GetHeight();
  if (width == 0 || height == 0)
    return false;

  const std::size_t pitch = std::size_t(width) * 4;
  auto data = std::make_unique<uint8_t[]>(pitch * height);
  for (unsigned y = 0; y < height; ++y)
    rasterizer.RenderRow(y, {data.get() + pitch * y, pitch});

  Bitmap bitmap;
  if (!bitmap.Load(UncompressedImage(UncompressedImage::Format::RGBA, pitch,
                                     width, height, std::move(data), false)))
    return false;

  const GeoQuadrilateral quadrilateral{
    source.GetSampleCorner(window.x, window.y),
    source.GetSampleCorner(window.x + window.width, window.y),
    source.GetSampleCorner(window.x, window.y + window.height),
    source.GetSampleCorner(window.x + window.width,
                           window.y + window.height),
  };

  cached.emplace(std::move(bitmap), quadrilateral, label.c_str());
  cached->SetAlpha(alpha);
  cached_window = window;
  return true;
} catch (...) {
  return false;
}

void
SkySightContourOverlay::Draw(Canvas &canvas,
                             const WindowProjection &projection) noexcept
{
  const auto window = SelectWindow(projection);
  if (window.width == 0 || window.height == 0)
    return;

  if ((!cached || !Contains(cached_window, window)) && !Render(window))
    return;

  cached->Draw(canvas, projection);
}

#endif
