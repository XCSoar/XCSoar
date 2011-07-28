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
#ifndef UNORDEREDTASK_H
#define UNORDEREDTASK_H

#include "AbstractTask.hpp"

/**
 *  Common class for all unordered task types
 */
class UnorderedTask: 
  public AbstractTask
{
public:
  /** 
   * Base constructor.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param gp Global glide polar used for navigation calculations
   * 
   */
  UnorderedTask(const enum type _type, TaskEvents &te,
                const TaskBehaviour &tb,
                const GlidePolar &gp);

  fixed get_finish_height() const;

  virtual GeoPoint get_task_center(const GeoPoint& fallback_location) const;
  virtual fixed get_task_radius(const GeoPoint& fallback_location) const;

  /**
   * Accept a (const) task point visitor; makes the visitor visit
   * all optional start points in the task
   *
   * @param visitor Visitor to accept
   * @param reverse Visit task points in reverse order
   */
  virtual void sp_CAccept(TaskPointConstVisitor& visitor, const bool reverse=false) const;

protected:

  bool check_task() const;

  bool calc_mc_best(const AircraftState &state_now, fixed& best) const;

  fixed calc_glide_required(const AircraftState &state_now) const;

  void glide_solution_remaining(const AircraftState &state_now, 
                                const GlidePolar &polar,
                                GlideResult &total,
                                GlideResult &leg);

  void glide_solution_travelled(const AircraftState &state_now, 
                                        GlideResult &total,
                                        GlideResult &leg);

  void glide_solution_planned(const AircraftState &state_now,
                                      GlideResult &total,
                                      GlideResult &leg,
                              DistanceStat &total_remaining_effective,
                              DistanceStat &leg_remaining_effective,
                                      const fixed total_t_elapsed,
                                      const fixed leg_t_elapsed);

  fixed scan_total_start_time(const AircraftState &state_now);

  fixed scan_leg_start_time(const AircraftState &state_now);

  fixed scan_distance_nominal();
  
  fixed scan_distance_planned();

  fixed scan_distance_remaining(const GeoPoint &ref);

  fixed scan_distance_scored(const GeoPoint &ref);

  fixed scan_distance_travelled(const GeoPoint &ref);

  void scan_distance_minmax(const GeoPoint &ref, 
                            bool full,
                            fixed *dmin, fixed *dmax);

  fixed calc_gradient(const AircraftState &state_now) const;

  bool has_targets() const { return false; }

  bool is_scored() const { return false; }

}; 

#endif
