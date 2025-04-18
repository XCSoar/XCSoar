// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitor.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

void
ConditionMonitor::Update(const NMEAInfo &basic, const DerivedInfo &calculated,
                         const ComputerSettings &settings) noexcept
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
ConditionMonitor::Ready_Time_Notification(TimeStamp T) const noexcept
{
  if (!T.IsDefined())
    return false;

  if (!LastTime_Notification.IsDefined() || T < LastTime_Notification)
    return true;

  if (T >= LastTime_Notification + Interval_Notification)
    return true;

  return false;
}

bool
ConditionMonitor::Ready_Time_Check(TimeStamp T, bool *restart) noexcept
{
  if (!T.IsDefined())
    return false;

  if (!LastTime_Check.IsDefined() || T < LastTime_Check) {
    LastTime_Notification = TimeStamp::Undefined();
    *restart = true;
    return true;
  }

  if (T >= LastTime_Check + Interval_Check)
    return true;

  return false;
}
