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
#ifndef TASKGLIDEREQUIRED_HPP
#define TASKGLIDEREQUIRED_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Math/ZeroFinder.hpp"

#include <vector>

/**
 *  Class to solve for virtual sink rate such that pure glide at
 *  block MacCready speeds with this sink rate would result in
 *  a solution perfectly on final glide.
 *
 * \todo
 * - f() fails if Mc too low for wind, need to account for failed solution
 *
 */
class TaskGlideRequired final : private ZeroFinder {
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AircraftState &aircraft;

public:
  /**
   * Constructor for ordered task points
   *
   * @param tps Vector of ordered task points comprising the task
   * @param activeTaskPoint Current active task point in sequence
   * @param _aircraft Current aircraft state
   * @param gp Glide polar to copy for calculations
   */
  TaskGlideRequired(const std::vector<OrderedTaskPoint *> &tps,
                    const unsigned activeTaskPoint,
                    const AircraftState &_aircraft,
                    const GlideSettings &settings, const GlidePolar &gp);

  /**
   * Constructor for single task points (non-ordered ones)
   *
   * @param tp Task point comprising the task
   * @param _aircraft Current aircraft state
   * @param gp Glide polar to copy for calculations
   */
  TaskGlideRequired(TaskPoint* tp,
                    const AircraftState &_aircraft,
                    const GlideSettings &settings, const GlidePolar &gp);

  /**
   * Search for sink rate to produce final glide solution
   *
   * @param s Default sink rate value (m/s)
   *
   * @return Solution sink rate (m/s, down positive)
   */
  gcc_pure
  double search(double s);

private:
  /* virtual methods from class ZeroFinder */
  virtual double f(double mc) override;
};

#endif

