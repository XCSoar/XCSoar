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
#ifndef TASKMACCREADYTOTAL_HPP
#define TASKMACCREADYTOTAL_HPP

#include "TaskMacCready.hpp"

/**
 * Specialisation of TaskMacCready for total task
 */
class TaskMacCreadyTotal final : public TaskMacCready {
public:
  /**
   * Constructor for ordered task points
   *
   * @param _activeTaskPoint Current active task point in sequence
   * @param _gp Glide polar to copy for calculations
   */
  template<class I>
  TaskMacCreadyTotal(const I tps_begin, const I tps_end,
                     const unsigned _activeTaskPoint,
                     const GlideSettings &settings, const GlidePolar &_gp)
    :TaskMacCready(tps_begin, tps_end, _activeTaskPoint, settings, _gp) {}

  /**
   * Calculate effective distance remaining such that at the virtual
   * point, the time remaining is the same as the reference time
   * remaining (for whole task)
   *
   * @param time_remaining Time remaining (s)
   *
   * @return Effective distance remaining (m)
   */
  gcc_pure
  double effective_distance(double time_remaining) const;

  /**
   * Calculate effective distance remaining such that at the virtual
   * point, the time remaining is the same as the reference time
   * remaining (for active leg)
   *
   * @param time_remaining Time remaining (s) for active leg
   *
   * @return Effective distance remaining (m) for active leg
   */
  gcc_pure
  double effective_leg_distance(double time_remaining) const;

private:
  /* virtual methods from class TaskMacCready */
  double get_min_height(gcc_unused const AircraftState &aircraft) const override {
    return double(0);
  }

  GlideResult SolvePoint(const TaskPoint &tp,
                         const AircraftState &aircraft,
                         double minH) const override;

  AircraftState get_aircraft_start(const AircraftState &aircraft) const override;
};


#endif
