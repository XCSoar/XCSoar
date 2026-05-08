// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <cstdint>
#include <string_view>

/**
 * Parsed subset of an APRS position line from OGN / glidernet-style feeds.
 */
struct OGNAprsParseResult {
  bool valid = false;

  /**
   * APRS source address (before '>').
   */
  std::string_view station_id;

  GeoPoint location = GeoPoint::Invalid();

  /** Altitude above MSL [m]. */
  int altitude = 0;

  unsigned track_deg = 0;
  bool track_valid = false;

  /** Lower 24 bits are meaningful when flarm_valid. */
  uint32_t flarm_id = 0;
  bool flarm_valid = false;

  /** Compact aircraft type hint (0 if unknown). */
  unsigned aircraft_type = 0;
};

/**
 * Parse one APRS-IS text line.  Invalid or irrelevant lines yield
 * result.valid == false.
 */
[[gnu::pure]]
OGNAprsParseResult
ParseOGNAprsLine(std::string_view line) noexcept;
