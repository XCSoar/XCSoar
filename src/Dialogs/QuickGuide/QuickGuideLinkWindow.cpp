// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QuickGuideLinkWindow.hpp"

#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"

#include <winuser.h>

unsigned
QuickGuideLinkWindow::DrawLink(Canvas &canvas, std::size_t index,
                                PixelRect rc, const char *text) noexcept
{
  const bool is_focused = IsLinkFocused(index);

  if (is_focused) {
    // Draw focus highlight background - estimate height based on font
    const unsigned line_height = canvas.GetFontHeight() + Layout::GetTextPadding();
    PixelRect highlight_rc = rc;
    highlight_rc.bottom = rc.top + int(line_height);
    canvas.DrawFilledRectangle(highlight_rc, COLOR_LIGHT_GRAY);
  }

  canvas.SetTextColor(COLOR_BLUE);
  const unsigned height = canvas.DrawFormattedText(rc, text,
                                                    DT_LEFT | DT_UNDERLINE);
  canvas.SetTextColor(COLOR_BLACK);

  // Register the link rectangle for hit-testing
  RegisterLinkRect(index, {rc.left, rc.top, rc.right, rc.top + int(height)});

  return height;
}
