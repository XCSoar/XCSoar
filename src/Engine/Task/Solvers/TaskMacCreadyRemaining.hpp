// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

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
  TaskMacCreadyRemaining(TaskPoint &tp,
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
  [[gnu::pure]]
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
  double get_min_height([[maybe_unused]] const AircraftState &aircraft) const override {
    return 0;
  }

  GlideResult SolvePoint(const TaskPoint &tp,
                         const AircraftState &aircraft,
                         double minH) const override;

  AircraftState get_aircraft_start(const AircraftState &aircraft) const override;
};
