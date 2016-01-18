/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Form/CheckBox.hpp"
#include "Form/ActionListener.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Canvas.hpp"
#include "Event/KeyCode.hpp"
#include "Asset.hpp"
#include "Util/Macros.hpp"

void
CheckBoxControl::Create(ContainerWindow &parent, const DialogLook &_look,
                        tstring::const_pointer _caption,
                        const PixelRect &rc,
                        const WindowStyle style,
                        ActionListener &_listener, int _id)
{
  checked = dragging = pressed = false;
  look = &_look;
  caption = _caption;

  listener = &_listener;
  id = _id;
  PaintWindow::Create(parent, rc, style);
}

void
CheckBoxControl::SetState(bool value)
{
  if (value == checked)
    return;

  checked = value;
  Invalidate();
}

void
CheckBoxControl::SetPressed(bool value)
{
  if (value == pressed)
    return;

  pressed = value;
  Invalidate();
}

bool
CheckBoxControl::OnClicked()
{
  if (listener != nullptr) {
    listener->OnAction(id);
    return true;
  }

  return false;
}

bool
CheckBoxControl::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case KEY_RETURN:
    return true;

  default:
    return false;
  }
}

bool
CheckBoxControl::OnKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_RETURN:
  case KEY_SPACE:
    SetState(!GetState());
    OnClicked();
    return true;
  }

  return PaintWindow::OnKeyDown(key_code);
}

bool
CheckBoxControl::OnMouseMove(PixelPoint p, unsigned keys)
{
  if (dragging) {
    SetPressed(IsInside(p));
    return true;
  } else
    return PaintWindow::OnMouseMove(p, keys);
}

bool
CheckBoxControl::OnMouseDown(PixelPoint p)
{
  if (IsTabStop())
    SetFocus();

  SetPressed(true);
  SetCapture();
  dragging = true;
  return true;
}

bool
CheckBoxControl::OnMouseUp(PixelPoint p)
{
  if (!dragging)
    return true;

  dragging = false;
  ReleaseCapture();

  if (!pressed)
    return true;

  SetPressed(false);
  SetState(!GetState());
  OnClicked();
  return true;
}

void
CheckBoxControl::OnSetFocus()
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
CheckBoxControl::OnKillFocus()
{
  PaintWindow::OnKillFocus();
  Invalidate();
}

void
CheckBoxControl::OnCancelMode()
{
  dragging = false;
  SetPressed(false);

  PaintWindow::OnCancelMode();
}

void
CheckBoxControl::OnPaint(Canvas &canvas)
{
  const auto &cb_look = look->check_box;

  const bool focused = HasCursorKeys() && HasFocus();

  if (focused)
    canvas.Clear(cb_look.focus_background_brush);
  else if (HaveClipping())
    canvas.Clear(look->background_brush);

  const auto &state_look = IsEnabled()
    ? (pressed
       ? cb_look.pressed
       : (focused
          ? cb_look.focused
          : cb_look.standard))
    : cb_look.disabled;

  unsigned size = canvas.GetHeight() - 4;

  canvas.Select(state_look.box_brush);
  canvas.Select(state_look.box_pen);
  canvas.Rectangle(2, 2, size, size);

  if (checked) {
    canvas.Select(state_look.check_brush);
    canvas.SelectNullPen();

    BulkPixelPoint check_mark[] = {
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

  canvas.Select(*cb_look.font);
  canvas.SetTextColor(state_look.text_color);
  canvas.SetBackgroundTransparent();
  canvas.DrawText(canvas.GetHeight() + 2, 2, caption.c_str());
}
