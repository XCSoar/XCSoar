/* Copyright_License {

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
#ifndef GLIDESTATE_HPP
#define GLIDESTATE_HPP

#include "Navigation/SpeedVector.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

/**
 * Class used to define a glide/navigation task
 */
struct GlideState 
{
  /** Distance/bearing of task */
  GeoVector Vector;
  /** Height (m above MSL) of end */
  fixed MinHeight;
  /** Direction of wind (deg True) */
  Angle WindDirection;
  /** Aircraft height less target height */
  fixed AltitudeDifference;

  /** (internal use) */
  fixed EffectiveWindSpeed;
  /** (internal use) */
  Angle EffectiveWindAngle;
  /** (internal use) */
  fixed wsq_;
  /** (internal use) */
  fixed dwcostheta_;
  /** headwind component (m/s) in cruise */
  fixed HeadWind;

  /**
   * Dummy task constructor.  Typically used for synthetic glide
   * tasks.  Where there are real targets, the other constructors should
   * be used instead.
   *
   * @param vector Specified vector for task
   * @param htarget Height of target (m above MSL)
   * @param altitude the altitude of the aircraft
   * @param wind the wind vector
   *
   * @return Initialised glide task
   */
  GlideState(const GeoVector &vector, const fixed htarget,
             fixed altitude, const SpeedVector wind);

  /**
   * Calculate internal quantities to reduce computation time
   * by clients of this class
   *
   * @param wind the wind vector
   */
  void calc_speedups(const SpeedVector wind);

  /**
   * Calculates average cross-country speed from effective
   * cross-country speed (accounting for wind)
   *
   * @param Veff Effective cruise speed (m/s)
   *
   * @return Average cross-country speed (m/s)
   */
  fixed calc_ave_speed(const fixed Veff) const;

  /**
   * Calculate distance a circling aircraft will drift
   * in a given time
   *
   * @param t_climb Time spent in climb (s)
   *
   * @return Distance (m) of drift
   */
  fixed drifted_distance(const fixed t_climb) const;
};

#endif
