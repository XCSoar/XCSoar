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

#include "DigitEntry.hpp"
#include "ActionListener.hpp"
#include "Screen/Font.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Point.hpp"
#include "Screen/Key.h"
#include "Screen/Canvas.hpp"
#include "Look/DialogLook.hpp"
#include "Units/Descriptor.hpp"
#include "Time/RoughTime.hpp"
#include "Math/Angle.hpp"
#include "Renderer/SymbolRenderer.hpp"

#include <algorithm>

#include <stdio.h>

DigitEntry::DigitEntry(const DialogLook &_look)
  :look(_look),
   button_renderer(look.button),
   action_listener(nullptr)
{
}

DigitEntry::~DigitEntry()
{
}

void
DigitEntry::Create(ContainerWindow &parent, const PixelRect &rc,
                   const WindowStyle style,
                   unsigned _length)
{
  assert(_length > 0);
  assert(_length <= MAX_LENGTH);

  length = _length;
  cursor = length - 1;
  valid = true;

  for (unsigned i = 0; i < length; ++i) {
    Column &digit = columns[i];
    digit.type = Column::Type::DIGIT;
    digit.value = 0;
  }

  max_width = rc.right - rc.left;

  CalculateLayout();

  PaintWindow::Create(parent, rc, style);
}

void
DigitEntry::CreateSigned(ContainerWindow &parent, const PixelRect &rc,
                         const WindowStyle style,
                         unsigned ndigits, unsigned precision)
{
  Create(parent, rc, style, 1 + ndigits + (precision > 0));

  columns[0].type = Column::Type::SIGN;

  if (precision > 0) {
    columns[1 + ndigits - precision].type = Column::Type::DECIMAL_POINT;

    if (ndigits > precision)
      cursor -= precision + 1;
  }
}

void
DigitEntry::CreateUnsigned(ContainerWindow &parent, const PixelRect &rc,
                           const WindowStyle style,
                           unsigned ndigits, unsigned precision)
{
  Create(parent, rc, style, ndigits + (precision > 0));

  if (precision > 0) {
    columns[ndigits - precision].type = Column::Type::DECIMAL_POINT;

    if (ndigits > precision)
      cursor -= precision + 1;
  }
}

void
DigitEntry::CreateAngle(ContainerWindow &parent, const PixelRect &rc,
                        const WindowStyle style)
{
  Create(parent, rc, style, 3);

  columns[0].type = Column::Type::DIGIT36;
  columns[2].type = Column::Type::DEGREES;
  cursor = 0;

  CalculateLayout();
}

void
DigitEntry::CreateLatitude(ContainerWindow &parent, const PixelRect &rc,
                           const WindowStyle style)
{
  // TODO: support all CoordinateFormats

  Create(parent, rc, style, 9);

  columns[0].type = Column::Type::NORTH_SOUTH;
  columns[3].type = Column::Type::DEGREES;
  columns[4].type = Column::Type::DIGIT6;
  columns[6].type = Column::Type::APOSTROPHE;
  columns[7].type = Column::Type::DIGIT6;

  cursor = 0;

  CalculateLayout();
}

void
DigitEntry::CreateLongitude(ContainerWindow &parent, const PixelRect &rc,
                            const WindowStyle style)
{
  // TODO: support all CoordinateFormats

  Create(parent, rc, style, 9);

  columns[0].type = Column::Type::EAST_WEST;
  columns[1].type = Column::Type::DIGIT19;
  columns[3].type = Column::Type::DEGREES;
  columns[4].type = Column::Type::DIGIT6;
  columns[6].type = Column::Type::APOSTROPHE;
  columns[7].type = Column::Type::DIGIT6;

  cursor = 0;

  CalculateLayout();
}

void
DigitEntry::CreateTime(ContainerWindow &parent, const PixelRect &rc,
                       const WindowStyle style)
{
  Create(parent, rc, style, 4);

  columns[0].type = Column::Type::HOUR;
  columns[1].type = Column::Type::COLON;
  columns[2].type = Column::Type::DIGIT6;
  cursor = 0;

  CalculateLayout();
}

