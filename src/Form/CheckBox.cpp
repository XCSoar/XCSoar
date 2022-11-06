/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"
#include "util/Macros.hpp"

void
CheckBoxControl::Create(ContainerWindow &parent, const DialogLook &_look,
                        tstring::const_pointer _caption,
                        const PixelRect &rc,
                        const WindowStyle style,
                        Callback _callback) noexcept
{
  checked = dragging = pressed = false;
  look = &_look;
  caption = _caption;

  callback = std::move(_callback);

  PaintWindow::Create(parent, rc, style);
}

unsigned
CheckBoxControl::GetMinimumWidth(const DialogLook &look, unsigned height,
                                 tstring::const_pointer caption) noexcept
{
  const unsigned padding = Layout::GetTextPadding();
  return 3 * padding + height + look.check_box.font->TextSize(caption).width;
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
CheckBoxControl::OnClicked() noexcept
{
  if (callback) {
    callback(GetState());
    return true;
  }

  return false;
}

bool
CheckBoxControl::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_RETURN:
    return true;

  default:
    return false;
  }
}

bool
CheckBoxControl::OnKeyDown(unsigned key_code) noexcept
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
CheckBoxControl::OnMouseMove(PixelPoint p, unsigned keys) noexcept
{
  if (dragging) {
    SetPressed(IsInside(p));
    return true;
  } else
    return PaintWindow::OnMouseMove(p, keys);
}

bool
CheckBoxControl::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  if (IsTabStop())
    SetFocus();

  SetPressed(true);
  SetCapture();
  dragging = true;
  return true;
}

bool
CheckBoxControl::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
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
CheckBoxControl::OnSetFocus() noexcept
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
CheckBoxControl::OnKillFocus() noexcept
{
  PaintWindow::OnKillFocus();
  Invalidate();
}

void
CheckBoxControl::OnCancelMode() noexcept
{
  dragging = false;
  SetPressed(false);

  PaintWindow::OnCancelMode();
}

void
CheckBoxControl::OnPaint(Canvas &canvas) noexcept
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

  const unsigned padding = Layout::GetTextPadding();
  unsigned size = canvas.GetHeight() - 2 * padding;

  canvas.Select(state_look.box_brush);
  canvas.Select(state_look.box_pen);
  canvas.DrawRectangle(PixelRect{PixelSize{canvas.GetHeight()}}.WithPadding(padding));

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
    for (auto &i : check_mark) {
      i.x = (i.x * (int)size) / 24 + top;
      i.y = (i.y * (int)size) / 24 + top;
    }

    canvas.DrawPolygon(check_mark, ARRAY_SIZE(check_mark));
  }

  canvas.Select(*cb_look.font);
  canvas.SetTextColor(state_look.text_color);
  canvas.SetBackgroundTransparent();

  const PixelPoint caption_position(canvas.GetHeight() + 2 * padding,
                                    ((int)canvas.GetHeight() - (int)cb_look.font->GetHeight()) / 2);
  canvas.DrawText(caption_position, caption.c_str());
}
