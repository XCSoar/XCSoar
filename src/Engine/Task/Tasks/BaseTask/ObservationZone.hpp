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

#ifndef OBSERVATIONZONE_HPP
#define OBSERVATIONZONE_HPP

#include "Navigation/Aircraft.hpp"
#include "Compiler.h"

/**
 * Abstract class giving properties of a zone which is used to measure
 * transitions in/out of and interior/exterior checking.
 */
class ObservationZone
{
public:
  ObservationZone() {}
  virtual ~ObservationZone() {}

  /** 
   * Check whether observer is within OZ
   *
   * @return True if reference point is inside sector
   */
  gcc_pure
  virtual bool IsInSector(const AircraftState &ref) const = 0;

  /**
   * If zone when used for start can trigger task start via vertical exit
   *
   * @return True if zone type can have a valid start through top
   */
  gcc_pure
  virtual bool CanStartThroughTop() const {
    return true;
  }

  /**
   * Check transition constraints
   *
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if constraints are satisfied
   */
  gcc_pure
  virtual bool TransitionConstraint(const AircraftState &ref_now,
                                    const AircraftState &ref_last) const = 0;

  /** 
   * Check if aircraft has transitioned to inside sector
   * 
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if aircraft now inside (and was outside)
   */
  gcc_pure
  virtual bool CheckEnterTransition(const AircraftState &ref_now,
                                    const AircraftState &ref_last) const {
    return IsInSector(ref_now) &&
           !IsInSector(ref_last) &&
           TransitionConstraint(ref_now, ref_last);
  }

  /** 
   * Check if aircraft has transitioned to outside sector
   * 
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if aircraft now outside (and was inside)
   */
  gcc_pure
  virtual bool CheckExitTransition(const AircraftState &ref_now,
                                   const AircraftState &ref_last) const {
    return CheckEnterTransition(ref_last, ref_now);
  }

  /**
   * Get point on boundary from parametric representation
   *
   * @param t T value [0,1]
   *
   * @return Point on boundary
   */
  gcc_pure
  virtual GeoPoint GetBoundaryParametric(fixed t) const = 0;

  /**
   * Distance reduction for scoring when outside this OZ
   * (used because FAI cylinders, for example, have their
   *  radius subtracted from scored distances)
   *
   * @return Distance (m) to subtract from score
   */
  gcc_pure
  virtual fixed ScoreAdjustment() const = 0;
};

#endif
