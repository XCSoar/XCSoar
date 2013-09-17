/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Form/Internal.hpp"
#include "Look/DialogLook.hpp"
#include "DataField/Base.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Features.hpp"
#include "Dialogs/DataField.hpp"

#include <assert.h>

static bool
CanEditInPlace()
{
  /* disabled for now, because we don't handle this yet properly:
     return HasKeyboard(); */
  return false;
}

bool
WndProperty::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case KEY_RETURN:
    return IsReadOnly() ||
      (mDataField != NULL && mDataField->supports_combolist) ||
      !CanEditInPlace() || HasHelp();

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
  if (key_code == KEY_RETURN && BeginEditing())
    return true;

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

  KeyTimer(true, key_code);
  return WindowControl::OnKeyDown(key_code);
}

bool
WndProperty::OnKeyUp(unsigned key_code)
{
  if (KeyTimer(false, key_code)) {
    // activate tool tips if hit return for long time
    if (key_code == KEY_RETURN) {
      if (OnHelp())
        return true;
    }
  } else if (key_code == KEY_RETURN) {
    if (CallSpecial())
      return true;
  }

  return WindowControl::OnKeyUp(key_code);
}

void
WndProperty::OnSetFocus()
{
  WindowControl::OnSetFocus();

  KeyTimer(true, 0);

  Invalidate();
}

void
WndProperty::OnKillFocus()
{
  WindowControl::OnKillFocus();

  KeyTimer(true, 0);

  Invalidate();
}

WndProperty::WndProperty(ContainerWindow &parent, const DialogLook &_look,
                         const TCHAR *Caption,
                         const PixelRect &rc,
                         int CaptionWidth,
                         const WindowStyle style)
  :look(_look),
   caption_width(CaptionWidth),
   mDataField(NULL),
   read_only(false),
   dragging(false), pressed(false)
{
  caption = Caption;

  Create(parent, rc, style);

#if defined(USE_GDI) && !defined(NDEBUG)
  ::SetWindowText(hWnd, Caption);
#endif
}

WndProperty::~WndProperty()
{
  delete mDataField;
}

UPixelScalar
WndProperty::GetRecommendedCaptionWidth() const
{
  return look.text_font->TextSize(caption).cx + Layout::GetTextPadding();
}

void
WndProperty::SetCaptionWidth(PixelScalar _caption_width)
{
  if (caption_width == _caption_width)
    return;

  caption_width = _caption_width;
  UpdateLayout();
}

bool
WndProperty::BeginEditing()
{
  if (IsReadOnly()) {
    /* this would display xml file help on a read-only wndproperty if
       it exists */
    return OnHelp();
  } else if (mDataField != NULL) {
    if (!EditDataFieldDialog(GetCaption(), *mDataField, GetHelpText()))
      return false;

    RefreshDisplay();
    return true;
  } else if (CanEditInPlace()) {
    // TODO: implement
    return true;
  } else
    return false;
}

void
WndProperty::UpdateLayout()
{
  edit_rc = GetClientRect();

  const UPixelScalar DEFAULTBORDERPENWIDTH = Layout::FastScale(1);

  if (caption_width >= 0) {
    edit_rc.left += caption_width + (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.top += (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.right -= (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.bottom -= (DEFAULTBORDERPENWIDTH + 1);
  } else {
    const unsigned caption_height = look.text_font->GetHeight();

    edit_rc.left += (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.top = DEFAULTBORDERPENWIDTH + caption_height;
    edit_rc.right -= (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.bottom -= (DEFAULTBORDERPENWIDTH + 1);
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
WndProperty::OnMouseDown(PixelScalar x, PixelScalar y)
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
WndProperty::OnMouseUp(PixelScalar x, PixelScalar y)
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
WndProperty::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (dragging) {
    const bool inside = IsInside(x, y);
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
WndProperty::CallSpecial()
{
  if (mDataField != NULL) {
    mDataField->Special();
    RefreshDisplay();
  }
  return 0;
}

int
WndProperty::IncValue()
{
  if (mDataField != NULL) {
    mDataField->Inc();
    RefreshDisplay();
  }
  return 0;
}

int
WndProperty::DecValue()
{
  if (mDataField != NULL) {
    mDataField->Dec();
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

  /* kludge: don't draw caption if width is too small (but not 0),
     used by the polar configuration panel.  This concept needs to be
     redesigned. */
  if (!caption.empty()) {
    canvas.SetTextColor(focused
                          ? look.focused.text_color
                          : look.text_color);
    canvas.SetBackgroundTransparent();
    canvas.Select(*look.text_font);

    PixelSize tsize = canvas.CalcTextSize(caption.c_str());

    RasterPoint org;
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
    canvas.Select(*look.text_font);

    const PixelScalar x = edit_rc.left + Layout::GetTextPadding();
    const PixelScalar canvas_height = edit_rc.bottom - edit_rc.top;
    const PixelScalar text_height = canvas.GetFontHeight();
    const PixelScalar y = edit_rc.top + (canvas_height - text_height) / 2;

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
  if (!mDataField)
    return;

  SetText(HasFocus() && CanEditInPlace()
          ? mDataField->GetAsString()
          : mDataField->GetAsDisplayString());
}

void
WndProperty::SetDataField(DataField *Value)
{
  assert(mDataField == NULL || mDataField != Value);

  delete mDataField;
  mDataField = Value;

  UpdateLayout();

  RefreshDisplay();
}
