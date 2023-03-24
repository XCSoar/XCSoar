// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProgressBar.hpp"
#include "ui/canvas/Features.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Renderer/ProgressBarRenderer.hpp"
#include "thread/Debug.hpp"

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

#if defined(EYE_CANDY) && !defined(HAVE_CLIPPING)
/* when the Canvas is clipped, we can't render rounded corners,
   because the parent's background would not be left visible then */
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
}
