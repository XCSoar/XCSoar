// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitorWind.hpp"
#include "NMEA/Derived.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "LogFile.hpp"

bool
ConditionMonitorWind::CheckCondition([[maybe_unused]] const NMEAInfo &basic,
                                     const DerivedInfo &calculated,
                                     [[maybe_unused]] const ComputerSettings &settings) noexcept
{
  wind = calculated.GetWindOrZero();

  if (!calculated.flight.flying) {
    last_wind = wind;
    return false;
  }

  auto mag_change = fabs(wind.norm - last_wind.norm);
  auto dir_change = (wind.bearing - last_wind.bearing).AsDelta().Absolute();

  // monitor wind vector
  const BrokenDateTime t = basic.date_time_utc;
  char log_buf[200];
  snprintf (log_buf,sizeof(log_buf),"MonitorWind,%d,%d,%1.1f,%1.1f,%d",
      (t.hour * 60 + t.minute) * 60 + t.second,calculated.circling,
      Units::ToUserWindSpeed(wind.norm),wind.bearing.Degrees(),(int)basic.baro_altitude);
  LogString(log_buf);

  if (mag_change > 2.5)
    return true;

  return wind.norm > 5 && dir_change > Angle::Degrees(45);
}

void
ConditionMonitorWind::Notify() noexcept
{
  Message::AddMessage(_("Significant wind change"));
}

void
ConditionMonitorWind::SaveLast() noexcept
{
  last_wind = wind;
}
