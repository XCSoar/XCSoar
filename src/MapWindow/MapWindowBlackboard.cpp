// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindowBlackboard.hpp"
#include "FLARM/Friends.hpp"

void
MapWindowBlackboard::ReadComputerSettings(const ComputerSettings &settings) noexcept
{
  computer_settings = settings;
}

void
MapWindowBlackboard::ReadMapSettings(const MapSettings &settings) noexcept
{
  settings_map = settings;
}

[[gnu::pure]]
static bool
IsFriend(FlarmId id) noexcept
{
  return FlarmFriends::GetFriendColor(id) != FlarmColor::NONE;
}

static void
UpdateFadingTraffic(bool fade_traffic,
                    std::map<FlarmId, FlarmTraffic> &dest,
                    const TrafficList &old_list, const TrafficList &new_list,
                    TimeStamp now) noexcept
{
  if (!fade_traffic) {
    dest.clear();
    return;
  }

  if (new_list.modified.Modified(old_list.modified)||true) {
    /* first add all items from the old list */
    for (const auto &traffic : old_list.list)
      if (traffic.location_available)
        dest.try_emplace(traffic.id, traffic);

    /* now remove all items that are in the new list; now only items
       remain that have disappeared */
    for (const auto &traffic : new_list.list)
      if (auto i = dest.find(traffic.id); i != dest.end())
        dest.erase(i);
  }

  /* remove all items that havn't been seen again for too long */
  std::erase_if(dest, [now](const auto &i){
    /* friends expire after 10 minutes, all others after one minute */
    const auto max_age = IsFriend(i.first)
      ? std::chrono::minutes{10}
      : std::chrono::minutes{1};

    return i.second.valid.IsOlderThan(now, max_age);
  });
}

void
MapWindowBlackboard::ReadBlackboard(const MoreData &nmea_info,
				    const DerivedInfo &derived_info) noexcept
{
  UpdateFadingTraffic(settings_map.fade_traffic,
                      fading_flarm_traffic, gps_info.flarm.traffic,
                      nmea_info.flarm.traffic,
                      nmea_info.clock);

  gps_info = nmea_info;
  calculated_info = derived_info;
}