void
DigitEntry::CalculateLayout()
{
  const UPixelScalar control_height = Layout::GetMaximumControlHeight();
  const UPixelScalar padding = Layout::GetTextPadding();

  const UPixelScalar min_value_height = control_height * 3 / 2;

  PixelSize digit_size = look.text_font->TextSize(_T("8"));
  digit_size.cy += 2 * padding;
  if (digit_size.cy < (PixelScalar)min_value_height)
    digit_size.cy = min_value_height;

  top = control_height;
  bottom = top + digit_size.cy;

  unsigned last_right = 0;
  for (unsigned i = 0; i < length; ++i) {
    Column &digit = columns[i];

    PixelScalar value_width = digit.GetWidth() * digit_size.cx;
    value_width += 2 * padding;
    if (value_width < (PixelScalar)control_height)
      value_width = control_height;

    digit.left = last_right;
    last_right = digit.right = digit.left + value_width;
  }

  /* check if width of entry form exceeds available width and adjust
     column widths appropiately */
  if (last_right > max_width) {
    unsigned width_adjust = (last_right - max_width) / length + 1;
    last_right = 0;
    for (unsigned i = 0; i < length; ++i) {
      Column &digit = columns[i];
      PixelScalar value_width = digit.right - digit.left;
      digit.left = last_right;
      last_right = digit.right = digit.left + value_width - width_adjust;
    }
  }
}

int
DigitEntry::FindEditableLeft(int i) const
{
  for (; i >= 0; --i)
    if (columns[i].IsEditable())
      break;

  return i;
}

int
DigitEntry::FindEditableRight(unsigned i) const
{
  for (; i < length; ++i)
    if (columns[i].IsEditable())
      return i;

  return -1;
}

void
DigitEntry::SetCursor(unsigned _cursor)
{
  assert(_cursor < length);

  if (_cursor == cursor)
    return;

  cursor = _cursor;
  Invalidate();
}

int
DigitEntry::FindDecimalPoint() const
{
  for (unsigned i = 0; i < length; ++i)
    if (columns[i].type == Column::Type::DECIMAL_POINT)
      return i;

  return -1;
}

int
DigitEntry::FindNumberLeft(int i) const
{
  for (; i >= 0; --i)
    if (columns[i].IsNumber())
      break;

  return i;
}


void
DigitEntry::SetInvalid()
{
  if (!valid)
    return;

  valid = false;
  Invalidate();
}

void
DigitEntry::SetValue(int value)
{
  assert(length > 0);

  unsigned uvalue = abs(value);
  if (columns[0].IsSign()) {
    columns[0].SetNegative(value < 0);
  } else if (value < 0)
    uvalue = 0u;

  const int dp = FindDecimalPoint();

  int i = (dp >= 0) ? dp : length;
  while (true) {
    i = FindNumberLeft(i - 1);
    if (i < 0)
      break;

    columns[i].value = uvalue % 10;
    uvalue /= 10;
  }

  if (dp >= 0)
    for (unsigned i = dp + 1; i < length; ++i)
      if (columns[i].type == Column::Type::DIGIT)
        columns[i].value = 0;

  Invalidate();
}

void
DigitEntry::SetValue(unsigned value)
{
  SetValue((int)value);
}

void
DigitEntry::SetValue(fixed value)
{
  // XXX implement
  SetValue((int)value);
}

void
DigitEntry::SetValue(RoughTime value)
{
  assert(length == 4);
  assert(columns[0].type == Column::Type::HOUR);
  assert(columns[1].type == Column::Type::COLON);
  assert(columns[2].type == Column::Type::DIGIT6);
  assert(columns[3].type == Column::Type::DIGIT);

  if (value.IsValid()) {
    columns[0].value = value.GetHour();
    columns[2].value = value.GetMinute() / 10;
    columns[3].value = value.GetMinute() % 10;
    valid = true;
  } else {
    columns[0].value = 0;
    columns[2].value = 0;
    columns[3].value = 0;
    valid = false;
  }

  Invalidate();
}

void
DigitEntry::SetValue(Angle value)
{
  assert(length == 3);
  assert(columns[0].type == Column::Type::DIGIT36);
  assert(columns[1].type == Column::Type::DIGIT);
  assert(columns[2].type == Column::Type::DEGREES);

  int degrees = iround(value.Degrees());

  columns[0].value = degrees / 10;
  columns[1].value = degrees % 10;

  Invalidate();
}

