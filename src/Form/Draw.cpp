// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Draw.hpp"

void
WndOwnerDrawFrame::OnPaint(Canvas &canvas) noexcept
{
  if (mOnPaintCallback == nullptr)
    return;

  mOnPaintCallback(canvas, GetClientRect());
}
