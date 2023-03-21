// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"

class WaypointReaderOzi final : public WaypointReaderBase {
  /* ignore the first 4 lines */
  unsigned ignore_lines = 4;

public:
  explicit WaypointReaderOzi(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

  static bool VerifyFormat(TLineReader &reader);

protected:
  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const TCHAR *line, Waypoints &way_points) override;
};
