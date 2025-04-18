// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"
#include "io/StringConverter.hpp"

/**
 * Waypoint file read/writer for WinPilot format
 */
class WaypointReaderWinPilot final : public WaypointReaderBase {
  StringConverter string_converter;

  bool first = true;
  bool welt2000_format = false;

public:
  explicit WaypointReaderWinPilot(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

protected:
  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const char *line, Waypoints &way_points) override;
};
