// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

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
  [[gnu::pure]]
  double effective_distance(FloatDuration time_remaining) const noexcept;

  /**
   * Calculate effective distance remaining such that at the virtual
   * point, the time remaining is the same as the reference time
   * remaining (for active leg)
   *
   * @param time_remaining Time remaining (s) for active leg
   *
   * @return Effective distance remaining (m) for active leg
   */
  [[gnu::pure]]
  double effective_leg_distance(FloatDuration time_remaining) const noexcept;

private:
  /* virtual methods from class TaskMacCready */
  double get_min_height([[maybe_unused]] const AircraftState &aircraft) const override {
    return double(0);
  }

  GlideResult SolvePoint(const TaskPoint &tp,
                         const AircraftState &aircraft,
                         double minH) const override;

  AircraftState get_aircraft_start(const AircraftState &aircraft) const override;
};
