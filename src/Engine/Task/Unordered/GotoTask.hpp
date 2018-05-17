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

#ifndef GOTOTASK_H
#define GOTOTASK_H

#include "UnorderedTask.hpp"
#include "Engine/Waypoint/Ptr.hpp"

class Waypoints;
class UnorderedTaskPoint;

/**
 * Class providing ability to go to a single task point
 */
class GotoTask final : public UnorderedTask
{
  UnorderedTaskPoint *tp;
  const Waypoints &waypoints;

public:
  /** 
   * Base constructor.
   * 
   * @param tb Global task behaviour settings
   * @param wps Waypoints container to be scanned for takeoff
   * 
   * @return Initialised object (with no waypoint to go to)
   */

  GotoTask(const TaskBehaviour &tb,
           const Waypoints &wps);
  ~GotoTask();

  void SetTaskBehaviour(const TaskBehaviour &tb);

/** 
 * Sets go to task point to specified waypoint. 
 * Obeys TaskBehaviour.goto_nonlandable, won't do anything
 * if destination is not landable.
 * 
 * @param wp Waypoint to Go To
 * @return True if successful
 */
  bool DoGoto(WaypointPtr &&wp);

  /**
   * When called on takeoff, creates a default goto task
   *
   * @param loc Location of takeoff point
   * @param terrain_alt Terrain height at takeoff point
   *
   * @return True if default task was created
   */
  bool TakeoffAutotask(const GeoPoint &loc, double terrain_alt);

public:
  /* virtual methods from class TaskInterface */
  virtual unsigned TaskSize() const override;
  virtual TaskWaypoint *GetActiveTaskPoint() const override;
  virtual void SetActiveTaskPoint(unsigned index) override;
  virtual bool IsValidTaskPoint(const int index_offset) const override;

protected:
  virtual bool UpdateSample(const AircraftState &state_now,
                            const GlidePolar &glide_polar,
                            const bool full_update) override;
public:
  virtual void AcceptTaskPointVisitor(TaskPointConstVisitor& visitor) const override;
};

#endif //GOTOTASK_H
