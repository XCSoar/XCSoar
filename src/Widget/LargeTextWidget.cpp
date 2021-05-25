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

#include "LargeTextWidget.hpp"
#include "ui/control/LargeTextWindow.hpp"
#include "ui/event/KeyCode.hpp"
#include "Look/DialogLook.hpp"

void
LargeTextWidget::SetText(const TCHAR *text) noexcept
{
  LargeTextWindow &w = (LargeTextWindow &)GetWindow();
  w.SetText(text);
}

void
LargeTextWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  LargeTextWindowStyle style;
  style.Hide();
  style.TabStop();

  auto w = std::make_unique<LargeTextWindow>();
  w->Create(parent, rc, style);
  w->SetFont(look.text_font);
  if (text != nullptr)
    w->SetText(text);

  SetWindow(std::move(w));
}

bool
LargeTextWidget::KeyPress(unsigned key_code) noexcept
{
  switch (key_code) {
  case KEY_UP:
    ((LargeTextWindow &)GetWindow()).ScrollVertically(-3);
    return true;

  case KEY_DOWN:
    ((LargeTextWindow &)GetWindow()).ScrollVertically(3);
    return true;

  default:
    return false;
  }
}
