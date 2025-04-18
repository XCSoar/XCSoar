// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "TaskMacCreadyRemaining.hpp"
#include "Math/ZeroFinder.hpp"

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
  static constexpr double TOLERANCE = 0.002;

  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AircraftState &aircraft;
  const FloatDuration t_remaining;
  StartPoint &tp_start;
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
  template<typename T>
  TaskMinTarget(T &tps,
                const unsigned activeTaskPoint,
                const AircraftState &_aircraft,
                const GlideSettings &settings, const GlidePolar &_gp,
                FloatDuration _t_remaining,
                StartPoint &_ts) noexcept
    :ZeroFinder(0, 1, TOLERANCE),
     tm(tps.begin(), tps.end(), activeTaskPoint, settings, _gp,
        /* ignore the travel to the start point */
        false),
     aircraft(_aircraft),
     t_remaining(_t_remaining),
     tp_start(_ts),
     force_current(false)
  {
  }

private:
  double f(double p) noexcept override;

  /**
   * Test validity of a solution given search parameter
   *
   * @param p Search parameter (target range parameter [0,1])
   *
   * @return True if solution is valid
   */
  bool valid(double p) const noexcept;

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
  double search(double p) noexcept;

private:
  void set_range(double p) noexcept;
};
