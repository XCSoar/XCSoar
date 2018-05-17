/* Copyright_License {

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
#ifndef ROUTEPOLAR_HPP
#define ROUTEPOLAR_HPP

#include "Compiler.h"

class Angle;
class GlidePolar;
struct GlideSettings;
struct GlideResult;
struct SpeedVector;
struct FlatGeoPoint;

#define ROUTEPOLAR_Q0 (6)
#define ROUTEPOLAR_Q1 (2*ROUTEPOLAR_Q0-1)
#define ROUTEPOLAR_Q2 (4*ROUTEPOLAR_Q0)
#define ROUTEPOLAR_Q3 (8*ROUTEPOLAR_Q0)
#define ROUTEPOLAR_POINTS (ROUTEPOLAR_Q3+1)

/**
 * Class to store fast lookup aircraft performance (glide slope and speed) as a
 * function of direction, for a particular GlidePolar and Wind condition.
 * Enables fast performance simulation without using GlidePolar calls.
 *
 */
class RoutePolar
{
  /**
   * Structure to hold aircraft performance for a single direction
   */
  struct RoutePolarPoint
  {
    /** Inverse speed (s/m) */
    double slowness;
    /** Glide slope gradient (m loss / m travelled) */
    double gradient;
    /** Reciprocal gradient (m travelled / m loss) */
    double inv_gradient;
    /** Whether this solution is valid (non-zero speed) */
    bool valid;

    RoutePolarPoint() = default;

    RoutePolarPoint(double _slowness, double _gradient)
      :slowness(_slowness), gradient(_gradient), valid(true)
    {
      if (gradient > 0)
        inv_gradient = 1. / gradient;
      else
        inv_gradient = 0;
    };
  };

  RoutePolarPoint points[ROUTEPOLAR_POINTS];

public:
  /**
   * Populate internal structure with performance data.
   * To be called when the glide polar settings or wind changes.
   *
   * @param polar GlidePolar used to obtain performance data
   * @param wind Wind condition
   * @param glide Whether pure glide or cruise-climb is enforced
   */
  void Initialise(const GlideSettings &settings, const GlidePolar& polar,
                  const SpeedVector& wind,
                  const bool glide);

  /**
   * Retrieve data corresponding to a particular (backwards-time) direction.
   *
   * @param index Index of direction (no range checking is performed!)
   *
   * @return RoutePolarPoint data corresponding to this direction index
   */
  const RoutePolarPoint& GetPoint(const int index) const {
    return points[index];
  }

  /**
   * Calculate distances normalised to 128 corresponding to direction index
   *
   * @param index Direction index
   * @param dx X distance units
   * @param dy Y distance units
   */
  gcc_const
  static FlatGeoPoint IndexToDXDY(int index);

private:
  GlideResult SolveTask(const GlideSettings &settings, const GlidePolar& polar,
                        const SpeedVector &wind,
                        const Angle theta, const bool glide) const;
};

#endif
