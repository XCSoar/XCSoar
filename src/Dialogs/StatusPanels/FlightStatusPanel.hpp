// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StatusPanel.hpp"
#include "Engine/Waypoint/Ptr.hpp"

class FlightStatusPanel : public StatusPanel {
  const WaypointPtr nearest_waypoint;

public:
  FlightStatusPanel(const DialogLook &look, WaypointPtr &&_waypoint) noexcept
    :StatusPanel(look), nearest_waypoint(std::move(_waypoint)) {}

  /* virtual methods from class StatusPanel */
  void Refresh() noexcept override;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
