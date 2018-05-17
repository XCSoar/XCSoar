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

#ifndef XCSOAR_DEBUG_REPLAY_HPP
#define XCSOAR_DEBUG_REPLAY_HPP

#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Computer/BasicComputer.hpp"
#include "Computer/FlyingComputer.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Time/WrapClock.hpp"
#include "OS/Args.hpp"
#include "Atmosphere/Pressure.hpp"


class DebugReplay {
protected:
  GlidePolar glide_polar;

  BasicComputer computer;
  FlyingComputer flying_computer;

  /**
   * Raw values parsed from the NMEA/IGC file.
   */
  NMEAInfo raw_basic;

  /**
   * A copy of #raw_basic with #BasicComputer changes.
   */
  MoreData computed_basic;

  /**
   * The #computed_basic value from the previous iteration.
   */
  MoreData last_basic;

  DerivedInfo calculated;

  WrapClock wrap_clock;

  AtmosphericPressure qnh;

public:
  DebugReplay();
  virtual ~DebugReplay();

  virtual long Size() const = 0;
  virtual long Tell() const = 0;
  virtual bool Next() = 0;

  /* Return a detail level for this fix - only used for skylines */
  virtual int Level() const {
    return 0;
  }

  const MoreData &Basic() const {
    return computed_basic;
  }

  const MoreData &LastBasic() const {
    return last_basic;
  }

  const DerivedInfo &Calculated() const {
    return calculated;
  }

  DerivedInfo &SetCalculated() {
    return calculated;
  }

  FlyingComputer &SetFlyingComputer() {
    return flying_computer;
  }

  void SetQNH(const AtmosphericPressure _qnh) {
    qnh = _qnh;
  }

protected:
  void Compute();
};

DebugReplay *
CreateDebugReplay(Args &args);

#endif
