// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Widget/ScrollableLargeTextWidget.hpp"
#include "Screen/Layout.hpp"

ScrollableLargeTextWidget::ScrollableLargeTextWidget(const DialogLook &dlgLook,
                                                     const TCHAR *t)
    : LargeTextWidget(dlgLook, t), look(dlgLook), text(t)
{
}

PixelSize
ScrollableLargeTextWidget::GetMinimumSize() const noexcept
{
  return {Layout::FastScale(200), Layout::FastScale(200)};
}

PixelSize
ScrollableLargeTextWidget::GetMaximumSize() const noexcept
{
  long lines = 1;
  for (auto ch : text) {
    if (ch == _T('\n')) ++lines;
  }

  unsigned line_height = look.text_font.GetHeight();
  unsigned total_height = lines * line_height + Layout::FastScale(10);

  return {static_cast<unsigned>(Layout::FastScale(300)), total_height};
}
