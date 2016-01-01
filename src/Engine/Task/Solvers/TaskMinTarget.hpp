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
#ifndef TASKMINTARGET_HPP
#define TASKMINTARGET_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Math/ZeroFinder.hpp"

#include <vector>

class StartPoint;

/**
 * Optimise target ranges (for adjustable tasks) to produce an estimated
 * time remaining with the current glide polar, equal to a target value.
 *
 * Targets are adjusted along line from min to max linearly; only
 * current and later task points are adjusted.
 *
 * \todo
 * - Allow for other schemes or weightings in how much to adjust each
 *   target.
 */
class TaskMinTarget final : private ZeroFinder {
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AircraftState &aircraft;
  const double t_remaining;
  StartPoint *tp_start;
  bool force_current;

public:
  /**
   * Constructor for ordered task points
   *
   * @param tps Vector of ordered task points comprising the task
   * @param activeTaskPoint Current active task point in sequence
   * @param _aircraft Current aircraft state
   * @param _gp Glide polar to copy for calculations
   * @param _t_remaining Desired time remaining (s) of task
   * @param _ts StartPoint of task (to initiate scans)
   */
  TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AircraftState &_aircraft,
                const GlideSettings &settings, const GlidePolar &_gp,
                double _t_remaining,
                StartPoint *_ts);

private:
  virtual double f(double p);

  /**
   * Test validity of a solution given search parameter
   *
   * @param p Search parameter (target range parameter [0,1])
   *
   * @return True if solution is valid
   */
  bool valid(double p);

public:
  /**
   * Search for target range to produce remaining time equal to
   * value specified in constructor.
   *
   * Running this adjusts the target values for AAT task points.
   *
   * @param p Default range (0-1)
   *
   * @return Range value for solution
   */
  double search(double p);

private:
  void set_range(double p);
};

#endif

