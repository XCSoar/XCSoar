// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"

#include <string_view>

class WaypointReaderCompeGPS final : public WaypointReaderBase {
  bool is_utm = false;

public:
  explicit WaypointReaderCompeGPS(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

  [[gnu::pure]]
  static bool VerifyFormat(std::string_view contents) noexcept;

protected:
  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const TCHAR *line, Waypoints &way_points) override;
};
