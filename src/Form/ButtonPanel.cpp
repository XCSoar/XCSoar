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
  :parent(_parent), look(_look), row_count(0) {
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
  unsigned row = i / row_capacity;
  unsigned column = i % row_capacity;
  unsigned margin = Layout::Scale(1);
  PixelRect r;
  if (vertical) {
    const unsigned width = Layout::Scale(80);
    const unsigned height = Layout::Scale(30);

    r.left = rc.left + row * width;
    r.top = rc.top + column * height;
    r.right = r.left + width - margin;
    r.bottom = r.top + height - margin;
  } else {
    const unsigned width = Layout::Scale(75);
    const unsigned height = Layout::Scale(32);

    r.left = rc.left + column * width;
    r.top = rc.top + height * row;
    r.right = r.left + width - margin;
    r.bottom = r.top + height - margin;
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
ButtonPanel::Resized(const PixelRect &area, unsigned count)
{
  rc = remaining = area;

  const unsigned margin = Layout::Scale(2);
  const bool landscape = area.right - area.left > area.bottom - area.top;
  vertical = landscape;

  if (count == 0)
    count = buttons.size();

  child_size = vertical ? Layout::Scale(30) : Layout::Scale(75);
  unsigned row_height = vertical ? Layout::Scale(80) : Layout::Scale(32);

  unsigned pixel_size = vertical
    ? (area.bottom - area.top)
    : (area.right - area.left);
  row_capacity = pixel_size / child_size;
  if (row_capacity == 0)
    row_capacity = 1;

  row_count = (count + row_capacity - 1) / row_capacity;
  if (row_count * row_height > pixel_size / 2)
    row_count = pixel_size / row_height / 2;

  if (vertical) {
    rc.left += margin;
    rc.top += margin;
    rc.right = rc.left + row_height * row_count;
    remaining.left = rc.right;
  } else {
    rc.left += margin;
    rc.bottom -= margin;
    rc.top = rc.bottom - row_height * row_count;
    remaining.bottom = rc.top;
  }

  MoveButtons();
}

WndButton *
ButtonPanel::Add(const TCHAR *caption,
                 WndButton::ClickNotifyCallback_t callback)
{
  if (row_count == 0 || buttons.size() % row_capacity == 0)
    Resized(parent.get_client_rect(), buttons.size() + 1);

  const PixelRect r = GetButtonRect(buttons.size());
  WndButton *button = new WndButton(parent, look, caption,
                                    r.left, r.top,
                                    r.right - r.left, r.bottom - r.top,
                                    style, callback);
  buttons.append(button);

  return button;
}
