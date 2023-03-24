// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"

class WaypointReaderCompeGPS final : public WaypointReaderBase {
  bool is_utm = false;

public:
  explicit WaypointReaderCompeGPS(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

  static bool VerifyFormat(TLineReader &reader);

protected:
  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const TCHAR *line, Waypoints &way_points) override;
};
