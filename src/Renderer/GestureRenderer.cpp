// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GestureRenderer.hpp"
#include "Look/GestureLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "Math/FastMath.hpp"

#ifdef ENABLE_OPENGL
#include "BoxShadowRenderer.hpp"
#include "Hardware/CPU.hpp"
#include "ui/canvas/opengl/RoundLines.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <new>
#include <vector>

/**
 * Minimum extent of the trail in pixels.  Without it, a tap would
 * flash a dot.
 */
static constexpr int MIN_TRAIL_EXTENT = 2;

/**
 * Number of jitter damping passes applied before and after
 * resampling.
 */
static constexpr unsigned DAMP_PASSES = 2;

/**
 * The distance between the points the trail is drawn from.  Short
 * segments would make the direction of each segment depend on the
 * pixel rounding of its ends, and a wide line drawn through them
 * looks frayed.
 */
[[gnu::const]]
static unsigned
GetTrailSpacing(unsigned width) noexcept
{
  return std::max(6u, width);
}

/**
 * Has the pointer moved far enough to draw a trail?
 */
[[gnu::pure]]
static bool
HasVisibleExtent(std::span<const PixelPoint> points) noexcept
{
  const PixelPoint first = points.front();

  for (const auto &p : points) {
    const auto d = p - first;
    if (compare_squared(d.x, d.y, MIN_TRAIL_EXTENT) == 1)
      return true;
  }

  return false;
}

/**
 * Damp the jitter of the touch screen; the first and the last point
 * are kept, so the trail still ends exactly at the finger.
 */
static void
DampJitter(std::span<const PixelPoint> src,
           std::vector<PixelPoint> &dest)
{
  dest.clear();
  dest.reserve(src.size());

  dest.push_back(src.front());

  for (std::size_t i = 1; i + 1 < src.size(); ++i)
    dest.emplace_back((src[i - 1].x + 2 * src[i].x + src[i + 1].x) / 4,
                      (src[i - 1].y + 2 * src[i].y + src[i + 1].y) / 4);

  dest.push_back(src.back());
}

/**
 * Distribute points evenly along the recorded path.
 */
static void
Resample(std::span<const PixelPoint> src, unsigned spacing,
         std::vector<PixelPoint> &dest)
{
  dest.clear();
  dest.reserve(src.size());

  dest.push_back(src.front());

  double carry = 0;

  for (std::size_t i = 0; i + 1 < src.size(); ++i) {
    const PixelPoint a = src[i];
    const auto d = src[i + 1] - a;
    const double length = std::hypot(double(d.x), double(d.y));
    if (length <= 0)
      continue;

    double position = spacing - carry;
    for (; position <= length; position += spacing) {
      const double t = position / length;
      dest.emplace_back(a.x + int(d.x * t), a.y + int(d.y * t));
    }

    carry = length - (position - spacing);
  }

  if (dest.back() != src.back())
    dest.push_back(src.back());
}

/**
 * Convert the recorded points to an evenly spaced smooth line.
 */
static void
BuildTrail(std::span<const PixelPoint> src, unsigned spacing,
           std::vector<BulkPixelPoint> &dest)
{
  std::vector<PixelPoint> a, b;

  DampJitter(src, a);
  for (unsigned i = 1; i < DAMP_PASSES; ++i) {
    DampJitter(a, b);
    a.swap(b);
  }

  Resample(a, spacing, b);

  for (unsigned i = 0; i < DAMP_PASSES; ++i) {
    DampJitter(b, a);
    a.swap(b);
  }

  dest.assign(b.begin(), b.end());
}

#ifdef ENABLE_OPENGL

/**
 * Draw the soft shadow of a line, one #RoundLines per layer.
 *
 * @param masked leave the pixels marked by RoundLines::DrawMask()
 * alone
 */
static void
DrawShadow(std::span<const BulkPixelPoint> line, float radius,
           const BoxShadowStyle &style, bool masked)
{
  for (const auto &layer : style.layers) {
    if (layer.alpha == 0)
      continue;

    RoundLines lines(layer.GetScaledBlur());
    lines.AddLine(line, radius + layer.GetScaledSpread());
    lines.Draw(COLOR_BLACK.WithAlpha(layer.alpha), masked);
  }
}

#else /* !ENABLE_OPENGL */

/**
 * Draw a filled circle in the given colour.
 */
static void
DrawDot(Canvas &canvas, PixelPoint center, unsigned radius,
        Color color) noexcept
{
  canvas.Select(Pen(1, color));
  canvas.Select(Brush(color));
  canvas.DrawCircle(center, radius);
}

/**
 * Draw the line with round ends.
 */
static void
DrawTrail(Canvas &canvas, std::span<const BulkPixelPoint> line,
          unsigned width, Color color) noexcept
{
  canvas.SelectHollowBrush();
  canvas.Select(Pen(width, color));
  canvas.DrawPolyline(line.data(), unsigned(line.size()));

  const auto &first = line.front(), &last = line.back();
  DrawDot(canvas, {first.x, first.y}, width / 2, color);
  DrawDot(canvas, {last.x, last.y}, width / 2, color);
}

#endif /* !ENABLE_OPENGL */

void
GestureRenderer::Draw([[maybe_unused]] Canvas &canvas,
                      const GestureLook &look,
                      std::span<const PixelPoint> points,
                      bool valid) noexcept
try {
  if (points.size() < 2 || !HasVisibleExtent(points))
    return;

  std::vector<BulkPixelPoint> line;
  BuildTrail(points, GetTrailSpacing(look.width), line);
  if (line.size() < 2)
    return;

  const Color color = valid ? look.color : look.invalid_color;

#ifdef ENABLE_OPENGL
  const float line_radius = look.width / 2.f;

  RoundLines trail;
  trail.AddLine(line, line_radius);

  /* a soft shadow lifts the line off the map ("elevation") and sets
     it off light backgrounds; too costly for a slow CPU */
  if (!IsSlowCPU()) {
    /* the map, not the shadow, shows through a translucent line */
    const bool translucent = !color.IsOpaque();
    if (translucent)
      trail.DrawMask();

    DrawShadow(line, line_radius, BoxShadowStyle::FLOATING, translucent);
  }

  trail.Draw(color);
#else
  DrawTrail(canvas, line, look.width, color);
#endif
} catch (const std::bad_alloc &) {
  /* out of memory: skip the trail, the next frame draws it again */
}
