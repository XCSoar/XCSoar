// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

#include <chrono>

/**
 * Logger settings
 */
struct LoggerSettings {
  /**
   * Enable the #FlightLogger?
   */
  bool enable_flight_logger;

  /**
   * Enable the #NMEALogger?
   */
  bool enable_nmea_logger;

  /** Logger interval in cruise mode */
  std::chrono::duration<unsigned> time_step_cruise;

  /** Logger interval in circling mode */
  std::chrono::duration<unsigned> time_step_circling;

  enum class AutoLogger: uint8_t {
    ON,
    START_ONLY,
    OFF,
  } auto_logger;

  StaticString<32> logger_id;

  StaticString<64> pilot_name;

  StaticString<64> copilot_name;

  /** Crew mass template in kg */
  unsigned crew_mass_template;

  void SetDefaults();
};
