// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RoughTimeProfile.hpp"
#include "Map.hpp"
#include "time/RoughTime.hpp"

bool
Profile::Get(const ProfileMap &map, std::string_view key, RoughTime &value) noexcept
{
  int minutes_of_day;
  if (!map.Get(key, minutes_of_day))
    return false;

  if (minutes_of_day < 0 || minutes_of_day >= 24 * 60)
    return false;

  value = RoughTime::FromMinuteOfDay(minutes_of_day);
  return true;
}

void
Profile::Set(std::string_view key, const RoughTime &value) noexcept
{
  if (!value.IsValid())
    return;

  Profile::Set(key, value.GetMinuteOfDay());
}