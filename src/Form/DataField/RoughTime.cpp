// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RoughTime.hpp"
#include "time/BrokenDateTime.hpp"

#include <stdio.h>

static char buffer[8];

void
RoughTimeDataField::ModifyValue(RoughTime _value) noexcept
{
  if (_value == value)
    return;

  value = _value;
  Modified();
}

const char *
RoughTimeDataField::GetAsString() const noexcept
{
  if (!value.IsValid())
    return _T("");

  _stprintf(buffer, _T("%02u:%02u"), value.GetHour(), value.GetMinute());
  return buffer;
}

const char *
RoughTimeDataField::GetAsDisplayString() const noexcept
{
  if (!value.IsValid())
    return _T("");

  RoughTime local_value = value + time_zone;
  _stprintf(buffer, _T("%02u:%02u"),
            local_value.GetHour(), local_value.GetMinute());
  return buffer;
}

void
RoughTimeDataField::Inc() noexcept
{
  RoughTime new_value = value;
  if (new_value.IsValid())
    ++new_value;
  else {
    const BrokenTime bt = BrokenDateTime::NowUTC();
    new_value = RoughTime(bt.hour, bt.minute);
  }

  ModifyValue(new_value);
}

void
RoughTimeDataField::Dec() noexcept
{
  RoughTime new_value = value;
  if (new_value.IsValid())
    --new_value;
  else {
    const BrokenTime bt = BrokenDateTime::NowUTC();
    new_value = RoughTime(bt.hour, bt.minute);
  }

  ModifyValue(new_value);
}
