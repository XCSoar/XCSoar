// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

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
  template<typename T>
  TaskEffectiveMacCready(T &tps,
                         const unsigned activeTaskPoint,
                         const AircraftState &_aircraft,
                         const GlideSettings &settings,
                         const GlidePolar &gp) noexcept
    :TaskSolveTravelled(tps, activeTaskPoint, _aircraft,
                        settings, gp, 0.001, 10.0)
  {
  }

protected:
  /* virtual methods from class ZeroFinder */
  double f(double x) noexcept override;
};
