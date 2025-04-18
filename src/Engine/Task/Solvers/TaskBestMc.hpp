// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskMacCreadyRemaining.hpp"
#include "Math/ZeroFinder.hpp"

/**
 * Class to solve for MacCready value, being the highest MC value to produce a
 * pure glide solution for the remainder of the task.
 *
 * \todo
 * - f() fails if Mc too low for wind, need to account for failed solution
 */
class TaskBestMc final : ZeroFinder
{
  static constexpr double TOLERANCE = 0.0001;

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
   * @param _gp Glide polar to copy for calculations
   * @param _mc_min Minimum legal value of MacCready (m/s) in search
   */
  template<typename T>
  TaskBestMc(T &tps,
             const unsigned activeTaskPoint,
             const AircraftState &_aircraft,
             const GlideSettings &settings, const GlidePolar &_gp,
             double _mc_min=0) noexcept
    :ZeroFinder(_mc_min, 10.0, TOLERANCE),
     tm(tps.begin(), tps.end(), activeTaskPoint, settings, _gp),
     aircraft(_aircraft)
  {
  }

  /**
   * Constructor for single task points (non-ordered ones)
   *
   * @param tp Task point comprising the task
   * @param _aircraft Current aircraft state
   * @param _gp Glide polar to copy for calculations
   */
  TaskBestMc(TaskPoint &tp,
             const AircraftState &_aircraft,
             const GlideSettings &settings, const GlidePolar &_gp);

  /**
   * Search for best MC.  If fails (MC=0 is below final glide), returns
   * default value.
   *
   * @param mc Default MacCready value (m/s)
   *
   * @return Best MC value found or default value if no solution
   */
  double search(double mc);

  bool search(double mc, double &result);

private:

  /**
   * Test validity of a solution given search parameter
   *
   * @param mc Search parameter (MacCready setting (m/s))
   *
   * @return True if solution is valid
   */
  [[gnu::pure]]
  bool valid(double mc) const;

  /* virtual methods from class ZeroFinder */
  virtual double f(double mc) noexcept override;
};
