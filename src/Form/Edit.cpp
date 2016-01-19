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

#include "Form/Edit.hpp"
#include "Look/DialogLook.hpp"
#include "DataField/Base.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Event/KeyCode.hpp"
#include "Screen/Features.hpp"
#include "Dialogs/DataField.hpp"

#include <assert.h>

bool
WndProperty::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case KEY_RETURN:
    return true;

  case KEY_LEFT:
  case KEY_RIGHT:
    return !IsReadOnly();

  default:
    return WindowControl::OnKeyCheck(key_code);
  }
}

bool
WndProperty::OnKeyDown(unsigned key_code)
{
  // If return key pressed (Compaq uses VKF23)
  if (key_code == KEY_RETURN) {
    BeginEditing();
    return true;
  }

  switch (key_code) {
  case KEY_RIGHT:
    if (IsReadOnly())
      break;

    IncValue();
    return true;
  case KEY_LEFT:
    if (IsReadOnly())
      break;

    DecValue();
    return true;
  }

  return WindowControl::OnKeyDown(key_code);
}

void
WndProperty::OnSetFocus()
{
  WindowControl::OnSetFocus();

  Invalidate();
}

void
WndProperty::OnKillFocus()
{
  WindowControl::OnKillFocus();

  Invalidate();
}

WndProperty::WndProperty(ContainerWindow &parent, const DialogLook &_look,
                         const TCHAR *Caption,
                         const PixelRect &rc,
                         int CaptionWidth,
                         const WindowStyle style)
  :look(_look),
   data_field(nullptr),
   edit_callback(EditDataFieldDialog),
   read_only(false),
   dragging(false), pressed(false)
{
  Create(parent, rc, Caption, CaptionWidth, style);

#if defined(USE_WINUSER) && !defined(NDEBUG)
  ::SetWindowText(hWnd, Caption);
#endif
}

WndProperty::WndProperty(const DialogLook &_look)
  :look(_look),
   data_field(nullptr),
   edit_callback(EditDataFieldDialog),
   read_only(false),
   dragging(false), pressed(false)
{
}

void
WndProperty::Create(ContainerWindow &parent, const PixelRect &rc,
                    const TCHAR *_caption,
                    unsigned _caption_width,
                    const WindowStyle style=WindowStyle())
{
  caption = _caption;
  caption_width = _caption_width;

  WindowControl::Create(parent, rc, style);
}

WndProperty::~WndProperty()
{
  delete data_field;
}

unsigned
WndProperty::GetRecommendedCaptionWidth() const
{
  return look.text_font.TextSize(caption).cx + Layout::GetTextPadding();
}

void
WndProperty::SetCaptionWidth(int _caption_width)
{
  if (caption_width == _caption_width)
    return;

  caption_width = _caption_width;
  UpdateLayout();
}

bool
WndProperty::BeginEditing()
{
  if (IsReadOnly() || data_field == nullptr || edit_callback == nullptr) {
    OnHelp();
    return false;
  } else {
    if (!edit_callback(GetCaption(), *data_field, GetHelpText()))
      return false;

    RefreshDisplay();
    return true;
  }
}

void
WndProperty::UpdateLayout()
{
  edit_rc = GetClientRect();

  const unsigned margin = Layout::VptScale(1u);

  if (caption_width >= 0) {
    edit_rc.left += caption_width + margin;
    edit_rc.top += margin;
    edit_rc.right -= margin;
    edit_rc.bottom -= margin;
  } else {
    const unsigned caption_height = look.text_font.GetHeight();

    edit_rc.left += margin;
    edit_rc.top = margin + caption_height;
    edit_rc.right -= margin;
    edit_rc.bottom -= margin;
  }

  Invalidate();
}

void
WndProperty::OnResize(PixelSize new_size)
{
  WindowControl::OnResize(new_size);
  UpdateLayout();
}

bool
WndProperty::OnMouseDown(PixelPoint p)
{
  if (!IsReadOnly() || HasHelp()) {
    dragging = true;
    pressed = true;
    Invalidate();
    SetCapture();
    return true;
  }

  return false;
}

