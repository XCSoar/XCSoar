// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GetWaypointReachability.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Computer/GlideComputer.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Interface.hpp"

WaypointReach
GetWaypointReach(const Waypoint &waypoint) noexcept
{
  if (!waypoint.IsLandable() && !waypoint.flags.watched)
    return {};

  const auto *glide_computer = backend_components != nullptr
    ? backend_components->glide_computer.get()
    : nullptr;
  const auto &settings = CommonInterface::GetComputerSettings();

  return CalculateWaypointReach(waypoint,
                                glide_computer != nullptr
                                ? &glide_computer->GetProtectedRoutePlanner()
                                : nullptr,
                                CommonInterface::Basic(),
                                CommonInterface::Calculated(),
                                settings.polar, settings.task);
}
