// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/Friends.hpp"
#include "FLARM/FlarmId.hpp"
#include "Global.hpp"
#include "TrafficDatabases.hpp"

FlarmColor
FlarmFriends::GetFriendColor(FlarmId id)
{
  if (traffic_databases == nullptr)
    return FlarmColor::NONE;

  return traffic_databases->GetColor(id);
}

void
FlarmFriends::SetFriendColor(FlarmId id, FlarmColor color)
{
  assert(traffic_databases != nullptr);

  if (color == FlarmColor::NONE)
    traffic_databases->flarm_colors.Remove(id);
  else
    traffic_databases->flarm_colors.Set(id, color);
}
