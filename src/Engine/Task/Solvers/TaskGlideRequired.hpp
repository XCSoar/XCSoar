// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "TaskMacCreadyRemaining.hpp"
#include "Math/ZeroFinder.hpp"

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
  static constexpr double TOLERANCE = 0.001;

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
  template<typename T>
  TaskGlideRequired(T &tps,
                    const unsigned activeTaskPoint,
                    const AircraftState &_aircraft,
                    const GlideSettings &settings,
                    const GlidePolar &_gp) noexcept
    :ZeroFinder(-10, 10, TOLERANCE),
     tm(tps.begin(), tps.end(), activeTaskPoint, settings, _gp),
     aircraft(_aircraft)
  {
    // Vopt at mc=0
    tm.set_mc(0);
  }

  /**
   * Constructor for single task points (non-ordered ones)
   *
   * @param tp Task point comprising the task
   * @param _aircraft Current aircraft state
   * @param gp Glide polar to copy for calculations
   */
  TaskGlideRequired(TaskPoint &tp,
                    const AircraftState &_aircraft,
                    const GlideSettings &settings, const GlidePolar &gp);

  /**
   * Search for sink rate to produce final glide solution
   *
   * @param s Default sink rate value (m/s)
   *
   * @return Solution sink rate (m/s, down positive)
   */
  [[gnu::pure]]
  double search(double s);

private:
  /* virtual methods from class ZeroFinder */
  double f(double mc) noexcept override;
};
