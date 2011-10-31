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

#ifndef XCSOAR_AIRSPACE_AT_POINT_DIALOG_HPP
#define XCSOAR_AIRSPACE_AT_POINT_DIALOG_HPP

class SingleWindow;
struct MapItem;
class MapItemList;
struct GeoVector;
struct AircraftLook;
struct AirspaceLook;
struct WaypointLook;
struct TaskLook;
struct MarkerLook;
struct SETTINGS_MAP;

void ShowMapItemDialog(const MapItem &item, SingleWindow &parent);

void ShowMapItemListDialog(SingleWindow &parent,
                           const GeoVector &_vector,
                           const MapItemList &_list, short _elevation,
                           const AircraftLook &_aircraft_look,
                           const AirspaceLook &_airspace_look,
                           const WaypointLook &_waypoint_look,
                           const TaskLook &_task_look,
                           const MarkerLook &_marker_look,
                           const SETTINGS_MAP &_settings);

#endif
