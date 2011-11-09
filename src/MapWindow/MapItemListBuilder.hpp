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

#ifndef XCSOAR_MAP_ITEM_LIST_BUILDER_HPP
#define XCSOAR_MAP_ITEM_LIST_BUILDER_HPP

#include "Engine/Navigation/GeoPoint.hpp"

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
class ProtectedMarkers;
struct FlarmState;
struct ThermalLocatorInfo;

class MapItemListBuilder
{
  MapItemList &list;
  GeoPoint location;
  fixed range;

public:
  MapItemListBuilder(MapItemList &_list, GeoPoint _location, fixed _range)
    :list(_list), location(_location), range(_range) {}

  void AddSelfIfNear(const GeoPoint &self, const Angle &bearing);
  void AddWaypoints(const Waypoints &waypoints);
  void AddVisibleAirspace(const Airspaces &airspaces,
                          const ProtectedAirspaceWarningManager *warning_manager,
                          const AirspaceComputerSettings &computer_settings,
                          const AirspaceRendererSettings &renderer_settings,
                          const MoreData &basic, const DerivedInfo &calculated);
  void AddTaskOZs(const ProtectedTaskManager &task);
  void AddMarkers(const ProtectedMarkers &marks);
  void AddTraffic(const FlarmState &flarm);
  void AddThermals(const ThermalLocatorInfo &thermals,
                   const MoreData &basic, const DerivedInfo &calculated);
};

#endif
