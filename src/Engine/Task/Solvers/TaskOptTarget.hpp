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

#ifndef TASKOPTTARGET_HPP
#define TASKOPTTARGET_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Task/Ordered/AATIsolineSegment.hpp"
#include "Math/ZeroFinder.hpp"

#include <vector>

class StartPoint;

/**
 * Adjust target lateral offset for active task point to minimise
 * elapsed time.
 *
 * \todo
 * - Merge with TaskMinTarget?
 * - Refactor, since passing in AATPoint is a hack
 */
class TaskOptTarget final : public ZeroFinder
{
  /** Object to calculate remaining task statistics */
  TaskMacCreadyRemaining tm;
  /** Glide solution used in search */
  GlideResult res;
  /** Observer */
  const AircraftState &aircraft;
  /** Start of task */
  StartPoint *tp_start;
  /** Active AATPoint */
  AATPoint &tp_current;
  /** Isoline for active AATPoint target */
  AATIsolineSegment iso;

public:
  /**
   * Constructor for ordered task points
   *
   * @param tps Vector of ordered task points comprising the task
   * @param activeTaskPoint Current active task point in sequence
   * @param _aircraft Current aircraft state
   * @param _gp Glide polar to copy for calculations
   * @param _tp_current Active AATPoint
   * @param _ts StartPoint of task (to initiate scans)
   */
  TaskOptTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AircraftState &_aircraft,
                const GlideSettings &settings, const GlidePolar &_gp,
                AATPoint& _tp_current,
                const FlatProjection &projection,
                StartPoint *_ts);

  virtual double f(double p);

  /**
   * Test validity of a solution given search parameter
   *
   * @param p Search parameter (isoline parameter [0,1])
   *
   * @return True if solution is valid
   */
  virtual bool valid(double p);

  /**
   * Search for active task point's target isoline to minimise elapsed time
   * to finish.
   *
   * Running this adjusts the target values for the active task point.
   *
   * @param p Default isoline value (0-1)
   *
   * @return Isoline value for solution
   */
  virtual double search(double p);

private:
  /** Sets target location along isoline */
  void SetTarget(double p);
};


#endif
