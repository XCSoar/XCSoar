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

#include "ConditionMonitorLandableReachable.hpp"
#include "NMEA/Derived.hpp"
#include "Input/InputQueue.hpp"

bool
ConditionMonitorLandableReachable::CheckCondition([[maybe_unused]] const NMEAInfo &basic,
                                                  const DerivedInfo &calculated,
                                                  [[maybe_unused]] const ComputerSettings &settings) noexcept
{
  if (!calculated.flight.flying)
    return false;

  now_reachable = calculated.common_stats.landable_reachable;

  // warn when becoming unreachable
  return (!now_reachable && last_reachable);
}

void
ConditionMonitorLandableReachable::Notify() noexcept
{
  InputEvents::processGlideComputer(GCE_LANDABLE_UNREACHABLE);
}

void
ConditionMonitorLandableReachable::SaveLast() noexcept
{
  last_reachable = now_reachable;
}
