/*
Copyright_License {

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

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "Util/StaticArray.hxx"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "NMEA/Aircraft.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "NMEA/Info.hpp"
#include "Terrain/RasterTerrain.hpp"

class WaypointListBuilderVisitor:
  public WaypointVisitor
{
  MapItemList &list;

public:
  WaypointListBuilderVisitor(MapItemList &_list):list(_list) {}

  void Visit(const WaypointPtr &waypoint) override {
    if (!list.full())
      list.append(new WaypointMapItem(waypoint));
  }
};

void
MapItemListBuilder::AddLocation(const NMEAInfo &basic,
                                const RasterTerrain *terrain)
{
  if (list.full())
    return;

  GeoVector vector;
  if (basic.location_available)
    vector = basic.location.DistanceBearing(location);
  else
    vector.SetInvalid();

  double elevation = LocationMapItem::UNKNOWN_ELEVATION;
  if (terrain != nullptr)
    elevation = terrain->GetTerrainHeight(location)
      .ToDouble(LocationMapItem::UNKNOWN_ELEVATION);

  list.append(new LocationMapItem(vector, elevation));
}

void
MapItemListBuilder::AddArrivalAltitudes(
    const ProtectedRoutePlanner &route_planner,
    const RasterTerrain *terrain, double safety_height)
{
  if (list.full())
    return;

  // Calculate terrain elevation if possible
  double elevation = LocationMapItem::UNKNOWN_ELEVATION;
  if (terrain != nullptr)
    elevation = terrain->GetTerrainHeight(location)
      .ToDouble(LocationMapItem::UNKNOWN_ELEVATION);

  // Calculate target altitude
  double target_elevation = 0;
  if (elevation > ArrivalAltitudeMapItem::UNKNOWN_ELEVATION_THRESHOLD)
    target_elevation += elevation;

  // Save destination point incl. elevation and safety height
  const AGeoPoint destination(location, target_elevation);

  // Calculate arrival altitudes
  ReachResult reach;

  ProtectedRoutePlanner::Lease leased_route_planner(route_planner);
  if (!leased_route_planner->FindPositiveArrival(destination, reach))
    return;

  list.append(new ArrivalAltitudeMapItem(elevation, reach, safety_height));
}

void
MapItemListBuilder::AddSelfIfNear(const GeoPoint &self, Angle bearing)
{
  if (!list.full() && location.DistanceS(self) < range)
    list.append(new SelfMapItem(self, bearing));
}

void
MapItemListBuilder::AddWaypoints(const Waypoints &waypoints)
{
  WaypointListBuilderVisitor waypoint_list_builder(list);
  waypoints.VisitWithinRange(location, range, waypoint_list_builder);
}

void
MapItemListBuilder::AddTaskOZs(const ProtectedTaskManager &task)
{
  ProtectedTaskManager::Lease task_manager(task);
  if (task_manager->GetMode() != TaskType::ORDERED)
    return;

  const OrderedTask &ordered_task = task_manager->GetOrderedTask();

  AircraftState a;
  a.location = location;

  for (unsigned i = 0, size = ordered_task.TaskSize(); i < size; i++) {
    if (list.full())
      break;

    const OrderedTaskPoint &task_point = ordered_task.GetTaskPoint(i);
    if (!task_point.IsInSector(a))
      continue;

    const ObservationZonePoint &oz = task_point.GetObservationZone();
    list.append(new TaskOZMapItem(i, oz, task_point.GetType(),
                                  task_point.GetWaypointPtr()));
  }
}
