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

#ifndef ALTERNATETASK_HPP
#define ALTERNATETASK_HPP

#include "AbortTask.hpp"

/**
 * AlternateTask is a specialisation of AbortTask to add functionality
 * to find alternate landing points along the task.
 *
 * @todo: take user preferences of landing points into account.
 */
class AlternateTask : 
  public AbortTask 
{
public:
  struct Divert : public Alternate {
    fixed delta;

    Divert(const Waypoint &_waypoint, const GlideResult &_solution,
           fixed _delta)
      :Alternate(_waypoint, _solution), delta(_delta) {}
  };

  typedef std::vector<Divert> DivertVector;

  static const unsigned max_alternates; /// number of alternates

private:
  AlternateVector alternates;
  GeoPoint destination;
  unsigned best_alternate_id;

public:
  /** 
   * Base constructor.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param gp Global glide polar used for navigation calculations
   * @param wps Waypoints container to be scanned during updates
   * 
   * @return Initialised object (with nothing in task)
   */
  AlternateTask(TaskEvents &te, const TaskBehaviour &tb,
                const GlidePolar &gp, const Waypoints &wps);

  void reset();

  /**
   * Sets the target of the task (if any) or fallback to aircraft location.  
   * Must be called before running update_sample!
   */
  void set_task_destination(const AircraftState &state_now,
                            const TaskPoint* _target);

  /**
   * Sets the target of the task to the home waypoint or fallback to
   * aircraft location.
   * Must be called before running update_sample!
   */
  void set_task_destination_home(const AircraftState &state_now);

  /**
   * Retrieve a copy of the task alternates
   *
   * @param index Index sequence of alternate
   *
   * @return Vector of alternates
   */
  const AlternateVector &getAlternates() const {
    return alternates;
  }

protected:
  void clear();
  void client_update(const AircraftState &state_now, const bool reachable);
  void check_alternate_changed();

private:
  /**
   * Determine if the candidate waypoint is already in the
   * alternate list.
   */
  bool is_waypoint_in_alternates(const Waypoint& waypoint) const;
};

#endif //ALTERNATETASK_HPP
