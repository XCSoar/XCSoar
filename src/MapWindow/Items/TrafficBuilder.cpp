// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "FLARM/List.hpp"
#include "FLARM/Friends.hpp"

void
MapItemListBuilder::AddTraffic(const TrafficList &flarm)
{
  for (const auto &t : flarm.list) {
    if (list.full())
      break;

    if (!t.location_available || !t.location.IsValid())
      continue;

    if (location.DistanceS(t.location) < range) {
      auto color = FlarmFriends::GetFriendColor(t.id);
      list.append(new TrafficMapItem(t.id, color));
    }
  }
}
