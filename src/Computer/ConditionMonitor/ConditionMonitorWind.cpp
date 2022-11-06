/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
