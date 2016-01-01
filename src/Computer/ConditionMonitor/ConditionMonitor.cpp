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

#include "ConditionMonitor.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

void
ConditionMonitor::Update(const NMEAInfo &basic, const DerivedInfo &calculated,
                         const ComputerSettings &settings)
{
  if (!calculated.flight.flying)
    return;

  bool restart = false;
  const auto Time = basic.time;
  if (Ready_Time_Check(Time, &restart)) {
    LastTime_Check = Time;
    if (CheckCondition(basic, calculated, settings)) {
      if (Ready_Time_Notification(Time) && !restart) {
        LastTime_Notification = Time;
        Notify();
        SaveLast();
      }
    }

    if (restart)
      SaveLast();
  }
}

bool
ConditionMonitor::Ready_Time_Notification(double T)
{
  if (T <= 0)
    return false;

  if (LastTime_Notification < 0 || T < LastTime_Notification)
    return true;

  if (T >= LastTime_Notification + Interval_Notification)
    return true;

  return false;
}

bool
ConditionMonitor::Ready_Time_Check(double T, bool *restart)
{
  if (T <= 0)
    return false;

  if (LastTime_Check < 0 || T < LastTime_Check) {
    LastTime_Notification = -1;
    *restart = true;
    return true;
  }

  if (T >= LastTime_Check + Interval_Check)
    return true;

  return false;
}
