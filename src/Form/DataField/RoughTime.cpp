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

#include "RoughTime.hpp"
#include "Time/BrokenDateTime.hpp"

#include <stdio.h>

static TCHAR buffer[6];

void
RoughTimeDataField::ModifyValue(RoughTime _value)
{
  if (_value == value)
    return;

  value = _value;
  Modified();
}

int
RoughTimeDataField::GetAsInteger() const
{
  return value.IsValid()
    ? value.GetMinuteOfDay()
    : -1;
}

const TCHAR *
RoughTimeDataField::GetAsString() const
{
  if (!value.IsValid())
    return _T("");

  _stprintf(buffer, _T("%02u:%02u"), value.GetHour(), value.GetMinute());
  return buffer;
}

const TCHAR *
RoughTimeDataField::GetAsDisplayString() const
{
  if (!value.IsValid())
    return _T("");

  RoughTime local_value = value + time_zone;
  _stprintf(buffer, _T("%02u:%02u"),
            local_value.GetHour(), local_value.GetMinute());
  return buffer;
}

void
RoughTimeDataField::Inc()
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
RoughTimeDataField::Dec()
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
