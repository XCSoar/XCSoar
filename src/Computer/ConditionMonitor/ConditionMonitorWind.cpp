// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitorWind.hpp"
#include "NMEA/Derived.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"

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
