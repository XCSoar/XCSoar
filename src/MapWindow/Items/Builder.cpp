// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "util/StaticArray.hxx"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "NMEA/Aircraft.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "NMEA/Info.hpp"
#include "Terrain/RasterTerrain.hpp"

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
  if (auto reach = route_planner.FindPositiveArrival(destination))
    list.append(new ArrivalAltitudeMapItem(elevation, *reach, safety_height));
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
  waypoints.VisitWithinRange(location, range, [&list=list](const auto &w){
    if (!list.full())
      list.append(new WaypointMapItem(w));
  });
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