bool
WndProperty::OnMouseUp(PixelPoint p)
{
  if (dragging) {
    dragging = false;
    ReleaseCapture();

    if (pressed) {
      pressed = false;
      Invalidate();
      BeginEditing();
    }

    return true;
  }

  return false;
}

bool
WndProperty::OnMouseMove(PixelPoint p, unsigned keys)
{
  if (dragging) {
    const bool inside = IsInside(p);
    if (inside != pressed) {
      pressed = inside;
      Invalidate();
    }

    return true;
  }

  return false;
}

void
WndProperty::OnCancelMode()
{
  if (dragging) {
    dragging = false;
    pressed = false;
    Invalidate();
    ReleaseCapture();
  }

  WindowControl::OnCancelMode();
}

int
WndProperty::IncValue()
{
  if (data_field != nullptr) {
    data_field->Inc();
    RefreshDisplay();
  }
  return 0;
}

int
WndProperty::DecValue()
{
  if (data_field != nullptr) {
    data_field->Dec();
    RefreshDisplay();
  }
  return 0;
}

void
WndProperty::OnPaint(Canvas &canvas)
{
  const bool focused = HasCursorKeys() && HasFocus();

  /* background and selector */
  if (pressed) {
    canvas.Clear(look.list.pressed.background_color);
  } else if (focused) {
    canvas.Clear(look.focused.background_color);
  } else {
    /* don't need to erase the background when it has been done by the
       parent window already */
    if (HaveClipping())
      canvas.Clear(look.background_color);
  }

  if (!caption.empty()) {
    canvas.SetTextColor(focused
                          ? look.focused.text_color
                          : look.text_color);
    canvas.SetBackgroundTransparent();
    canvas.Select(look.text_font);

    PixelSize tsize = canvas.CalcTextSize(caption.c_str());

    PixelPoint org;
    if (caption_width < 0) {
      org.x = edit_rc.left;
      org.y = edit_rc.top - tsize.cy;
    } else {
      org.x = caption_width - tsize.cx;
      org.y = (GetHeight() - tsize.cy) / 2;
    }

    if (org.x < 1)
      org.x = 1;

    if (HaveClipping())
      canvas.DrawText(org.x, org.y, caption.c_str());
    else
      canvas.DrawClippedText(org.x, org.y, caption_width - org.x,
                             caption.c_str());
  }

  Color background_color, text_color;
  if (pressed) {
    background_color = COLOR_BLACK;
    text_color = COLOR_WHITE;
  } else if (IsEnabled()) {
    if (IsReadOnly())
      background_color = Color(0xf0, 0xf0, 0xf0);
    else
      background_color = COLOR_WHITE;
    text_color = COLOR_BLACK;
  } else {
    background_color = COLOR_LIGHT_GRAY;
    text_color = COLOR_DARK_GRAY;
  }

  canvas.DrawFilledRectangle(edit_rc, background_color);

  canvas.SelectHollowBrush();
  canvas.SelectBlackPen();
  canvas.Rectangle(edit_rc.left, edit_rc.top,
                   edit_rc.right, edit_rc.bottom);

  if (!value.empty()) {
    canvas.SetTextColor(text_color);
    canvas.SetBackgroundTransparent();
    canvas.Select(look.text_font);

    const int x = edit_rc.left + Layout::GetTextPadding();
    const int canvas_height = edit_rc.GetHeight();
    const int text_height = canvas.GetFontHeight();
    const int y = edit_rc.top + (canvas_height - text_height) / 2;

    canvas.TextAutoClipped(x, y, value.c_str());
  }
}

void
WndProperty::SetText(const TCHAR *_value)
{
  assert(_value != nullptr);

  if (value.compare(_value) == 0)
    return;

  value = _value;
  Invalidate();
}

void
WndProperty::RefreshDisplay()
{
  if (!data_field)
    return;

  SetText(data_field->GetAsDisplayString());
}

void
WndProperty::SetDataField(DataField *Value)
{
  assert(data_field == nullptr || data_field != Value);

  delete data_field;
  data_field = Value;

  UpdateLayout();

  RefreshDisplay();
}