unsigned
DigitEntry::GetPositiveInteger() const
{
  unsigned value = 0;

  for (unsigned i = 0; i < length; ++i) {
    const Column &c = columns[i];

    if (c.type == Column::Type::DIGIT) {
      assert(c.value < 10);
      value = (value * 10) + c.value;
    } else if (c.type == Column::Type::DIGIT36) {
      assert(c.value < 36);
      value = (value * 100) + c.value;
    } else if (c.type == Column::Type::DECIMAL_POINT)
      break;
  }

  return value;
}

fixed
DigitEntry::GetPositiveFractional() const
{
  const int dp = FindDecimalPoint();
  if (dp < 0)
    return fixed(0);

  fixed value = fixed(0);
  unsigned factor = 10;

  for (unsigned i = dp + 1; i < length; ++i) {
    const Column &c = columns[i];
    if (c.type != Column::Type::DIGIT)
      continue;

    assert(c.value < 10);
    value += fixed(c.value) / factor;
    factor *= 10;
  }

  return value;
}

RoughTime
DigitEntry::GetTimeValue() const
{
  assert(length == 4);
  assert(columns[0].type == Column::Type::HOUR);
  assert(columns[1].type == Column::Type::COLON);
  assert(columns[2].type == Column::Type::DIGIT6);
  assert(columns[3].type == Column::Type::DIGIT);

  if (!valid)
    return RoughTime::Invalid();

  return RoughTime(columns[0].value,
                   columns[2].value * 10 + columns[3].value);
}

void
DigitEntry::SetLatitude(Angle value)
{
  // TODO: support all CoordinateFormats

  value = value.AsBearing();

  assert(length == 9);
  assert(columns[0].type == Column::Type::NORTH_SOUTH);
  assert(columns[1].type == Column::Type::DIGIT);
  assert(columns[2].type == Column::Type::DIGIT);
  assert(columns[4].type == Column::Type::DIGIT6);
  assert(columns[5].type == Column::Type::DIGIT);
  assert(columns[7].type == Column::Type::DIGIT6);
  assert(columns[8].type == Column::Type::DIGIT);

  columns[0].value = negative(value.Native());

  const fixed degrees = fabs(value.Degrees());
  const unsigned i_degrees = std::min(unsigned(degrees), 90u);
  const unsigned full_seconds = unsigned(degrees * 3600u) % 3600u;
  const unsigned minutes = std::min(full_seconds / 60u, 59u);
  const unsigned seconds = full_seconds % 60u;

  columns[1].value = i_degrees / 10;
  columns[2].value = i_degrees % 10;

  columns[4].value = minutes / 10;
  columns[5].value = minutes % 10;

  columns[7].value = seconds / 10;
  columns[8].value = seconds % 10;

  Invalidate();
}


void
DigitEntry::SetLongitude(Angle value)
{
  // TODO: support all CoordinateFormats

  value = value.AsBearing();

  assert(length == 9);
  assert(columns[0].type == Column::Type::EAST_WEST);
  assert(columns[1].type == Column::Type::DIGIT19);
  assert(columns[2].type == Column::Type::DIGIT);
  assert(columns[4].type == Column::Type::DIGIT6);
  assert(columns[5].type == Column::Type::DIGIT);
  assert(columns[7].type == Column::Type::DIGIT6);
  assert(columns[8].type == Column::Type::DIGIT);

  columns[0].value = negative(value.Native());

  const fixed degrees = fabs(value.Degrees());
  const unsigned i_degrees = std::min(unsigned(degrees), 180u);
  const unsigned full_seconds = unsigned(degrees * 3600u) % 3600u;
  const unsigned minutes = std::min(full_seconds / 60u, 59u);
  const unsigned seconds = full_seconds % 60u;

  columns[1].value = i_degrees / 10;
  columns[2].value = i_degrees % 10;

  columns[4].value = minutes / 10;
  columns[5].value = minutes % 10;

  columns[7].value = seconds / 10;
  columns[8].value = seconds % 10;

  Invalidate();
}

