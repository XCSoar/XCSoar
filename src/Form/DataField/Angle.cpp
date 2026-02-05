// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Angle.hpp"
#include "ComboList.hpp"
#include "util/Macros.hpp"

#include <stdio.h>

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

const char *
AngleDataField::GetAsString() const noexcept
{
  _stprintf(string_buffer, _T("%u"), GetIntegerValue());
  return string_buffer;
}

const char *
AngleDataField::GetAsDisplayString() const noexcept
{
  _stprintf(string_buffer, _T("%u°"), GetIntegerValue());
  return string_buffer;
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
AngleDataField::SetFromCombo(int i, [[maybe_unused]] const char *s) noexcept
{
  assert(i >= 0);
  assert(unsigned(i) < MAX);

  ModifyValue(unsigned(i));
}

static void
AppendComboValue(ComboList &combo_list, unsigned value) noexcept
{
  char buffer1[16], buffer2[16];
  _stprintf(buffer1, _T("%u"), value);
  _stprintf(buffer2, _T("%u°"), value);
  combo_list.Append(value, buffer1, buffer2);
}

ComboList
AngleDataField::CreateComboList([[maybe_unused]] const char *reference) const noexcept
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
