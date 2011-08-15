/*
  Copyright_License {

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


#ifndef TASKWAYPOINT_HPP
#define TASKWAYPOINT_HPP

#include "TaskPoint.hpp"
#include "Waypoint/Waypoint.hpp"

/** Task points that have a waypoint associated */
class TaskWaypoint :
  public TaskPoint
{
  friend class PrintHelper;

  /** local copy of waypoint */
  Waypoint m_waypoint;

public:
  /**
   * Constructor.  Location and elevation of waypoint is used
   * as the task point's reference values; a copy of the waypoint
   * is also stored to facilitate user-feedback.
   *
   * @param wp Waypoint to be used as task point origin
   * @param tb Task Behaviour defining options (esp safety heights)
   *
   * @return Initialised object
   */
  TaskWaypoint(enum type _type, const Waypoint & wp)
    :TaskPoint(_type, wp.location, wp.altitude),
     m_waypoint(wp) {}

  /**
   * Recall waypoint associated with this task point.
   * Can be used for user feedback (e.g. queries on details of active
   * task point)
   *
   * @return Copy of waypoint associated with this task point
   */
  gcc_pure
  const Waypoint& get_waypoint() const {
    return m_waypoint;
  }
};

#endif
