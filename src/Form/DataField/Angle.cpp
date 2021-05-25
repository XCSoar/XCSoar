/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Angle.hpp"
#include "ComboList.hpp"
#include "util/NumberParser.hpp"
#include "util/Macros.hpp"

#include <stdio.h>

static TCHAR buffer[16];

unsigned
AngleDataField::Import(int value) noexcept
{
  assert(value >= -int(MAX));
  if (value < 0)
    return value + MAX;

  return Import(unsigned(value));
}

void
AngleDataField::ModifyValue(unsigned _value) noexcept
{
  unsigned value2 = Import(_value);
  if (value2 == value)
    return;

  value = value2;
  Modified();
}

void
AngleDataField::ModifyValue(int _value) noexcept
{
  unsigned value2 = Import(_value);
  if (value2 == value)
    return;

  value = value2;
  Modified();
}

void
AngleDataField::ModifyValue(Angle _value) noexcept
{
  unsigned value2 = Import(_value);
  if (value2 == value)
    return;

  value = value2;
  Modified();
}

int
AngleDataField::GetAsInteger() const noexcept
{
  return GetIntegerValue();
}

const TCHAR *
AngleDataField::GetAsString() const noexcept
{
  _stprintf(buffer, _T("%u"), GetIntegerValue());
  return buffer;
}

const TCHAR *
AngleDataField::GetAsDisplayString() const noexcept
{
  _stprintf(buffer, _T("%u°"), GetIntegerValue());
  return buffer;
}

void
AngleDataField::SetAsInteger(int _value) noexcept
{
  ModifyValue(_value);
}

void
AngleDataField::SetAsString(const TCHAR *_value) noexcept
{
  ModifyValue(Angle::Degrees(ParseDouble(_value)));
}

void
AngleDataField::Inc() noexcept
{
  ModifyValue(value + step);
}

void
AngleDataField::Dec() noexcept
{
  ModifyValue(MAX + value - step);
}

void
AngleDataField::SetFromCombo(int i, gcc_unused const TCHAR *s) noexcept
{
  assert(i >= 0);
  assert(unsigned(i) < MAX);

  ModifyValue(unsigned(i));
}

static void
AppendComboValue(ComboList &combo_list, unsigned value) noexcept
{
  TCHAR buffer1[ARRAY_SIZE(buffer)], buffer2[ARRAY_SIZE(buffer)];
  _stprintf(buffer1, _T("%u"), value);
  _stprintf(buffer2, _T("%u°"), value);
  combo_list.Append(value, buffer1, buffer2);
}

ComboList
AngleDataField::CreateComboList(const TCHAR *reference) const noexcept
{
  ComboList combo_list;

  const unsigned fine_step = std::max(1u, step / 10u);
  const unsigned fine_start_value = (value >= step) ? value - step : 0;
  const unsigned fine_stop_value = value + step;

  bool found_current = false;
  bool in_fine_step = false;
  unsigned current_step = step;
  unsigned i = 0;

  while (i < MAX) {
    if (!found_current && value <= i) {
      combo_list.current_index = combo_list.size();

      if (value < i)
        /* the current value is not listed - insert it here */
        AppendComboValue(combo_list, value);

      found_current = true;
    }

    AppendComboValue(combo_list, i);

    if (fine) {
      if (i + current_step > fine_stop_value) {
        if (in_fine_step) {
          in_fine_step = false;
          current_step = step;
          i = ((i + step) / step) * step;
        } else
          i += current_step;
      } else if (i + current_step > fine_start_value) {
        if (!in_fine_step) {
          in_fine_step = true;
          current_step = fine_step;
          i = fine_start_value + fine_step;
        } else
          i += current_step;
      } else
        i += current_step;
    } else
      i += current_step;
  }

  if (!found_current) {
    /* the current value out of range - append it here */
    combo_list.current_index = combo_list.size();
    AppendComboValue(combo_list, value);
  }

  return combo_list;
}
