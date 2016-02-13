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

#include "Time.hpp"
#include "ComboList.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Util/StringFormat.hpp"
#include "Util/NumberParser.hpp"

static bool data_field_key_up = false;

const TCHAR *
DataFieldTime::GetAsString() const
{
  StringFormatUnsafe(string_buffer, _T("%d"), value);
  return string_buffer;
}

const TCHAR *
DataFieldTime::GetAsDisplayString() const
{
  FormatTimespanSmart(string_buffer, value, max_tokens);
  return string_buffer;
}

void
DataFieldTime::Inc()
{
  SetValue(value + step * SpeedUp(true));
}

void
DataFieldTime::Dec()
{
  SetValue(value - step * SpeedUp(false));
}

int
DataFieldTime::SpeedUp(bool key_up)
{
  if (key_up != data_field_key_up) {
    speedup = 0;
    data_field_key_up = key_up;
    last_step.Update();
    return 1;
  }

  if (!last_step.Check(200)) {
    speedup++;
    if (speedup > 5) {
      last_step.UpdateWithOffset(350);
      return 10;
    }
  } else
    speedup = 0;

  last_step.Update();

  return 1;
}

void
DataFieldTime::SetFromCombo(int data_field_index,
                            gcc_unused const TCHAR *value_string)
{
  SetValue(data_field_index);
}

void
DataFieldTime::AppendComboValue(ComboList &combo_list, int value) const
{
  TCHAR buffer2[32];
  StringFormatUnsafe(buffer2, _T("%d"), value);
  combo_list.Append(value, buffer2, FormatTimespanSmart(value, max_tokens));
}

ComboList
DataFieldTime::CreateComboList(const TCHAR *reference_string) const
{
  const int reference = reference_string != nullptr
    ? ParseInt(reference_string)
    : value;

  ComboList combo_list;

  /* how many items before and after the current value? */
  unsigned surrounding_items = ComboList::MAX_SIZE / 2 - 2;

  /* the value aligned to mStep */
  int corrected_value = ((reference - min) / step) * step + min;

  int first = corrected_value - (int)surrounding_items * step;
  if (first > min)
    /* there are values before "first" - give the user a choice */
    combo_list.Append(ComboList::Item::PREVIOUS_PAGE, _T("<<More Items>>"));
  else if (first < min)
    first = min;

  int last = std::min(first + (int)(surrounding_items * step * 2), max);

  bool found_current = false;
  for (int i = first; i <= last; i += step) {
    if (!found_current && reference <= i) {
      if (reference < i)
        /* the current value is not listed - insert it here */
        AppendComboValue(combo_list, reference);

      combo_list.current_index = combo_list.size();
      found_current = true;
    }

    AppendComboValue(combo_list, i);
  }

  if (reference > last) {
    /* the current value out of range - append it here */
    last = reference;
    AppendComboValue(combo_list, reference);
    combo_list.current_index = combo_list.size();
  }

  if (last < max)
    /* there are values after "last" - give the user a choice */
    combo_list.Append(ComboList::Item::NEXT_PAGE, _T("<<More Items>>"));

  return combo_list;
}
