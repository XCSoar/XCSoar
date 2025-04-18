// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"

void
LoggerSettings::SetDefaults()
{
  time_step_cruise = std::chrono::seconds{5};
  time_step_circling = std::chrono::seconds{1};
  auto_logger = AutoLogger::ON;
  logger_id.clear();
  pilot_name.clear();
  copilot_name.clear();
  crew_mass_template = 90;

  /* XXX disabled by default for now, until the FlightLogger
     implementation is finished */
  enable_flight_logger = false;

  enable_nmea_logger = false;
}
