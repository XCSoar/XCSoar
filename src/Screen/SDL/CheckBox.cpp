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

#include "Screen/CheckBox.hpp"
#include "Screen/ContainerWindow.hpp"

#include <algorithm>

void
CheckBox::set(ContainerWindow &parent, const TCHAR *text, unsigned id,
              int left, int top, unsigned width, unsigned height,
              const CheckBoxStyle style)
{
  reset();

  PaintWindow::set(parent, left, top, width, height, style);

  this->text = text;
  this->id = id;
}

void
CheckBox::set_checked(bool value)
{
  if (value == checked)
    return;

  checked = value;
  invalidate();
}

void
CheckBox::set_pressed(bool value)
{
  if (value == pressed)
    return;

  pressed = value;
  invalidate();
}

bool
CheckBox::on_mouse_move(int x, int y, unsigned keys)
{
  if (dragging) {
    set_pressed(x >= 0 && y >= 0 &&
                (unsigned)x < get_width() && (unsigned)y < get_height());
    return true;
  } else
    return PaintWindow::on_mouse_move(x, y, keys);
}

bool
CheckBox::on_mouse_down(int x, int y)
{
  set_pressed(true);
  set_capture();
  dragging = true;
  return true;
}

bool
CheckBox::on_mouse_up(int x, int y)
{
  if (!dragging)
    return true;

  dragging = false;
  release_capture();

  if (!pressed)
    return true;

  set_pressed(false);
  checked = !checked;
  invalidate();

  if (!on_clicked() && id != 0 && parent != NULL)
    parent->on_command(id, 0);

  return true;
}

bool
CheckBox::on_cancel_mode()
{
  release_capture();
  dragging = false;
  set_pressed(false);

  PaintWindow::on_cancel_mode();

  return true;
}

void
CheckBox::on_paint(Canvas &canvas)
{
  Brush brush(pressed ? Color::GRAY : Color::WHITE);
  canvas.select(brush);
  canvas.black_pen();
  canvas.rectangle(2, 2, canvas.get_height() - 4, canvas.get_height() - 4);

  if (checked) {
    canvas.line(4, 4, canvas.get_height() - 8, canvas.get_height() - 8);
    canvas.line(canvas.get_height() - 8, 4, 4, canvas.get_height() - 8);
  }

  canvas.set_text_color(Color::BLACK);
  canvas.background_transparent();
  canvas.text(canvas.get_height() + 2, 2, text.c_str());

  if (has_focus()) {
    RECT rc = { 0, 0, canvas.get_width()-1, canvas.get_height()-1 };
    canvas.draw_focus(rc);
  }
}

bool
CheckBox::on_clicked()
{
  return false;
}
