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

#include "Integer.hpp"
#include "ComboList.hpp"
#include "Math/FastMath.h"
#include "Util/NumberParser.hpp"
#include "Asset.hpp"

#include <stdlib.h>
#include <stdio.h>

static bool datafield_key_up = false;

int
DataFieldInteger::GetAsInteger() const
{
  return value;
}

const TCHAR *
DataFieldInteger::GetAsString() const
{
  TCHAR *output_buffer = const_cast<TCHAR *>(this->outpuf_buffer);
  _stprintf(output_buffer, edit_format, value);
  return output_buffer;
}

const TCHAR *
DataFieldInteger::GetAsDisplayString() const
{
  TCHAR *output_buffer = const_cast<TCHAR *>(this->outpuf_buffer);
  _stprintf(output_buffer, display_format, value);
  return output_buffer;
}

void
DataFieldInteger::SetAsInteger(int _value)
{
  if (_value < min)
    _value = min;
  if (_value > max)
    _value = max;
  if (value != _value) {
    value = _value;
    Modified();
  }
}

void
DataFieldInteger::SetAsString(const TCHAR *_value)
{
  SetAsInteger(ParseInt(_value));
}

void
DataFieldInteger::Inc()
{
  SetAsInteger(value + step * SpeedUp(true));
}

void
DataFieldInteger::Dec()
{
  SetAsInteger(value - step * SpeedUp(false));
}

int
DataFieldInteger::SpeedUp(bool keyup)
{
  int res = 1;

  if (IsAltair())
    return res;

  if (keyup != datafield_key_up) {
    speedup = 0;
    datafield_key_up = keyup;
    last_step.Update();
    return 1;
  }

  if (!last_step.Check(200)) {
    speedup++;
    if (speedup > 5) {
      res = 10;
      last_step.UpdateWithOffset(350);
      return (res);
    }
  } else
    speedup = 0;

  last_step.Update();

  return res;
}

void
DataFieldInteger::AppendComboValue(ComboList &combo_list, int value) const
{
  TCHAR a[edit_format.MAX_SIZE], b[display_format.MAX_SIZE];
  _stprintf(a, edit_format, value);
  _stprintf(b, display_format, value);
  combo_list.Append(combo_list.size(), a, b);
}

ComboList *
DataFieldInteger::CreateComboList() const
{
  ComboList *combo_list = new ComboList();

  /* how many items before and after the current value? */
  unsigned surrounding_items = ComboList::MAX_SIZE / 2 - 2;

  /* the value aligned to mStep */
  int corrected_value = ((value - min) / step) * step + min;

  int first = corrected_value - (int)surrounding_items * step;
  if (first > min)
    /* there are values before "first" - give the user a choice */
    combo_list->Append(ComboList::Item::PREVIOUS_PAGE, _T("<<More Items>>"));
  else if (first < min)
    first = min;

  int last = std::min(first + (int)surrounding_items * step * 2, max);

  bool found_current = false;
  for (int i = first; i <= last; i += step) {
    if (!found_current && value <= i) {
      if (value < i)
        /* the current value is not listed - insert it here */
        AppendComboValue(*combo_list, value);

      combo_list->ComboPopupItemSavedIndex = combo_list->size() - 1;
      found_current = true;
    }

    AppendComboValue(*combo_list, i);
  }

  if (value > last) {
    /* the current value out of range - append it here */
    last = value;
    combo_list->ComboPopupItemSavedIndex = combo_list->size();
    AppendComboValue(*combo_list, value);
  }

  if (last < max)
    /* there are values after "last" - give the user a choice */
    combo_list->Append(ComboList::Item::NEXT_PAGE, _T("<<More Items>>"));

  return combo_list;
}

void
DataFieldInteger::SetFromCombo(int index, TCHAR *value)
{
  SetAsString(value);
}
