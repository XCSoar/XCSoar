// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "TaskMacCreadyTravelled.hpp"
#include "Math/ZeroFinder.hpp"
#include "time/Cast.hxx"

/**
 *  Abstract class to solve for travelled time.
 */
class TaskSolveTravelled : protected ZeroFinder {
  static constexpr double TOLERANCE_CRUISE_EFFICIENCY = 0.001;

  const AircraftState &aircraft;
  double inv_dt;
  FloatDuration dt;

protected:
  TaskMacCreadyTravelled tm; /**< Travelled calculator */

public:
  /**
   * Constructor for ordered task points
   *
   * @param tps Vector of ordered task points comprising the task
   * @param activeTaskPoint Current active task point in sequence
   * @param _aircraft Current aircraft state
   * @param gp Glide polar to copy for calculations
   * @param xmin Min value of search parameter
   * @param xmax Max value of search parameter
   */
  template<typename T, typename A>
  TaskSolveTravelled(T &tps,
                     unsigned activeTaskPoint,
                     const A &_aircraft,
                     const GlideSettings &settings, const GlidePolar &gp,
                     double _xmin, double _xmax) noexcept
    :ZeroFinder(_xmin, _xmax, TOLERANCE_CRUISE_EFFICIENCY),
     aircraft(_aircraft),
     tm(tps.begin(), activeTaskPoint, settings, gp)
  {
    dt = _aircraft.time - tps.begin()->GetEnteredState().time;
    if (dt.count() > 0) {
      inv_dt = 1. / ToFloatSeconds(dt);
    } else {
      inv_dt = 0; // error!
    }
  }

protected:
  /**
   * Calls travelled calculator
   *
   * @return Time error
   */
  double time_error();

public:
  /**
   * Search for parameter value.
   *
   * @param ce Default parameter value
   *
   * @return Value producing same travelled time
   */
  double search(double ce);
};
