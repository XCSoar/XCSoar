/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Screen/Layout.hpp"
#include "Screen/Key.h"

#include <algorithm>

void
CheckBox::set(ContainerWindow &parent, const TCHAR *text, unsigned id,
              const PixelRect &rc,
              const CheckBoxStyle style)
{
  reset();

  PaintWindow::set(parent, rc, style);

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
CheckBox::OnKeyDown(unsigned key_code)
{
  switch (key_code) {
  case VK_RETURN:
  case VK_SPACE:
    checked = !checked;
    invalidate();

    if (!on_clicked() && id != 0 && parent != NULL)
      parent->OnCommand(id, 0);
    return true;

  default:
    return PaintWindow::OnKeyDown(key_code);
  }
}

bool
CheckBox::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (dragging) {
    set_pressed(x >= 0 && y >= 0 &&
                (unsigned)x < get_width() && (unsigned)y < get_height());
    return true;
  } else
    return PaintWindow::OnMouseMove(x, y, keys);
}

bool
CheckBox::OnMouseDown(PixelScalar x, PixelScalar y)
{
  if (is_tab_stop())
    set_focus();

  set_pressed(true);
  set_capture();
  dragging = true;
  return true;
}

bool
CheckBox::OnMouseUp(PixelScalar x, PixelScalar y)
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
    parent->OnCommand(id, 0);

  return true;
}

void
CheckBox::OnSetFocus()
{
  PaintWindow::OnSetFocus();
  invalidate();
}

void
CheckBox::OnKillFocus()
{
  PaintWindow::OnKillFocus();
  invalidate();
}

bool
CheckBox::OnCancelMode()
{
  dragging = false;
  set_pressed(false);

  return PaintWindow::OnCancelMode();
}

void
CheckBox::OnPaint(Canvas &canvas)
{
  if (has_focus())
    canvas.clear(COLOR_XCSOAR_DARK);

  Brush brush(pressed ? COLOR_XCSOAR_LIGHT : COLOR_WHITE);
  canvas.Select(brush);

  if (is_enabled())
    canvas.SelectBlackPen();
  else
    canvas.Select(Pen(1, COLOR_GRAY));

  canvas.Rectangle(2, 2, canvas.get_height() - 4, canvas.get_height() - 4);

  if (checked) {
    canvas.line(4, 4, canvas.get_height() - 8, canvas.get_height() - 8);
    canvas.line(canvas.get_height() - 8, 4, 4, canvas.get_height() - 8);
  }

  canvas.SetTextColor(
      is_enabled() ? (has_focus() ? COLOR_WHITE : COLOR_BLACK) : COLOR_GRAY);
  canvas.SetBackgroundTransparent();
  canvas.text(canvas.get_height() + 2, 2, text.c_str());
}

bool
CheckBox::on_clicked()
{
  return false;
}
