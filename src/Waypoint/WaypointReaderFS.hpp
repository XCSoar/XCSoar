// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"
#include "io/StringConverter.hpp"

#include <string_view>

class WaypointReaderFS final : public WaypointReaderBase {
  StringConverter string_converter;

  bool is_utm = false;

public:
  explicit WaypointReaderFS(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

  [[gnu::pure]]
  static bool VerifyFormat(std::string_view contents) noexcept;

protected:
  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const char *line, Waypoints &way_points) override;
};
