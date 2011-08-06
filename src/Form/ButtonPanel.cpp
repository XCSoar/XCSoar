/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Form/ButtonPanel.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Layout.hpp"

ButtonPanel::ButtonPanel(ContainerWindow &_parent, const DialogLook &_look)
  :parent(_parent), look(_look) {
  style.tab_stop();
  Resized(parent.get_client_rect());
}

ButtonPanel::~ButtonPanel()
{
  for (unsigned i = 0; i < buttons.size(); ++i)
    delete buttons[i];
}

PixelRect
ButtonPanel::GetButtonRect(unsigned i) const
{
  unsigned margin = Layout::Scale(1);
  PixelRect r;
  if (vertical) {
    const unsigned height = Layout::Scale(30);
    r.left = rc.left;
    r.top = rc.top + i * height;
    r.right = rc.right;
    r.bottom = r.top + height - margin;
  } else {
    const unsigned width = Layout::Scale(58);
    r.left = rc.left + i * width;
    r.top = rc.top;
    r.right = r.left + width - margin;
    r.bottom = rc.bottom;
  }

  return r;
}

void
ButtonPanel::MoveButtons()
{
  for (unsigned i = 0; i < buttons.size(); ++i) {
    const PixelRect r = GetButtonRect(i);
    buttons[i]->move(r.left, r.top,
                     r.right - r.left, r.bottom - r.top);
  }
}

void
ButtonPanel::Resized(const PixelRect &area)
{
  rc = remaining = area;

  const unsigned margin = Layout::Scale(2);
  const bool landscape = area.right - area.left > area.bottom - area.top;
  vertical = landscape;
  if (vertical) {
    rc.left += margin;
    rc.top += margin;
    rc.right = rc.left + Layout::Scale(80);
    remaining.left = rc.right;
  } else {
    rc.left += margin;
    rc.bottom -= margin;
    rc.top = rc.bottom - Layout::Scale(32);
    remaining.bottom = rc.top;
  }

  MoveButtons();
}

WndButton *
ButtonPanel::Add(const TCHAR *caption,
                 WndButton::ClickNotifyCallback_t callback)
{
  const PixelRect r = GetButtonRect(buttons.size());
  WndButton *button = new WndButton(parent, look, caption,
                                    r.left, r.top,
                                    r.right - r.left, r.bottom - r.top,
                                    style, callback);
  buttons.append(button);
  return button;
}
