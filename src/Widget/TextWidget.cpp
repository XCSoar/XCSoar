/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TextWidget.hpp"
#include "UIGlobals.hpp"
#include "Form/Frame.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/DialogLook.hpp"

void
TextWidget::SetText(const TCHAR *text) noexcept
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


