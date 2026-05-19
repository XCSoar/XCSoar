// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProgressBarRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/Colors.hpp"
#include "Asset.hpp"

#include <algorithm>
#include <cstdint>

static constexpr unsigned
CalcProgressBarPosition(unsigned current_value,
                        unsigned min_value, unsigned max_value,
                        unsigned width) noexcept
{
  if (min_value >= max_value)
    return 0;

  const unsigned value = std::clamp(current_value, min_value, max_value);
  const uint64_t num =
      uint64_t{value - min_value} * uint64_t{width};
  return static_cast<unsigned>(num / uint64_t{max_value - min_value});
}

void
DrawSimpleProgressBar(Canvas &canvas, const PixelRect &r,
                      unsigned current_value,
                      unsigned min_value, unsigned max_value,
                      const Color *background_color) noexcept
{
  const int position =
    CalcProgressBarPosition(current_value, min_value, max_value,
                            r.GetWidth());

  const Color &bg = background_color != nullptr ? *background_color
                                                 : COLOR_WHITE;
  /* Full track first so the completed segment can sit on top without a gap. */
  canvas.DrawFilledRectangle(r, bg);

  if (position <= 0)
    return;

  auto a = r;
  a.right = a.left + position;
  if (a.right > r.right)
    a.right = r.right;

  canvas.DrawFilledRectangle(a, IsDithered() ? COLOR_BLACK : COLOR_GREEN);
}

void
DrawRoundProgressBar(Canvas &canvas, const PixelRect &r,
                     unsigned current_value,
                     unsigned min_value, unsigned max_value,
                     const Color *background_color) noexcept
{
  const unsigned position =
    CalcProgressBarPosition(current_value, min_value, max_value,
                            r.GetWidth());

  canvas.SelectNullPen();
  if (background_color != nullptr) {
    Brush bg_brush(*background_color);
    canvas.Select(bg_brush);
  } else {
    canvas.SelectWhiteBrush();
  }
  canvas.DrawRoundRectangle(r, PixelSize{r.GetHeight()});

  Brush progress_brush(IsDithered() ? COLOR_BLACK : COLOR_XCSOAR_LIGHT);
  canvas.Select(progress_brush);
  unsigned margin = r.GetHeight() / 9;
  unsigned top, bottom;
  if (position <= r.GetHeight() - 2 * margin) {
    // Use a centered "circle" for small position values. This keeps the progress
    // bar inside the background.
    unsigned center_y = r.GetHeight() / 2;
    top = center_y - position / 2;
    bottom = center_y + position / 2;
  } else {
    top = margin;
    bottom = r.GetHeight() - margin;
  }
  canvas.DrawRoundRectangle(PixelRect(margin, top, margin + position, bottom),
                            PixelSize{r.GetHeight()});
}