Angle
DigitEntry::GetGeoAngle() const
{
  // TODO: support all CoordinateFormats

  if (!valid)
    return Angle::FullCircle();

  assert(columns[0].type == Column::Type::NORTH_SOUTH ||
         columns[0].type == Column::Type::EAST_WEST);
  assert(columns[1].type == Column::Type::DIGIT ||
         columns[1].type == Column::Type::DIGIT19);
  assert(columns[2].type == Column::Type::DIGIT);
  assert(columns[4].type == Column::Type::DIGIT6);
  assert(columns[5].type == Column::Type::DIGIT);
  assert(columns[7].type == Column::Type::DIGIT6);
  assert(columns[8].type == Column::Type::DIGIT);

  fixed degrees = fixed(columns[1].value * 10 + columns[2].value)
    + (columns[4].value * 10 + columns[5].value) * fixed(1 / 60.)
    + (columns[7].value * 10 + columns[8].value) * fixed(1 / 3600.);
  if (columns[0].IsNegative())
    degrees = -degrees;

  return Angle::Degrees(degrees);
}

Angle
DigitEntry::GetLatitude() const
{
  // TODO: support all CoordinateFormats

  assert(columns[0].type == Column::Type::NORTH_SOUTH);
  assert(columns[1].type == Column::Type::DIGIT);

  return GetGeoAngle();
}

Angle
DigitEntry::GetLongitude() const
{
  assert(columns[0].type == Column::Type::EAST_WEST);
  assert(columns[1].type == Column::Type::DIGIT19);

  return GetGeoAngle();
}

void
DigitEntry::IncrementColumn(unsigned i)
{
  assert(i < length);

  valid = true;

  Column &c = columns[i];
  if (c.IsNumber()) {
    if (c.value < c.GetMaxNumber())
      ++c.value;
    else {
      c.value = 0;

      int previous = FindNumberLeft(i - 1);
      if (previous >= 0)
        IncrementColumn(previous);
    }
  } else if (c.IsSign()) {
    c.value = !c.value;
  }

  Invalidate();
}

void
DigitEntry::DecrementColumn(unsigned i)
{
  assert(i < length);

  valid = true;

  Column &c = columns[i];
  if (c.IsNumber()) {
    if (c.value > 0)
      --c.value;
    else {
      c.value = c.GetMaxNumber();

      int previous = FindNumberLeft(i - 1);
      if (previous >= 0)
        DecrementColumn(previous);
    }
  } else if (c.IsSign()) {
    c.value = !c.value;
  }

  Invalidate();
}

int
DigitEntry::FindColumnAt(unsigned x) const
{
  for (unsigned i = 0; i < length; ++i)
    if (x >= columns[i].left && x < columns[i].right)
      return i;

  /* not found */
  return -1;
}

int
DigitEntry::GetIntegerValue() const
{
  int value = GetPositiveInteger();
  return IsNegative() ? -value : value;
}

unsigned
DigitEntry::GetUnsignedValue() const
{
  assert(!IsSigned());

  return GetPositiveInteger();
}

fixed
DigitEntry::GetFixedValue() const
{
  fixed value = fixed(GetPositiveInteger()) + GetPositiveFractional();
  return IsNegative() ? -value : value;
}

Angle
DigitEntry::GetAngleValue() const
{
  return Angle::Degrees(GetFixedValue());
}

bool
DigitEntry::OnMouseDown(PixelScalar x, PixelScalar y)
{
  int i = FindColumnAt(x);
  if (i >= 0 && columns[i].IsEditable()) {
    SetCursor(i);
    SetFocus();

    if (y < int(top))
      IncrementColumn(i);
    else if (unsigned(y) > bottom)
      DecrementColumn(i);

    return true;
  }

  return PaintWindow::OnMouseDown(x, y);
}

bool
DigitEntry::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case KEY_UP:
  case KEY_DOWN:
    return true;

  case KEY_LEFT:
    return cursor > 0;

  case KEY_RIGHT:
    return cursor < length - 1;

  default:
    return PaintWindow::OnKeyCheck(key_code);
  }
}

bool
DigitEntry::OnKeyDown(unsigned key_code)
{
  assert(cursor < length);

  switch (key_code) {
    int i;

  case KEY_UP:
    IncrementColumn(cursor);
    return true;

  case KEY_DOWN:
    DecrementColumn(cursor);
    return true;

  case KEY_LEFT:
    i = FindEditableLeft(cursor - 1);
    if (i >= 0)
      SetCursor(i);
    return true;

  case KEY_RIGHT:
    i = FindEditableRight(cursor + 1);
    if (i >= 0)
      SetCursor(i);
    return true;

  case KEY_RETURN:
    if (action_listener != nullptr) {
      action_listener->OnAction(action_id);
      return true;
    }

    break;
  }

  return PaintWindow::OnKeyDown(key_code);
}

