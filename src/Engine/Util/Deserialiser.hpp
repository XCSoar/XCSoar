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
#ifndef DESERIALISER_HPP
#define DESERIALISER_HPP

#include "Task/Tasks/OrderedTask.hpp"

class DataNode;
struct Waypoint;
class Waypoints;

/**
 * Class to serialise and de-serialise tasks to/from a #DataNode structure
 */
class Deserialiser
{
  DataNode &m_node;

  const Waypoints *waypoints;

public:
  /** 
   * Constructor
   * 
   * @param the_node Node this Deserialiser will edit
   * @param _waypoints a waypoint database to merge with (optional)
   * 
   * @return Initialised object
   */
  Deserialiser(DataNode& the_node, const Waypoints *_waypoints=NULL)
    :m_node(the_node), waypoints(_waypoints) {}

  /** 
   * De-serialise a task (create a task to reflect the DataNode structure)
   * 
   * @param data OrderedTask to serialise
   */
  void deserialise(OrderedTask &task);

protected:
  /** 
   * Deserialise OrderedTaskBehaviour
   * 
   * @param data Item to deserialise
   */
  void deserialise(OrderedTaskBehaviour& data);

  /** 
   * Deserialise a Waypoint; client responsible for deletion
   * 
   * @return Newly constructed Waypoint or NULL on failure
   */
  Waypoint* deserialise_waypoint();

  /** 
   * Deserialise a GeoPoint
   * 
   * @param data Item to deserialise
   */
  void deserialise(GeoPoint& data);

  /** 
   * Deserialise an ObservationZonePoint; client responsible for deletion
   * 
   * @param wp Waypoint base of point
   * @param is_turnpoint Whether the point is a turnpoint
   *
   * @return Newly constructed ObservationZonePoint or NULL on failure
   */
  ObservationZonePoint* deserialise_oz(const Waypoint& wp, const bool is_turnpoint);

  /** 
   * Deserialise a point, appending it to the task
   * 
   * @param data OrderedTask to append to
   */
  void deserialise_point(OrderedTask& data);

private:
  HeightReferenceType height_ref(const TCHAR *nodename) const;
  TaskBehaviour::FactoryType task_factory_type() const;
};

#endif
