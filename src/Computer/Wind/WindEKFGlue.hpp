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

#ifndef WINDEKF_GLUE_HPP
#define WINDEKF_GLUE_HPP

#include "WindEKF.hpp"
#include "NMEA/Validity.hpp"
#include "Geo/SpeedVector.hpp"

struct NMEAInfo;
struct DerivedInfo;

class WindEKFGlue
{
  /**
   * time to not add points after flight condition is false
   */
  static constexpr unsigned BLACKOUT_TIME = 3;

  WindEKF ekf;

  /**
   * When this flag is true, then WindEKF::Init() should be called
   * before the WindEKF object is used.  This flag is used to postpone
   * the initialisation to the time when WindEKF is really needed, to
   * reduce the Reset() overhead when no airspeed indicator is
   * available.
   */
  bool reset_pending;

  /**
   * These attributes are used to check if updated values are
   * available since the last call.
   */
  Validity last_ground_speed_available, last_airspeed_available;

  /**
   * The number of samples we have fed into the #WindEKF.  This is
   * used to determine the quality.
   */
  unsigned i;

  unsigned time_blackout;

public:
  struct Result
  {
    SpeedVector wind;
    int quality;

    Result() {}
    Result(int _quality):quality(_quality) {}
  };

  void Reset();

  Result Update(const NMEAInfo &basic, const DerivedInfo &derived);

private:
  void ResetBlackout() {
    time_blackout = 0;
  }

  bool InBlackout(const unsigned time) const {
    return time < time_blackout;
  }

  void SetBlackout(const unsigned time) {
    time_blackout = time + BLACKOUT_TIME;
  }
};

#endif
