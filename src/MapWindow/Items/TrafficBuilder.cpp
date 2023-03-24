// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "FLARM/List.hpp"
#include "FLARM/Friends.hpp"
#include "Tracking/SkyLines/Data.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "Components.hpp"

void
MapItemListBuilder::AddTraffic(const TrafficList &flarm)
{
  for (const auto &t : flarm.list) {
    if (list.full())
      break;

    if (location.DistanceS(t.location) < range) {
      auto color = FlarmFriends::GetFriendColor(t.id);
      list.append(new TrafficMapItem(t.id, color));
    }
  }
}

void
MapItemListBuilder::AddSkyLinesTraffic()
{
#ifdef HAVE_SKYLINES_TRACKING
  const auto &data = tracking->GetSkyLinesData();
  const std::lock_guard lock{data.mutex};

  StaticString<32> buffer;

  for (const auto &i : data.traffic) {
    if (list.full())
      break;

    if (i.second.location.IsValid() &&
        location.DistanceS(i.second.location) < range) {
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

      list.append(new SkyLinesTrafficMapItem(id, i.second.time_of_day,
                                             i.second.altitude,
                                             name));
    }
  }
#endif
}
