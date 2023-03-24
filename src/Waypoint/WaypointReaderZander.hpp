// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"

class WaypointReaderZander final : public WaypointReaderBase {
public:
  explicit WaypointReaderZander(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

protected:
  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const TCHAR *line, Waypoints &way_points) override;
};
