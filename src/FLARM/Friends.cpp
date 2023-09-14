// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Friends.hpp"
#include "Id.hpp"
#include "Global.hpp"
#include "TrafficDatabases.hpp"

namespace FlarmFriends {

FlarmColor
GetFriendColor(FlarmId id) noexcept
{
  if (traffic_databases == nullptr)
    return FlarmColor::NONE;

  return traffic_databases->GetColor(id);
}

void
SetFriendColor(FlarmId id, FlarmColor color) noexcept
{
  assert(traffic_databases != nullptr);

  if (color == FlarmColor::NONE)
    traffic_databases->flarm_colors.Remove(id);
  else
    traffic_databases->flarm_colors.Set(id, color);
}

} // namespace FlarmFriends
