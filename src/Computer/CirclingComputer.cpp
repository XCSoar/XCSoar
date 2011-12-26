/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "CirclingComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "ComputerSettings.hpp"
#include "Math/LowPassFilter.hpp"

static const fixed MinTurnRate(4);
static const fixed CruiseClimbSwitch(15);
static const fixed ClimbCruiseSwitch(10);

/**
 * Calculates the turn rate
 */
void
CirclingComputer::TurnRate(CirclingInfo &circling_info,
                           const NMEAInfo &basic, const NMEAInfo &last_basic,
                           const DerivedInfo &calculated,
                           const DerivedInfo &last_calculated)
{
  // Calculate turn rate

  if (!basic.time_available || !last_basic.time_available ||
      !calculated.flight.flying) {
    circling_info.turn_rate = fixed_zero;
    circling_info.turn_rate_heading = fixed_zero;
    return;
  }

  if (!basic.HasTimeAdvancedSince(last_basic))
    return;

  // Calculate time passed since last calculation
  const fixed dT = basic.time - last_basic.time;
  assert(positive(dT));

  circling_info.turn_rate =
    (basic.track - last_basic.track).AsDelta().Degrees() / dT;
  circling_info.turn_rate_heading =
    (calculated.heading - last_calculated.heading).AsDelta().Degrees() / dT;
}

void
CirclingComputer::Turning(CirclingInfo &circling_info,
                          const MoreData &basic, const MoreData &last_basic,
                          const DerivedInfo &calculated,
                          const DerivedInfo &last_calculated,
                          const ComputerSettings &settings_computer)
{
  // You can't be circling unless you're flying
  if (!calculated.flight.flying || !basic.HasTimeAdvancedSince(last_basic))
    return;

  // JMW limit rate to 50 deg per second otherwise a big spike
  // will cause spurious lock on circling for a long time
  fixed Rate = max(fixed(-50), min(fixed(50), calculated.turn_rate));

  // Make the turn rate more smooth using the LowPassFilter
  Rate = LowPassFilter(last_calculated.turn_rate_smoothed, Rate, fixed(0.3));
  circling_info.turn_rate_smoothed = Rate;
  circling_info.turning = fabs(Rate) >= MinTurnRate;

  // Force cruise or climb mode if external device says so
  bool forcecruise = false;
  bool forcecircling = false;
  if (settings_computer.external_trigger_cruise_enabled && !basic.gps.replay) {
    switch (basic.switch_state.flight_mode) {
    case SwitchInfo::FlightMode::UNKNOWN:
      forcecircling = false;
      forcecruise = false;
      break;

    case SwitchInfo::FlightMode::CIRCLING:
      forcecircling = true;
      forcecruise = false;
      break;

    case SwitchInfo::FlightMode::CRUISE:
      forcecircling = false;
      forcecruise = true;
      break;
    }
  }

  switch (calculated.turn_mode) {
  case CirclingMode::CRUISE:
    // If (in cruise mode and beginning of circling detected)
    if (circling_info.turning || forcecircling) {
      // Remember the start values of the turn
      circling_info.turn_start_time = basic.time;
      circling_info.turn_start_location = basic.location;
      circling_info.turn_start_altitude = basic.nav_altitude;
      circling_info.turn_start_energy_height = basic.energy_height;
      circling_info.turn_mode = CirclingMode::POSSIBLE_CLIMB;
    }
    if (!forcecircling)
      break;

  case CirclingMode::POSSIBLE_CLIMB:
    if (forcecruise) {
      circling_info.turn_mode = CirclingMode::CRUISE;
      break;
    }
    if (circling_info.turning || forcecircling) {
      if (((basic.time - calculated.turn_start_time) > CruiseClimbSwitch)
          || forcecircling) {
        // yes, we are certain now that we are circling
        circling_info.circling = true;

        // JMW Transition to climb
        circling_info.turn_mode = CirclingMode::CLIMB;

        // Remember the start values of the climbing period
        circling_info.climb_start_location = circling_info.turn_start_location;
        circling_info.climb_start_altitude = circling_info.turn_start_altitude
            + circling_info.turn_start_energy_height;
        circling_info.climb_start_time = circling_info.turn_start_time;
      }
    } else {
      // nope, not turning, so go back to cruise
      circling_info.turn_mode = CirclingMode::CRUISE;
    }
    break;

  case CirclingMode::CLIMB:
    if (!circling_info.turning || forcecruise) {
      // Remember the end values of the turn
      circling_info.turn_start_time = basic.time;
      circling_info.turn_start_location = basic.location;
      circling_info.turn_start_altitude = basic.nav_altitude;
      circling_info.turn_start_energy_height = basic.energy_height;

      // JMW Transition to cruise, due to not properly turning
      circling_info.turn_mode = CirclingMode::POSSIBLE_CRUISE;
    }
    if (!forcecruise)
      break;

  case CirclingMode::POSSIBLE_CRUISE:
    if (forcecircling) {
      circling_info.turn_mode = CirclingMode::CLIMB;
      break;
    }

    if (!circling_info.turning || forcecruise) {
      if (((basic.time - circling_info.turn_start_time) > ClimbCruiseSwitch)
          || forcecruise) {
        // yes, we are certain now that we are cruising again
        circling_info.circling = false;

        // Transition to cruise
        circling_info.turn_mode = CirclingMode::CRUISE;
        circling_info.cruise_start_location = circling_info.turn_start_location;
        circling_info.cruise_start_altitude = circling_info.turn_start_altitude;
        circling_info.cruise_start_time = circling_info.turn_start_time;
      }
    } else {
      // nope, we are circling again
      // JMW Transition back to climb, because we are turning again
      circling_info.turn_mode = CirclingMode::CLIMB;
    }
    break;

  default:
    // error, go to cruise
    circling_info.turn_mode = CirclingMode::CRUISE;
  }
}
