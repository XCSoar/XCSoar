// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProgressBar.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Renderer/ProgressBarRenderer.hpp"
#include "thread/Debug.hpp"
#include "Look/FontDescription.hpp"
#include "Look/Colors.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"

#include <algorithm>

void
ProgressBar::SetRange(unsigned min_value, unsigned max_value)
{
  AssertThread();

  this->min_value = min_value;
  this->max_value = max_value;
  value = 0;
  step_size = 1;
  Invalidate();
}

void
ProgressBar::SetValue(unsigned value)
{
  AssertThread();

  if (value == this->value)
    return;

  this->value = value;
  Invalidate();
}

void
ProgressBar::SetStep(unsigned size)
{
  AssertThread();

  step_size = size;
  Invalidate();
}

void
ProgressBar::Step()
{
  AssertThread();

  value += step_size;
  Invalidate();
}

void
ProgressBar::SetText(const char *text) noexcept
{
  AssertThread();

  if (text == nullptr)
    text = "";

  if (caption == text)
    return;

  caption = text;
  if (!font.IsDefined())
    font.Load(FontDescription(Layout::FontScale(10)));
  Invalidate();
}

#ifdef EYE_CANDY
/* Rounded corners leave the parent's background visible in the
   corner pixels. */
#define ROUND_PROGRESS_BAR
#endif

void
ProgressBar::OnPaint(Canvas &canvas) noexcept
{
#ifdef ROUND_PROGRESS_BAR
  DrawRoundProgressBar(canvas, canvas.GetRect(), value, min_value, max_value);
#else
  DrawSimpleProgressBar(canvas, canvas.GetRect(), value, min_value, max_value);
#endif

  if (caption.empty() || !font.IsDefined())
    return;

  canvas.Select(font);
  canvas.SetBackgroundTransparent();

  const PixelSize size = canvas.CalcTextSize(caption);
  const int x = (int(canvas.GetWidth()) - int(size.width)) / 2;
  const int y = (int(canvas.GetHeight()) - int(size.height)) / 2;

  canvas.SetTextColor(COLOR_BLACK);
  canvas.DrawText({x, y}, caption);

  /* On a black e-paper fill, redraw the part that sits on the bar
     in white.  The colour fill is light, so black stays readable. */
  if (!IsDithered() || max_value <= min_value)
    return;

  const unsigned clamped = std::clamp(value, min_value, max_value);
  unsigned fill;
#ifdef ROUND_PROGRESS_BAR
  /* Same inset as DrawRoundProgressBar, so the white slice ends
     with the fill and not past the track cap. */
  const unsigned margin = canvas.GetHeight() / 9;
  const unsigned width = canvas.GetWidth();
  if (width <= 2 * margin)
    return;
  fill = margin
    + (clamped - min_value) * (width - 2 * margin)
      / (max_value - min_value);
#else
  fill = (clamped - min_value) * canvas.GetWidth()
    / (max_value - min_value);
#endif
  if (x >= int(fill))
    return;

  canvas.SetTextColor(COLOR_WHITE);
  canvas.DrawClippedText({x, y}, unsigned(int(fill) - x),
                         std::string_view{caption});
}
