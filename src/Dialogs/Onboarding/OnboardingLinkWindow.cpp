// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OnboardingLinkWindow.hpp"

#include "ui/canvas/Canvas.hpp"

#include <winuser.h>

OnboardingLinkWindow::OnboardingLinkWindow() noexcept = default;

unsigned
OnboardingLinkWindow::DrawLink(Canvas &canvas, std::size_t index, PixelRect rc,
                               const TCHAR *text) noexcept
{
  canvas.SetTextColor(COLOR_BLUE);
  const unsigned height = canvas.DrawFormattedText(rc, text, DT_LEFT | DT_UNDERLINE);
  canvas.SetTextColor(COLOR_BLACK);

  link_rects[index] = {rc.left, rc.top, rc.right, rc.top + int(height)};

  return height;
}

bool
OnboardingLinkWindow::OnMouseUp(PixelPoint p) noexcept
{
  for (std::size_t i = 0; i < link_rects.size(); ++i) {
    if (link_rects[i].Contains(p) && OnLinkActivated(i)) {
      Invalidate();
      return true;
    }
  }

  return PaintWindow::OnMouseUp(p);
}
