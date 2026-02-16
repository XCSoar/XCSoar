// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointLabelList.hpp"
#include "util/StringUtil.hpp"
#include "util/Macros.hpp"

#include <algorithm>

[[gnu::pure]]
static bool
MapWaypointLabelListCompare(const WaypointLabelList::Label &e1,
                            const WaypointLabelList::Label &e2) noexcept
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
WaypointLabelList::Add(const char *Name, PixelPoint p,
                       TextInBoxMode Mode, bool bold,
                       int AltArivalAGL, bool inTask,
                       bool isLandable, bool isAirport,
                       bool isWatchedWaypoint) noexcept
{
  if (!clip_rect.Contains(p))
    return;

  if (labels.full())
    return;

  auto &l = labels.append();

  CopyString(l.Name, ARRAY_SIZE(l.Name), Name);
  l.Pos = p;
  l.Mode = Mode;
  l.AltArivalAGL = AltArivalAGL;
  l.bold = bold;
  l.inTask = inTask;
  l.isLandable = isLandable;
  l.isAirport  = isAirport;
  l.isWatchedWaypoint = isWatchedWaypoint;
}

void
WaypointLabelList::Sort() noexcept
{
  std::sort(labels.begin(), labels.end(),
            MapWaypointLabelListCompare);
}
