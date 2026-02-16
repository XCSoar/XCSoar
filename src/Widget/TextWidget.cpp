// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextWidget.hpp"
#include "UIGlobals.hpp"
#include "Form/Frame.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/DialogLook.hpp"

void
TextWidget::SetText(const char *text) noexcept
{
  WndFrame &w = (WndFrame &)GetWindow();
  w.SetText(text);
}

void
TextWidget::SetColor(Color _color) noexcept
{
  WndFrame &w = (WndFrame &)GetWindow();
  w.SetTextColor(_color);
}

PixelSize
TextWidget::GetMinimumSize() const noexcept
{
  const Font &font = UIGlobals::GetDialogLook().text_font;
  const int height = 2 * Layout::GetTextPadding() + font.GetHeight();

  return {0, height};
}

PixelSize
TextWidget::GetMaximumSize() const noexcept
{
  PixelSize size = GetMinimumSize();

  if (IsDefined()) {
    const WndFrame &w = (const WndFrame &)GetWindow();
    const unsigned text_height = 2 * Layout::GetTextPadding() + w.GetTextHeight();
    if (text_height > size.height)
      size.height = text_height;
  }

  return size;
}

void
TextWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();

  SetWindow(std::make_unique<WndFrame>(parent, UIGlobals::GetDialogLook(),
                                       rc, style));
}


