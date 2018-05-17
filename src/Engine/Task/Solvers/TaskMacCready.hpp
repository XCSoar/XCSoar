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
#ifndef TASK_MACCREADY_HPP
#define TASK_MACCREADY_HPP

#include "Util/NonCopyable.hpp"
#include "Util/StaticArray.hxx"
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideResult.hpp"

#include <array>

struct AircraftState;
struct GlideSettings;
class TaskPoint;
class OrderedTaskPoint;

/**
 * Abstract class for calculation of glide solutions with respect
 * to ordered tasks.  This class handles high intermediate turnpoints
 * whereby the aircraft's glide angle after achieving a turnpoint at minimum
 * height may be higher than final glide beyond that.
 *
 * An assumption is made that final glide is made at the latest part of the
 * task, and climbs are made as early as possible.
 *
 * Therefore, this system performs a scan to calculate effective minimum
 * heights for each task point which may be higher than the actual task point's
 * true minimum height.
 *
 * This system also uses a local copy of the glide polar so it can be used
 * for algorithms that adjust glide polar parameters.
 *
 * The class is not intended to be used directly, but to be specialised.
 *
 */
class TaskMacCready : private NonCopyable
{
protected:
  static constexpr unsigned MAX_SIZE = 32;

   /**
    * The TaskPoints in the task.
    */
  StaticArray<TaskPoint *, MAX_SIZE> points;

  /**
   * Glide solutions for each leg.
   */
  std::array<GlideResult, MAX_SIZE> leg_solutions;

  /**
   * Active task point (local copy for speed).
   */
  const unsigned active_index;

  const GlideSettings &settings;

  /**
   * Glide polar used for computations.
   */
  GlidePolar glide_polar;

public:
  /**
   * Constructor for ordered task points
   *
   * @param _tps Vector of ordered task points comprising the task
   * @param _active_index Current active task point in sequence
   * @param gp Glide polar to copy for calculations
   */
  template<class I>
  TaskMacCready(const I tps_begin, const I tps_end,
                const unsigned _active_index,
                const GlideSettings &_settings, const GlidePolar &gp)
    :points(tps_begin, tps_end),
     active_index(_active_index),
     settings(_settings),
     glide_polar(gp) {}

  /**
   * Constructor for single task points (non-ordered ones)
   *
   * @param tp Task point comprising the task
   * @param gp Glide polar to copy for calculations
   */
  TaskMacCready(TaskPoint* tp,
                const GlideSettings &_settings, const GlidePolar &gp)
    :points(1, tp),
     active_index(0),
     settings(_settings),
     glide_polar(gp) {}

  /**
   * Calculate glide solution
   *
   * @param aircraft Aircraft state
   *
   * @return Glide result for entire task
   */
  GlideResult glide_solution(const AircraftState &aircraft);

  /**
   * Calculate glide solution for externally specified aircraft sink rate
   *
   * @param aircraft Aircraft state
   * @param S Sink rate (m/s, positive down)
   *
   * @return Glide result for entire task with virtual sink rate
   */
  gcc_pure
  GlideResult glide_sink(const AircraftState &aircraft, double S) const;

  /**
   * Adjust MacCready value of internal glide polar
   *
   * @param mc MacCready value (m/s)
   */
  void set_mc(double mc) {
    glide_polar.SetMC(mc);
  };

  /**
   * Adjust cruise efficiency of internal glide polar
   *
   * @param ce Cruise efficiency
   */
  void set_cruise_efficiency(double ce) {
    glide_polar.SetCruiseEfficiency(ce);
  };

  /**
   * Return glide solution for current leg.
   * This method is provided since glide_solution() and
   * glide_sink() both return the solutions for the entire task.
   *
   * @return Glide solution of current leg
   */
  gcc_pure
  const GlideResult &get_active_solution() const {
    return leg_solutions[active_index];
  }

private:

  /**
   * Pure virtual method to retrieve the absolute minimum height of
   * aircraft for entire task.
   * This is used to provide alternate methods for different perspectives
   * on the task, e.g. planned/remaining/travelled
   *
   * @param state Aircraft state
   *
   * @return Min height (m) of entire task
   */
  gcc_pure
  virtual double get_min_height(const AircraftState &state) const = 0;

  /**
   * Pure virtual method to calculate glide solution for specified point, given
   * aircraft state and height constraint.
   * This is used to provide alternate methods for different perspectives
   * on the task, e.g. planned/remaining/travelled
   *
   * @param state Aircraft state at origin
   * @param minH Minimum height at destination
   *
   * @return Glide result for segment
   */
  gcc_pure
  virtual GlideResult SolvePoint(const TaskPoint &tp,
                                 const AircraftState &state,
                                 double minH) const = 0;

  /**
   * Pure virtual method to obtain aircraft state at start of task.
   * This is used to provide alternate methods for different perspectives
   * on the task, e.g. planned/remaining/travelled
   *
   * @param state Actual aircraft state
   *
   * @return Aircraft state at start of task
   */
  gcc_pure
  virtual AircraftState get_aircraft_start(const AircraftState &state) const = 0;
};

#endif
