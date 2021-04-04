/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef TASKBESTMC_HPP
#define TASKBESTMC_HPP

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

#endif

