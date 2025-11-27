// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>

/**
 * Settings for NOTAM (Notice to Airmen) support
 */
struct NOTAMSettings {
  /** Enable/disable NOTAM loading */
  bool enabled = false;
  
  /** Radius around current location to search for NOTAMs (in kilometers) */
  unsigned radius_km = 50;
  
  /** Maximum number of NOTAMs to fetch */
  unsigned max_notams = 500;
  
  /** Refresh interval (in minutes) - 0 = manual only */
  unsigned refresh_interval_min = 60;
  
  /** Base URL for the NOTAM API */
  const char *api_base_url = "https://enroute-data.akaflieg-freiburg.de/enrouteProxy/notam.php";
  
  /** Filter NOTAMs that are only active during daylight (sunrise to sunset) */
  bool filter_daylight_only = false;
  
  /** Filter NOTAMs that are only active during night (sunset to sunrise) */
  bool filter_night_only = false;
  
  /** Hours before sunrise to include NOTAMs (-1 = disabled) */
  int hours_before_sunrise = -1;
  
  /** Hours after sunset to include NOTAMs (-1 = disabled) */
  int hours_after_sunset = -1;
  
  /** Filter by NOTAM series (empty = show all) */
  std::string filter_series = "";
  
  // Feature type filters (simplified for glider pilots - based on official NOTAM API)
  /** Show AIRSPACE-related NOTAMs (airspace restrictions) */
  bool show_airspace = true;
  
  /** Show OBST (obstacle) NOTAMs (towers, cranes, construction) */
  bool show_obst = true;
  
  /** Show MILITARY NOTAMs (military exercises, operations) */
  bool show_military = true;
  
  /** Show other/unclassified NOTAMs (AD, RWY, NAV, COM, procedures, etc.) */
  bool show_other = false;
};