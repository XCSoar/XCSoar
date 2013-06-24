/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Task/Factory/TaskFactoryType.hpp"
#include "Geo/AltitudeReference.hpp"

#include <tchar.h>

class DataNode;
struct GeoPoint;
struct Waypoint;
class Waypoints;
class OrderedTask;
class ObservationZonePoint;
struct OrderedTaskSettings;

/**
 * Class to serialise and de-serialise tasks to/from a #DataNode structure
 */
class Deserialiser
{
  DataNode &node;

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
  Deserialiser(DataNode &_node, const Waypoints *_waypoints=nullptr)
    :node(_node), waypoints(_waypoints) {}

  /** 
   * De-serialise a task (create a task to reflect the DataNode structure)
   * 
   * @param data OrderedTask to serialise
   */
  void Deserialise(OrderedTask &task);

protected:
  /** 
   * Deserialise #OrderedTaskSettings
   * 
   * @param data Item to deserialise
   */
  void Deserialise(OrderedTaskSettings &data);

  /** 
   * Deserialise a Waypoint; client responsible for deletion
   * 
   * @return Newly constructed Waypoint or nullptr on failure
   */
  Waypoint* DeserialiseWaypoint();

  /** 
   * Deserialise a GeoPoint
   * 
   * @param data Item to deserialise
   */
  void Deserialise(GeoPoint &data);

  /** 
   * Deserialise an ObservationZonePoint; client responsible for deletion
   * 
   * @param wp Waypoint base of point
   * @param is_turnpoint Whether the point is a turnpoint
   *
   * @return Newly constructed ObservationZonePoint or nullptr on failure
   */
  ObservationZonePoint* DeserialiseOZ(const Waypoint &wp, bool is_turnpoint);

  /** 
   * Deserialise a point, appending it to the task
   * 
   * @param data OrderedTask to append to
   */
  void DeserialiseTaskpoint(OrderedTask &data);

private:
  AltitudeReference GetHeightRef(const TCHAR *nodename) const;
  TaskFactoryType GetTaskFactoryType() const;
};

#endif
