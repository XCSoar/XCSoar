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

#include "ConditionMonitorFinalGlide.hpp"
#include "NMEA/Derived.hpp"
#include "Input/InputQueue.hpp"

bool
ConditionMonitorFinalGlide::CheckCondition(const NMEAInfo &basic,
                                           const DerivedInfo &calculated,
                                           const ComputerSettings &settings)
{
  if (!calculated.flight.flying || !calculated.task_stats.task_valid)
    return false;

  const GlideResult &res = calculated.task_stats.total.solution_remaining;

  // TODO: use low pass filter
  tad = res.altitude_difference * 0.2 + 0.8 * tad;

  bool BeforeFinalGlide = !res.IsFinalGlide();

  if (BeforeFinalGlide) {
    Interval_Notification = 60 * 5;
    if (tad > 50 && last_tad < -50)
      // report above final glide early
      return true;
    else if (tad < -50)
      last_tad = tad;
  } else {
    Interval_Notification = 60;
    if (res.IsFinalGlide()) {
      if (last_tad < -50 && tad > 1)
        // just reached final glide, previously well below
        return true;

      if (last_tad > 1 && tad < -50) {
        // dropped well below final glide, previously above
        last_tad = tad;
        return true; // JMW this was true before
      }
    }
  }
  return false;
}

void
ConditionMonitorFinalGlide::Notify()
{
  if (tad > 1)
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_ABOVE);
  if (tad < -1)
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_BELOW);
}

void
ConditionMonitorFinalGlide::SaveLast()
{
  last_tad = tad;
}
