// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <cstdint>
#include <string>
#include <string_view>

/**
 * Parsed subset of an APRS position line from OGN / glidernet-style feeds.
 */
struct OGNAprsParseResult {
  bool valid = false;

  /**
   * APRS source address (before '>').
   */
  std::string station_id;

  GeoPoint location = GeoPoint::Invalid();

  /** Altitude above MSL [m]. */
  int altitude = 0;
  bool altitude_valid = false;

  unsigned track_deg = 0;
  bool track_valid = false;

  /** Lower 24 bits are meaningful when flarm_valid. */
  uint32_t flarm_id = 0;
  bool flarm_valid = false;

  /** Compact aircraft type hint (0 if unknown). */
  unsigned aircraft_type = 0;

  /**
   * Registration or callsign when present in the APRS comment (e.g. ADSB
   * "regHB-XXX" / "fnA...") or derived from the FLARM address.
   */
  std::string callsign;
};

/**
 * Parse one APRS-IS text line.  Invalid or irrelevant lines yield
 * result.valid == false.
 */
[[gnu::pure]]
OGNAprsParseResult
ParseOGNAprsLine(std::string_view line) noexcept;
