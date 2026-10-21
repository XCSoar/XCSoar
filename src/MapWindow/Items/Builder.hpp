// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <span>

class MapItemList;
class Angle;
class Airspaces;
class ProtectedAirspaceWarningManager;
struct AirspaceComputerSettings;
struct AirspaceRendererSettings;
class Waypoints;
struct MoreData;
struct DerivedInfo;
class ProtectedTaskManager;
struct TrafficList;
struct NMEAInfo;
class RasterTerrain;
class ProtectedRoutePlanner;
struct ComputerSettings;
class NOAAStore;
class ObservationDatabase;

class MapItemListBuilder
{
  MapItemList &list;
  GeoPoint location;
  double range;

public:
  MapItemListBuilder(MapItemList &_list, GeoPoint _location, double _range)
    :list(_list), location(_location), range(_range) {}

  void AddLocation(const NMEAInfo &basic, const RasterTerrain *terrain);
  void AddArrivalAltitudes(const ProtectedRoutePlanner &route_planner,
                     const RasterTerrain *terrain, double safety_height);
  void AddSelfIfNear(const GeoPoint &self, Angle bearing);
  void AddWaypoints(const Waypoints &waypoints,
                    const ProtectedRoutePlanner *route_planner,
                    const MoreData &basic, const DerivedInfo &calculated,
                    const ComputerSettings &settings);
  void AddVisibleAirspace(const Airspaces &airspaces,
                          const ProtectedAirspaceWarningManager *warning_manager,
                          const AirspaceComputerSettings &computer_settings,
                          const AirspaceRendererSettings &renderer_settings,
                          const MoreData &basic, const DerivedInfo &calculated);
  void AddTaskOZs(const ProtectedTaskManager &task);
  void AddTraffic(const TrafficList &flarm);
  void AddThermals(const ObservationDatabase &observations,
                   const MoreData &basic, const DerivedInfo &calculated,
                   const ComputerSettings &settings);

  void AddWeatherStations(NOAAStore &store);
};
