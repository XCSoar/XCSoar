/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
  typedef std::pair < Alternate, fixed > Divert;
  typedef std::vector < Divert > DivertVector; 

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
  AlternateTask(TaskEvents &te, 
                const TaskBehaviour &tb,
                GlidePolar &gp,
                const Waypoints &wps);

  void reset();

  /**
   * Sets the target of the task (if any) or fallback to aircraft location.  
   * Must be called before running update_sample!
   */
  void set_task_destination(const AIRCRAFT_STATE &state_now,
                            const TaskPoint* _target);

  /**
   * Retrieve the task alternate waypoints
   *
   * @param index Index sequence of alternate
   *
   * @return Waypoint pointer or null if invalid
   */
  const Waypoint* getAlternateWaypoint(const unsigned index) const;

protected:
  void clear();
  void client_update(const AIRCRAFT_STATE &state_now, const bool reachable);
  void check_alternate_changed();

private:
  AlternateVector alternates;
  GeoPoint destination;
  unsigned best_alternate_id;
};

#endif //ALTERNATETASK_HPP
