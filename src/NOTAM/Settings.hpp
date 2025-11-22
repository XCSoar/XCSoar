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
  unsigned radius_km = 99;
  
  /** Maximum number of NOTAMs to fetch */
  unsigned max_notams = 1000;
  
  /** Refresh interval (in minutes) */
  std::chrono::minutes refresh_interval_min{30};
  
  /** Base URL for the NOTAM API */
  const char *api_base_url = "https://enroute-data.akaflieg-freiburg.de/enrouteProxy/notam.php";
};