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
#ifndef TASK_SOLVE_TRAVELLED_HPP
#define TASK_SOLVE_TRAVELLED_HPP

#include "TaskSolveTravelled.hpp"

/**
 * Class to solve for effective MC.
 *
 * This is the MC setting that would produce the same travelled speed
 * at 100% cruise efficiency.
 *
 * This is calculated for the part of the task that has been travelled
 */
class TaskEffectiveMacCready final : public TaskSolveTravelled
{
 public:
  /**
   * Constructor for ordered task points
   *
   * @param tps Vector of ordered task points comprising the task
   * @param activeTaskPoint Current active task point in sequence
   * @param _aircraft Current aircraft state
   * @param gp Glide polar to copy for calculations
   */
  TaskEffectiveMacCready(const std::vector<OrderedTaskPoint *> &tps,
                         const unsigned activeTaskPoint,
                         const AircraftState &_aircraft,
                         const GlideSettings &settings, const GlidePolar &gp);

protected:
  /* virtual methods from class ZeroFinder */
  virtual double f(const double x) override;
};

#endif
