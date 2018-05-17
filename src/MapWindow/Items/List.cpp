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

#include "List.hpp"
#include "MapItem.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Weather/Features.hpp"

static bool
CompareWaypointItems(const WaypointMapItem *a, const WaypointMapItem *b)
{
  enum {
    AIRPORT, LANDABLE, WAYPOINT,
  } type1, type2;

  if (a->waypoint->IsAirport())
    type1 = AIRPORT;
  else if (a->waypoint->IsLandable())
    type1 = LANDABLE;
  else
    type1 = WAYPOINT;

  if (b->waypoint->IsAirport())
    type2 = AIRPORT;
  else if (b->waypoint->IsLandable())
    type2 = LANDABLE;
  else
    type2 = WAYPOINT;

  if (type1 != type2)
    return type1 < type2;

  return a->waypoint->id < b->waypoint->id;
}

static bool
CompareMapItems(const MapItem *a, const MapItem *b)
{
  if (a->type == MapItem::LOCATION)
    return true;

  if (b->type == MapItem::LOCATION)
    return false;

  if (a->type == MapItem::ARRIVAL_ALTITUDE)
    return true;

  if (b->type == MapItem::ARRIVAL_ALTITUDE)
    return false;

  if (a->type == MapItem::SELF)
    return true;

  if (b->type == MapItem::SELF)
    return false;

  if (a->type == MapItem::WAYPOINT && b->type != MapItem::WAYPOINT &&
      ((const WaypointMapItem *)a)->waypoint->IsAirport())
    return true;

  if (a->type != MapItem::WAYPOINT && b->type == MapItem::WAYPOINT &&
      ((const WaypointMapItem *)b)->waypoint->IsAirport())
    return false;

  if (a->type == MapItem::WAYPOINT && b->type != MapItem::WAYPOINT &&
      ((const WaypointMapItem *)a)->waypoint->IsLandable())
    return true;

  if (a->type != MapItem::WAYPOINT && b->type == MapItem::WAYPOINT &&
      ((const WaypointMapItem *)b)->waypoint->IsLandable())
    return false;

  if (a->type == MapItem::WAYPOINT && b->type == MapItem::WAYPOINT)
    return CompareWaypointItems((const WaypointMapItem *)a,
                                (const WaypointMapItem *)b);

  if (a->type == MapItem::TASK_OZ && b->type == MapItem::TASK_OZ)
    return ((const TaskOZMapItem *)a)->index <
           ((const TaskOZMapItem *)b)->index;

  if (a->type == MapItem::TRAFFIC && b->type == MapItem::TRAFFIC)
    return ((const TrafficMapItem *)a)->id <
           ((const TrafficMapItem *)b)->id;

  if (a->type == MapItem::THERMAL && b->type == MapItem::THERMAL)
    return ((const ThermalMapItem *)a)->thermal.time >
           ((const ThermalMapItem *)b)->thermal.time;

  if (a->type == MapItem::AIRSPACE && b->type == MapItem::AIRSPACE)
    return AirspaceAltitude::SortHighest(
        ((const AirspaceMapItem *)a)->airspace->GetBase(),
        ((const AirspaceMapItem *)b)->airspace->GetBase());

#ifdef HAVE_NOAA
  if (a->type == MapItem::WEATHER && b->type == MapItem::WEATHER)
    return strcmp(((const WeatherStationMapItem *)a)->station->code,
                  ((const WeatherStationMapItem *)b)->station->code) < 0;
#endif

  return a->type < b->type;
}

MapItemList::~MapItemList()
{
  for (auto it = begin(), it_end = end(); it != it_end; ++it)
    delete *it;
}

void
MapItemList::Sort()
{
  std::sort(begin(), end(), CompareMapItems);
}
