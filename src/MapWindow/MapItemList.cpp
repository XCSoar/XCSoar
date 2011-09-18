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

#include "MapItemList.hpp"
#include "MapItem.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"

static bool
CompareMapItems(const MapItem *a, const MapItem *b)
{
  if (a->type == MapItem::SELF)
    return true;

  if (b->type == MapItem::SELF)
    return false;

  if (a->type == MapItem::WAYPOINT &&
      ((const WaypointMapItem *)a)->waypoint.IsAirport())
    return true;

  if (b->type == MapItem::WAYPOINT &&
      ((const WaypointMapItem *)b)->waypoint.IsAirport())
    return false;

  if (a->type == MapItem::WAYPOINT &&
      ((const WaypointMapItem *)a)->waypoint.IsLandable())
    return true;

  if (b->type == MapItem::WAYPOINT &&
      ((const WaypointMapItem *)b)->waypoint.IsLandable())
    return false;

  if (a->type == MapItem::AIRSPACE && b->type == MapItem::AIRSPACE)
    return AirspaceAltitude::SortHighest(
        ((const AirspaceMapItem *)a)->airspace->GetBase(),
        ((const AirspaceMapItem *)b)->airspace->GetBase());

  return a->type < b->type;
}

MapItemList::~MapItemList()
{
  for (iterator it = begin(), it_end = end(); it != it_end; ++it)
    delete *it;
}

void
MapItemList::Sort()
{
  std::sort(begin(), end(), CompareMapItems);
}
