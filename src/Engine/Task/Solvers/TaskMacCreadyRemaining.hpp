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
#ifndef TASKMACCREADYREMAINING_HPP
#define TASKMACCREADYREMAINING_HPP

#include "TaskMacCready.hpp"
#include "Geo/GeoPoint.hpp"

/**
 * Specialisation of TaskMacCready for task remaining
 */
class TaskMacCreadyRemaining final : public TaskMacCready {
  /**
   * Include the travel necessary to reach the first task point?
   */
  const bool include_travel_to_start;

  /**
   * Storage used by target_save() and target_restore().
   */
  std::array<GeoPoint, MAX_SIZE> saved_targets;

public:
  /**
   * Constructor for ordered task points
   *
   * @param _activeTaskPoint Current active task point in sequence
   * @param _gp Glide polar to copy for calculations
   */
  template<class I>
  TaskMacCreadyRemaining(const I tps_begin, const I tps_end,
                         const unsigned _activeTaskPoint,
                         const GlideSettings &settings, const GlidePolar &_gp,
                         const bool _include_travel_to_start=true)
    :TaskMacCready(std::next(tps_begin, _activeTaskPoint), tps_end, 0,
                   settings, _gp),
     include_travel_to_start(_include_travel_to_start) {}

  /**
   * Constructor for single task points (non-ordered ones)
   *
   * @param tp Task point comprising the task
   * @param gp Glide polar to copy for calculations
   */
  TaskMacCreadyRemaining(TaskPoint* tp,
                         const GlideSettings &settings, const GlidePolar &gp)
    :TaskMacCready(tp, settings, gp),
     include_travel_to_start(true) {}

  /**
   * Set ranges of all remaining task points
   *
   * @param tp Range parameter [0,1]
   * @param force_current If true, will force active AAT point (even if inside) to move
   */
  void set_range(double tp, const bool force_current);

  /**
   * Determine if any of the remaining TaskPoints have an adjustable target
   *
   * @return True if adjustable targets
   */
  gcc_pure
  bool has_targets() const;

  /**
   * Save targets in case optimisation fails
   */
  void target_save();
  /**
   * Restore target from copy
   */
  void target_restore();

private:
  /* virtual methods from class TaskMacCready */
  double get_min_height(gcc_unused const AircraftState &aircraft) const override {
    return 0;
  }

  GlideResult SolvePoint(const TaskPoint &tp,
                         const AircraftState &aircraft,
                         double minH) const override;

  AircraftState get_aircraft_start(const AircraftState &aircraft) const override;
};

#endif
