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

#ifndef XCSOAR_CLIMB_INFO_HPP
#define XCSOAR_CLIMB_INFO_HPP

#include <type_traits>

/** Information about one thermal that was or is being climbed. */
struct OneClimbInfo
{
  /** Time when circling started. */
  double start_time;

  /**
   * Time when circling ended
   * (or current time stamp if circling has not ended yet).
   */
  double end_time;

  /** Time spent in this thermal [s]. */
  double duration;

  /** Altitude gained while in the thermal [m]. May be negative. */
  double gain;

  /** Start altitude [m] */
  double start_altitude;

  /** Average vertical speed in the thermal [m/s]. May be negative. */
  double lift_rate;

  void Clear();

  bool IsDefined() const {
    return duration > 0;
  }

  void CalculateDuration() {
    duration = end_time - start_time;
  }

  void CalculateLiftRate() {
    lift_rate = IsDefined()
      ? gain / duration
      : 0.;
  }

  void CalculateAll() {
    CalculateDuration();
    CalculateLiftRate();
  }
};

/** Derived climb data */
struct ClimbInfo
{
  OneClimbInfo current_thermal;

  OneClimbInfo last_thermal;

  /** Average vertical speed in the last thermals smoothed by low-pass-filter */
  double last_thermal_average_smooth;

  void Clear();
};

static_assert(std::is_trivial<ClimbInfo>::value, "type is not trivial");

#endif
