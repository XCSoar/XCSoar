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
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Util/Macros.hpp"

#include <algorithm>

void
CheckBox::Create(ContainerWindow &parent, const TCHAR *text, unsigned id,
                 const PixelRect &rc,
                 const CheckBoxStyle style)
{
  Destroy();

  PaintWindow::Create(parent, rc, style);

  this->text = text;
  this->id = id;
}

void
CheckBox::SetState(bool value)
{
  if (value == checked)
    return;

  checked = value;
  Invalidate();
}

void
CheckBox::SetPressed(bool value)
{
  if (value == pressed)
    return;

  pressed = value;
  Invalidate();
}

bool
CheckBox::OnKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_RETURN:
  case KEY_SPACE:
    checked = !checked;
    Invalidate();

    if (!OnClicked() && id != 0 && parent != NULL)
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
    SetPressed(IsInside(x, y));
    return true;
  } else
    return PaintWindow::OnMouseMove(x, y, keys);
}

bool
CheckBox::OnMouseDown(PixelScalar x, PixelScalar y)
{
  if (IsTabStop())
    SetFocus();

  SetPressed(true);
  SetCapture();
  dragging = true;
  return true;
}

bool
CheckBox::OnMouseUp(PixelScalar x, PixelScalar y)
{
  if (!dragging)
    return true;

  dragging = false;
  ReleaseCapture();

  if (!pressed)
    return true;

  SetPressed(false);
  checked = !checked;
  Invalidate();

  if (!OnClicked() && id != 0 && parent != NULL)
    parent->OnCommand(id, 0);

  return true;
}

void
CheckBox::OnSetFocus()
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
CheckBox::OnKillFocus()
{
  PaintWindow::OnKillFocus();
  Invalidate();
}

bool
CheckBox::OnCancelMode()
{
  dragging = false;
  SetPressed(false);

  return PaintWindow::OnCancelMode();
}

void
CheckBox::OnPaint(Canvas &canvas)
{
  if (HasFocus())
    canvas.Clear(COLOR_XCSOAR_DARK);

  Brush brush(pressed ? COLOR_XCSOAR_LIGHT : COLOR_WHITE);
  canvas.Select(brush);

  if (IsEnabled())
    canvas.SelectBlackPen();
  else
    canvas.Select(Pen(1, COLOR_GRAY));

  unsigned size = canvas.GetHeight() - 4;
  canvas.Rectangle(2, 2, size, size);

  if (checked) {
    canvas.SelectNullPen();

    if (IsEnabled())
      canvas.SelectBlackBrush();
    else
      canvas.Select(Brush(COLOR_GRAY));

    RasterPoint check_mark[] = {
      {-8, -2},
      {-3, 6},
      {7, -9},
      {8, -5},
      {-3, 9},
      {-9, 2},
    };

    unsigned top = canvas.GetHeight() / 2;
    for (unsigned i = 0; i < ARRAY_SIZE(check_mark); ++i) {
      check_mark[i].x = (check_mark[i].x * (int)size) / 24 + top;
      check_mark[i].y = (check_mark[i].y * (int)size) / 24 + top;
    }

    canvas.DrawPolygon(check_mark, ARRAY_SIZE(check_mark));
  }

  canvas.SetTextColor(IsEnabled()
                      ? (HasFocus() ? COLOR_WHITE : COLOR_BLACK)
                      : COLOR_GRAY);
  canvas.SetBackgroundTransparent();
  canvas.DrawText(canvas.GetHeight() + 2, 2, text.c_str());
}

bool
CheckBox::OnClicked()
{
  return false;
}
