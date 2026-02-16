// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Integer.hpp"
#include "ComboList.hpp"
#include "util/NumberParser.hpp"

#include <stdio.h>

static bool datafield_key_up = false;

[[gnu::pure]]
static int
ParseString(const char *s) noexcept
{
  return ParseInt(s);
}

const char *
DataFieldInteger::GetAsString() const noexcept
{
  _stprintf(output_buffer, edit_format, value);
  return output_buffer;
}

const char *
DataFieldInteger::GetAsDisplayString() const noexcept
{
  _stprintf(output_buffer, display_format, value);
  return output_buffer;
}

void
DataFieldInteger::SetAsInteger(int _value) noexcept
{
  if (_value < min)
    _value = min;
  if (_value > max)
    _value = max;
  ModifyValue(_value);
}

void
DataFieldInteger::Inc() noexcept
{
  SetAsInteger(value + step * SpeedUp(true));
}

void
DataFieldInteger::Dec() noexcept
{
  SetAsInteger(value - step * SpeedUp(false));
}

int
DataFieldInteger::SpeedUp(bool keyup) noexcept
{
  int res = 1;

  if (keyup != datafield_key_up) {
    speedup = 0;
    datafield_key_up = keyup;
    last_step.Update();
    return 1;
  }

  if (!last_step.Check(std::chrono::milliseconds(200))) {
    speedup++;
    if (speedup > 5) {
      res = 10;
      last_step.UpdateWithOffset(std::chrono::milliseconds(350));
      return (res);
    }
  } else
    speedup = 0;

  last_step.Update();

  return res;
}

void
DataFieldInteger::AppendComboValue(ComboList &combo_list,
                                   int value) const noexcept
{
  char a[decltype(edit_format)::capacity()], b[decltype(display_format)::capacity()];
  _stprintf(a, edit_format, value);
  _stprintf(b, display_format, value);
  combo_list.Append(combo_list.size(), a, b);
}

ComboList
DataFieldInteger::CreateComboList(const char *reference_string) const noexcept
{
  const int reference = reference_string != nullptr
    ? ParseString(reference_string)
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

  int last = std::min(first + (int)surrounding_items * step * 2, max);

  bool found_current = false;
  for (int i = first; i <= last; i += step) {
    if (!found_current && reference <= i) {
      combo_list.current_index = combo_list.size();

      if (reference < i)
        /* the current value is not listed - insert it here */
        AppendComboValue(combo_list, reference);

      found_current = true;
    }

    AppendComboValue(combo_list, i);
  }

  if (reference > last) {
    /* the current value out of range - append it here */
    last = reference;
    combo_list.current_index = combo_list.size();
    AppendComboValue(combo_list, reference);
  }

  if (last < max)
    /* there are values after "last" - give the user a choice */
    combo_list.Append(ComboList::Item::NEXT_PAGE, _T("<<More Items>>"));

  return combo_list;
}

void
DataFieldInteger::SetFromCombo([[maybe_unused]] int index,
                               const char *value) noexcept
{
  SetAsInteger(ParseString(value));
}
