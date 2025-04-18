// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"
#include "io/StringConverter.hpp"

class WaypointReaderZander final : public WaypointReaderBase {
  StringConverter string_converter;

public:
  explicit WaypointReaderZander(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

protected:
  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const char *line, Waypoints &way_points) override;
};
