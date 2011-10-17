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

#include "MapItemListBuilder.hpp"

#include "Util/StaticArray.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspaceVisitor.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicateInside.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "NMEA/Aircraft.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "ProtectedMarkers.hpp"
#include "Markers.hpp"
#include "MapItem.hpp"
#include "MapItemList.hpp"

class AirspaceWarningList
{
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning;

public:
  void Add(const AirspaceWarning& as) {
    if (as.get_warning_state() == AirspaceWarning::WARNING_INSIDE)
      ids_inside.checked_append(&as.get_airspace());
    else if (as.get_warning_state() > AirspaceWarning::WARNING_CLEAR)
      ids_warning.checked_append(&as.get_airspace());
  }

  void Fill(const AirspaceWarningManager &awm) {
    for (AirspaceWarningManager::const_iterator i = awm.begin(),
           end = awm.end(); i != end; ++i)
      Add(*i);
  }

  void Fill(const ProtectedAirspaceWarningManager &awm) {
    const ProtectedAirspaceWarningManager::Lease lease(awm);
    Fill(lease);
  }

  bool ContainsWarning(const AbstractAirspace& as) const {
    return ids_warning.contains(&as);
  }

  bool ContainsInside(const AbstractAirspace& as) const {
    return ids_inside.contains(&as);
  }
};

class AirspaceWarningPredicate: public AirspacePredicate
{
  const AirspaceWarningList &warnings;

public:
  AirspaceWarningPredicate(const AirspaceWarningList &_warnings)
    :warnings(_warnings) {}

  bool condition(const AbstractAirspace& airspace) const {
    return warnings.ContainsInside(airspace) ||
           warnings.ContainsWarning(airspace);
  }
};

class AirspaceAtPointPredicate: public AirspacePredicate
{
  AirspaceVisiblePredicate visible_predicate;
  AirspaceWarningPredicate warning_predicate;
  AirspacePredicateInside inside_predicate;

public:
  AirspaceAtPointPredicate(const AirspaceComputerSettings &_computer_settings,
                           const AirspaceRendererSettings &_renderer_settings,
                           const AircraftState& _state,
                           const AirspaceWarningList &_warnings,
                           const GeoPoint _location)
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warning_predicate(_warnings),
     inside_predicate(_location) {}

  bool condition(const AbstractAirspace& airspace) const {
    // Airspace should be visible or have a warning/inside status
    // and airspace needs to be at specified location

    return (visible_predicate(airspace) || warning_predicate(airspace)) &&
           inside_predicate(airspace);
  }
};

/**
 * Class to display airspace details dialog
 */
class AirspaceListBuilderVisitor:
  public AirspaceVisitor
{
  MapItemList &list;

public:
  AirspaceListBuilderVisitor(MapItemList &_list):list(_list) {}

  void Visit(const AirspacePolygon &airspace) {
    list.checked_append(new AirspaceMapItem(&airspace));
  }

  void Visit(const AirspaceCircle &airspace) {
    list.checked_append(new AirspaceMapItem(&airspace));
  }
};

class WaypointListBuilderVisitor:
  public WaypointVisitor
{
  MapItemList &list;

public:
  WaypointListBuilderVisitor(MapItemList &_list):list(_list) {}

  void Visit(const Waypoint &waypoint) {
    list.checked_append(new WaypointMapItem(waypoint));
  }
};

void
MapItemListBuilder::AddSelfIfNear(const GeoPoint &self, const Angle &bearing,
                                  fixed range)
{
  if (location.Distance(self) < range)
    list.checked_append(new SelfMapItem(self, bearing));
}

void
MapItemListBuilder::AddWaypoints(const Waypoints &waypoints, fixed range)
{
  WaypointListBuilderVisitor waypoint_list_builder(list);
  waypoints.visit_within_range(location, range, waypoint_list_builder);
}

void
MapItemListBuilder::AddVisibleAirspace(
    const Airspaces &airspaces,
    const ProtectedAirspaceWarningManager *warning_manager,
    const AirspaceComputerSettings &computer_settings,
    const AirspaceRendererSettings &renderer_settings,
    const MoreData &basic, const DerivedInfo &calculated)
{
  AirspaceWarningList warnings;
  if (warning_manager != NULL)
    warnings.Fill(*warning_manager);

  AirspaceAtPointPredicate predicate(computer_settings, renderer_settings,
                                     ToAircraftState(basic, calculated),
                                     warnings, location);

  AirspaceListBuilderVisitor builder(list);
  airspaces.visit_within_range(location, fixed(100.0), builder, predicate);
}

void
MapItemListBuilder::AddTaskOZs(const ProtectedTaskManager &task)
{
  ProtectedTaskManager::Lease task_manager(task);
  if (task_manager->GetMode() != TaskManager::MODE_ORDERED)
    return;

  const OrderedTask &ordered_task = task_manager->GetOrderedTask();

  AircraftState a;
  a.location = location;

  for (unsigned i = 0, size = ordered_task.TaskSize(); i < size; i++) {
    const OrderedTaskPoint *task_point = ordered_task.GetTaskPoint(i);
    if (!task_point || !task_point->IsInSector(a))
      continue;

    const ObservationZonePoint *oz = task_point->get_oz();
    if (oz)
      list.checked_append(new TaskOZMapItem(i, *oz, task_point->GetType(),
                                            task_point->GetWaypoint()));
  }
}

void
MapItemListBuilder::AddMarkers(const ProtectedMarkers &marks, fixed range)
{
  ProtectedMarkers::Lease lease(marks);
  unsigned i = 0;
  for (Markers::const_iterator it = lease->begin(), it_end = lease->end();
       it != it_end; ++it) {
    if (location.Distance(it->location) < range)
      list.checked_append(new MarkerMapItem(i, *it));

    i++;
  }
}
