// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"
#include "io/StringConverter.hpp"

/**
 * Parses a SeeYou waypoint file.
 *
 * @see http://data.naviter.si/docs/cup_format.pdf
 */
class WaypointReaderSeeYou final : public WaypointReaderBase {
  StringConverter string_converter;

  bool first = true;

  bool ignore_following = false;

private:
  /* field positions for typical SeeYou *.cup waypoint file */
  unsigned iFrequency = 9;
  unsigned iDescription = 10;

public:
  explicit WaypointReaderSeeYou(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const char *line, Waypoints &way_points) override;
};