void
DigitEntry::OnSetFocus()
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
DigitEntry::OnKillFocus()
{
  PaintWindow::OnKillFocus();
  Invalidate();
}

void
DigitEntry::OnPaint(Canvas &canvas)
{
  assert(cursor < length);

  const bool focused = HasCursorKeys() && HasFocus();

  if (HaveClipping())
    canvas.Clear(look.background_color);

  canvas.Select(*look.text_font);
  canvas.SetBackgroundOpaque();

  const unsigned text_height = look.text_font->GetHeight();
  const int y = (top + bottom - text_height) / 2;

  PixelRect rc;
  rc.top = top;
  rc.bottom = bottom;

  TCHAR buffer[4];

  for (unsigned i = 0; i < length; ++i) {
    const Column &c = columns[i];

    rc.left = c.left;
    rc.right = c.right;

    if (focused && i == cursor) {
      canvas.SetTextColor(look.list.focused.text_color);
      canvas.SetBackgroundColor(look.list.focused.background_color);
    } else if (c.IsEditable()) {
      canvas.SetTextColor(look.list.text_color);
      canvas.SetBackgroundColor(look.list.background_color);
    } else {
      canvas.SetTextColor(look.list.text_color);
      canvas.SetBackgroundColor(look.background_color);
    }

    const TCHAR *text = buffer;
    buffer[1] = _T('\0');

    switch (c.type) {
    case Column::Type::DIGIT:
    case Column::Type::DIGIT6:
      assert(c.value < 10);
      buffer[0] = _T('0') + c.value;
      break;

    case Column::Type::HOUR:
      assert(c.value < 24);
      _stprintf(buffer, _T("%02u"), c.value);
      break;

    case Column::Type::DIGIT36:
      assert(c.value < 36);
      _stprintf(buffer, _T("%02u"), c.value);
      break;

    case Column::Type::DIGIT19:
      assert(c.value < 19);
      _stprintf(buffer, _T("%02u"), c.value);
      break;

    case Column::Type::SIGN:
      buffer[0] = c.IsNegative() ? _T('-') : _T('+');
      break;

    case Column::Type::DECIMAL_POINT:
      buffer[0] = _T('.');
      break;

    case Column::Type::COLON:
      buffer[0] = _T(':');
      break;

    case Column::Type::NORTH_SOUTH:
      buffer[0] = c.IsNegative() ? _T('S') : _T('N');
      break;

    case Column::Type::EAST_WEST:
      buffer[0] = c.IsNegative() ? _T('W') : _T('E');
      break;

    case Column::Type::DEGREES:
      text = _T("°");
      break;

    case Column::Type::APOSTROPHE:
      text = _T("'");
      break;

    case Column::Type::UNIT:
      // TODO: render unit symbol?
      text = Units::unit_descriptors[c.value].name;
      break;
    }

    if (c.IsEditable() && !valid)
      buffer[0] = _T('\0');

    const int x = (c.left + c.right - canvas.CalcTextWidth(text)) / 2;

    canvas.DrawOpaqueText(x, y, rc, text);
  }

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(look.text_color);

  unsigned control_height = Layout::GetMaximumControlHeight();

  PixelRect plus_rc(0, top - control_height, 0, top);
  PixelRect minus_rc(0, bottom, 0, bottom + control_height);

  for (unsigned i = 0; i < length; ++i) {
    const Column &c = columns[i];
    if (!c.IsEditable())
      continue;

    plus_rc.left = minus_rc.left = c.left;
    plus_rc.right = minus_rc.right = c.right;

    button_renderer.DrawButton(canvas, plus_rc, false, false);
    button_renderer.DrawButton(canvas, minus_rc, false, false);

    canvas.SelectNullPen();
    canvas.Select(look.button.standard.foreground_brush);

    SymbolRenderer::DrawArrow(canvas, plus_rc, SymbolRenderer::UP);
    SymbolRenderer::DrawArrow(canvas, minus_rc, SymbolRenderer::DOWN);
  }
}
