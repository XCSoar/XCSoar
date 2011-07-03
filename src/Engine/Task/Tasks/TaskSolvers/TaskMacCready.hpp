/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Navigation/Aircraft.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"
#include <vector>

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
class TaskMacCready:
  private NonCopyable 
{
public:
/** 
 * Constructor for ordered task points
 * 
 * @param _tps Vector of ordered task points comprising the task
 * @param _activeTaskPoint Current active task point in sequence
 * @param gp Glide polar to copy for calculations
 */
  TaskMacCready(const std::vector<OrderedTaskPoint*> &_tps,
                const unsigned _activeTaskPoint,
                const GlidePolar &gp);

/** 
 * Constructor for single task points (non-ordered ones)
 * 
 * @param tp Task point comprising the task
 * @param gp Glide polar to copy for calculations
 */
  TaskMacCready(TaskPoint* tp,
                const GlidePolar &gp);

/**
 * Constructor for sequence of task points, starting from first point
 *
 * @param _tps Vector of ordered task points comprising the task
 * @param gp Glide polar to copy for calculations
 */
  TaskMacCready(const std::vector<TaskPoint*> &_tps,
                const GlidePolar &gp);

  virtual ~TaskMacCready() {};

/** 
 * Calculate glide solution
 * 
 * @param aircraft Aircraft state
 * 
 * @return Glide result for entire task
 */
  GlideResult glide_solution(const AIRCRAFT_STATE &aircraft);

/** 
 * Calculate glide solution for externally specified aircraft sink rate
 * 
 * @param aircraft Aircraft state
 * @param S Sink rate (m/s, positive down)
 * 
 * @return Glide result for entire task with virtual sink rate
 */
  GlideResult glide_sink(const AIRCRAFT_STATE &aircraft,
                         const fixed S);

/** 
 * Adjust MacCready value of internal glide polar
 * 
 * @param mc MacCready value (m/s)
 */
  void set_mc(fixed mc) {
    m_glide_polar.SetMC(mc);
  };

/** 
 * Adjust cruise efficiency of internal glide polar
 * 
 * @param ce Cruise efficiency
 */
  void set_cruise_efficiency(fixed ce) {
    m_glide_polar.SetCruiseEfficiency(ce);
  };

/** 
 * Return glide solution for current leg.
 * This method is provided since glide_solution() and
 * glide_sink() both return the solutions for the entire task.
 * 
 * @return Glide solution of current leg
 */
  const GlideResult& get_active_solution(const AIRCRAFT_STATE &aircraft);

protected:

/** 
 * Calculate glide solution for specified index, given
 * aircraft state and virtual sink rate.
 * 
 * @param index Index of task point
 * @param state Aircraft state at origin
 * @param S Sink rate (m/s, positive down)
 * 
 * @return Glide result for segment
 */
  GlideResult tp_sink(const unsigned index,
                       const AIRCRAFT_STATE &state, 
                       const fixed S) const;

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
  virtual fixed get_min_height(const AIRCRAFT_STATE &state) const = 0;

/** 
 * Pure virtual method to calculate glide solution for specified index, given
 * aircraft state and height constraint.
 * This is used to provide alternate methods for different perspectives
 * on the task, e.g. planned/remaining/travelled
 * 
 * @param index Index of task point
 * @param state Aircraft state at origin
 * @param minH Minimum height at destination
 * 
 * @return Glide result for segment
 */
  virtual GlideResult tp_solution(const unsigned index,
                                  const AIRCRAFT_STATE& state, 
                                  fixed minH) const = 0;

/** 
 * Pure virtual method to obtain aircraft state at start of task.
 * This is used to provide alternate methods for different perspectives
 * on the task, e.g. planned/remaining/travelled
 * 
 * @param state Actual aircraft state
 * 
 * @return Aircraft state at start of task
 */
  virtual const AIRCRAFT_STATE 
  get_aircraft_start(const AIRCRAFT_STATE &state) const = 0;

/** 
 * Calculate clearance heights for all turnpoints, given the
 * glide polar.  This is the absolute minimum height the aircraft
 * can be at for solutions along the task.
 * 
 * @param state Aircraft state
 */
  void clearance_heights(const AIRCRAFT_STATE &state);

protected:
  const std::vector<TaskPoint*> m_tps; /**< The TaskPoints in the task */
  std::vector<GlideResult> m_gs; /**< Glide solutions for each leg */
  std::vector<fixed> m_minHs; /**< Minimum altitude for each taskpoint (m) */
  const unsigned m_activeTaskPoint; /**< Active task point (local copy for speed) */
  int m_start; /**< TaskPoint sequence index of first taskpoint included in scan */
  int m_end; /**< TaskPoint sequence index of last taskpoint included in scan */
  GlidePolar m_glide_polar; /**< Glide polar used for computations */
};

#endif
