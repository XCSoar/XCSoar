// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlideComputerBlackboard.hpp"

/**
 * Resets the GlideComputerBlackboard
 * @param full Reset all data?
 */
void
GlideComputerBlackboard::ResetFlight([[maybe_unused]] const bool full)
{
  gps_info.Reset();
  calculated_info.Reset();
}

/**
 * Starts the task on the GlideComputerBlackboard
 */
void
GlideComputerBlackboard::StartTask()
{
  calculated_info.cruise_start_location = gps_info.location;
  calculated_info.cruise_start_altitude = gps_info.nav_altitude;
  calculated_info.cruise_start_time = gps_info.time;

  // JMW reset time cruising/time circling stats on task start
  calculated_info.time_circling = {};
  calculated_info.time_cruise = {};
  calculated_info.time_climb_noncircling = {};
  calculated_info.time_climb_circling = {};
  calculated_info.total_height_gain = 0;

  // reset max height gain stuff on task start
  calculated_info.max_height_gain = 0;
}

void
GlideComputerBlackboard::SaveFinish()
{
  // JMW save calculated data at finish
  Finish_Derived_Info = calculated_info;
}

void
GlideComputerBlackboard::RestoreFinish()
{
  const auto flight = calculated_info.flight;

  calculated_info = Finish_Derived_Info;

  /* retain some of the data to avoid confusing some of our subsystems
     (e.g. spurious takeoff/landing detection) */
  calculated_info.flight = flight;
}

/**
 * Retrieves GPS data from the DeviceBlackboard
 * @param nmea_info New GPS data
 */
void
GlideComputerBlackboard::ReadBlackboard(const MoreData &nmea_info)
{
  gps_info = nmea_info;
}

/**
 * Retrieves settings from the DeviceBlackboard
 * @param settings New settings
 */
void
GlideComputerBlackboard::ReadComputerSettings(const ComputerSettings &settings)
{
  computer_settings = settings;
}
