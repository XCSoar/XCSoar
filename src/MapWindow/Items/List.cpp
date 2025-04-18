// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  if (a->type != b->type)
    return a->type < b->type;

  switch (a->type) {
  case MapItem::Type::LOCATION:
  case MapItem::Type::ARRIVAL_ALTITUDE:
  case MapItem::Type::SELF:
    break;

  case MapItem::Type::WAYPOINT:
    return CompareWaypointItems((const WaypointMapItem *)a,
                                (const WaypointMapItem *)b);

  case MapItem::Type::TASK_OZ:
    return ((const TaskOZMapItem *)a)->index <
           ((const TaskOZMapItem *)b)->index;

  case MapItem::Type::TRAFFIC:
    return ((const TrafficMapItem *)a)->id <
           ((const TrafficMapItem *)b)->id;

  case MapItem::Type::THERMAL:
    return ((const ThermalMapItem *)a)->thermal.time >
           ((const ThermalMapItem *)b)->thermal.time;

  case MapItem::Type::AIRSPACE:
    return AirspaceAltitude::SortHighest(
        ((const AirspaceMapItem *)a)->airspace->GetBase(),
        ((const AirspaceMapItem *)b)->airspace->GetBase());

#ifdef HAVE_NOAA
  case MapItem::Type::WEATHER:
    return strcmp(((const WeatherStationMapItem *)a)->station->code,
                  ((const WeatherStationMapItem *)b)->station->code) < 0;
#endif

  case MapItem::Type::SKYLINES_TRAFFIC:
  case MapItem::Type::OVERLAY:
  case MapItem::Type::RASP:
    break;
  }

  return false;
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
