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

#include "WaypointLabelList.hpp"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"

#include <algorithm>

static constexpr int WPCIRCLESIZE = 2;

gcc_pure
static bool
MapWaypointLabelListCompare(const WaypointLabelList::Label &e1,
                            const WaypointLabelList::Label &e2)
{
  if (e1.inTask && !e2.inTask)
    return true;

  if (!e1.inTask && e2.inTask)
    return false;

  if (e1.isAirport && !e2.isAirport)
    return true;

  if (!e1.isAirport && e2.isAirport)
    return false;

  if (e1.isLandable && !e2.isLandable)
    return true;

  if (!e1.isLandable && e2.isLandable)
    return false;

  if (e1.isWatchedWaypoint && !e2.isWatchedWaypoint)
    return true;

  if (!e1.isWatchedWaypoint && e2.isWatchedWaypoint)
    return false;

  if (e1.AltArivalAGL > e2.AltArivalAGL)
    return true;

  if (e1.AltArivalAGL < e2.AltArivalAGL)
    return false;

  return false;
}

void
WaypointLabelList::Add(const TCHAR *Name, int X, int Y,
                       TextInBoxMode Mode, bool bold,
                       int AltArivalAGL, bool inTask,
                       bool isLandable, bool isAirport, bool isWatchedWaypoint)
{
  if (X < - WPCIRCLESIZE || X > (int)width + WPCIRCLESIZE * 3 ||
      Y < - WPCIRCLESIZE || Y > (int)height + WPCIRCLESIZE)
    return;

  if (labels.full())
    return;

  auto &l = labels.append();

  CopyString(l.Name, Name, ARRAY_SIZE(l.Name));
  l.Pos.x = X;
  l.Pos.y = Y;
  l.Mode = Mode;
  l.AltArivalAGL = AltArivalAGL;
  l.bold = bold;
  l.inTask = inTask;
  l.isLandable = isLandable;
  l.isAirport  = isAirport;
  l.isWatchedWaypoint = isWatchedWaypoint;
}

void
WaypointLabelList::Sort()
{
  std::sort(labels.begin(), labels.end(),
            MapWaypointLabelListCompare);
}
