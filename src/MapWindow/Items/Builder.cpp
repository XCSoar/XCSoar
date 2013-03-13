/*
Copyright_License {

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

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "Util/StaticArray.hpp"
#include "Engine/Airspace/AirspaceVisitor.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "NMEA/Aircraft.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "Markers/ProtectedMarkers.hpp"
#include "Markers/Markers.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "FLARM/Friends.hpp"
#include "TeamCodeSettings.hpp"
#include "Tracking/SkyLines/Data.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "Components.hpp"

#ifdef HAVE_NOAA
#include "Weather/NOAAStore.hpp"
#endif

class AirspaceWarningList
{
  StaticArray<const AbstractAirspace *,64> list;

public:
  void Add(const AirspaceWarning& as) {
    if (as.GetWarningState() > AirspaceWarning::WARNING_CLEAR)
      list.checked_append(&as.GetAirspace());
  }

  void Fill(const AirspaceWarningManager &awm) {
    for (const AirspaceWarning &as : awm)
      Add(as);
  }

  void Fill(const ProtectedAirspaceWarningManager &awm) {
    const ProtectedAirspaceWarningManager::Lease lease(awm);
    Fill(lease);
  }

  bool Contains(const AbstractAirspace& as) const {
    return list.contains(&as);
  }
};

class AirspaceAtPointPredicate: public AirspacePredicate
{
  const AirspaceVisibility visible_predicate;
  const AirspaceWarningList &warnings;
  const GeoPoint location;

public:
  AirspaceAtPointPredicate(const AirspaceComputerSettings &_computer_settings,
                           const AirspaceRendererSettings &_renderer_settings,
                           const AircraftState& _state,
                           const AirspaceWarningList &_warnings,
                           const GeoPoint _location)
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warnings(_warnings),
     location(_location) {}

  bool operator()(const AbstractAirspace& airspace) const {
    // Airspace should be visible or have a warning/inside status
    // and airspace needs to be at specified location

    return (visible_predicate(airspace) || warnings.Contains(airspace)) &&
      airspace.Inside(location);
  }
};

/**
 * Class to display airspace details dialog
 */
class AirspaceListBuilderVisitor final : public AirspaceVisitor
{
  MapItemList &list;

public:
  AirspaceListBuilderVisitor(MapItemList &_list):list(_list) {}

  virtual void Visit(const AbstractAirspace &airspace) override {
    if (!list.full())
      list.append(new AirspaceMapItem(airspace));
  }
};

class WaypointListBuilderVisitor:
  public WaypointVisitor
{
  MapItemList &list;

public:
  WaypointListBuilderVisitor(MapItemList &_list):list(_list) {}

  void Visit(const Waypoint &waypoint) {
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

  short elevation;
  if (terrain != NULL)
    elevation = terrain->GetTerrainHeight(location);
  else
    elevation = RasterBuffer::TERRAIN_INVALID;

  list.append(new LocationMapItem(vector, elevation));
}

void
MapItemListBuilder::AddArrivalAltitudes(
    const ProtectedRoutePlanner &route_planner,
    const RasterTerrain *terrain, fixed safety_height)
{
  if (list.full())
    return;

  // Calculate terrain elevation if possible
  short elevation;
  if (terrain != NULL)
    elevation = terrain->GetTerrainHeight(location);
  else
    elevation = RasterBuffer::TERRAIN_INVALID;

  // Calculate target altitude
  RoughAltitude safety_elevation(safety_height);
  if (!RasterBuffer::IsInvalid(elevation))
    safety_elevation += RoughAltitude(elevation);

  // Save destination point incl. elevation and safety height
  const AGeoPoint destination(location, safety_elevation);

  // Calculate arrival altitudes
  ReachResult reach;

  ProtectedRoutePlanner::Lease leased_route_planner(route_planner);
  if (!leased_route_planner->FindPositiveArrival(destination, reach))
    return;

  reach.Subtract(RoughAltitude(safety_height));

  list.append(new ArrivalAltitudeMapItem(RoughAltitude(elevation), reach));
}

void
MapItemListBuilder::AddSelfIfNear(const GeoPoint &self, Angle bearing)
{
  if (!list.full() && location.Distance(self) < range)
    list.append(new SelfMapItem(self, bearing));
}

void
MapItemListBuilder::AddWaypoints(const Waypoints &waypoints)
{
  WaypointListBuilderVisitor waypoint_list_builder(list);
  waypoints.VisitWithinRange(location, range, waypoint_list_builder);
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
  airspaces.VisitWithinRange(location, fixed(100.0), builder, predicate);
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
                                  task_point.GetWaypoint()));
  }
}

void
MapItemListBuilder::AddMarkers(const ProtectedMarkers &marks)
{
  ProtectedMarkers::Lease lease(marks);
  unsigned i = 0;

  for (const auto &m : (const Markers &)lease) {
    if (list.full())
      break;

    if (location.Distance(m.location) < range)
      list.append(new MarkerMapItem(i, m));

    i++;
  }
}

#ifdef HAVE_NOAA
void
MapItemListBuilder::AddWeatherStations(NOAAStore &store)
{
  for (auto it = store.begin(), end = store.end(); it != end; ++it) {
    if (list.full())
      break;

    if (it->parsed_metar_available &&
        it->parsed_metar.location_available &&
        location.Distance(it->parsed_metar.location) < range)
      list.checked_append(new WeatherStationMapItem(it));
  }
}
#endif

void
MapItemListBuilder::AddTraffic(const TrafficList &flarm)
{
  for (const auto &t : flarm.list) {
    if (list.full())
      break;

    if (location.Distance(t.location) < range) {
      auto color = FlarmFriends::GetFriendColor(t.id);
      list.append(new TrafficMapItem(t.id, color));
    }
  }
}

void
MapItemListBuilder::AddSkyLinesTraffic()
{
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  const auto &data = tracking->GetSkyLinesData();
  const ScopeLock protect(data.mutex);

  StaticString<32> buffer;

  for (const auto &i : data.traffic) {
    if (list.full())
      break;

    if (i.second.location.IsValid() &&
        location.Distance(i.second.location) < range) {
      const uint32_t id = i.first;
      auto name_i = data.user_names.find(id);
      const TCHAR *name;
      if (name_i == data.user_names.end()) {
        /* no name found */
        buffer.UnsafeFormat(_T("SkyLines %u"), (unsigned)id);
        name = buffer;
      } else
        /* we know the name */
        name = name_i->second.c_str();

      list.append(new SkyLinesTrafficMapItem(id, i.second.time_of_day_ms,
                                             name));
    }
  }
#endif
}

void
MapItemListBuilder::AddThermals(const ThermalLocatorInfo &thermals,
                                const MoreData &basic,
                                const DerivedInfo &calculated)
{
  for (const auto &t : thermals.sources) {
    if (list.full())
      break;

    // find height difference
    if (basic.nav_altitude < t.ground_height)
      continue;

    GeoPoint loc = calculated.wind_available
      ? t.CalculateAdjustedLocation(basic.nav_altitude, calculated.wind)
      : t.location;

    if (location.Distance(loc) < range)
      list.append(new ThermalMapItem(t));
  }
}
