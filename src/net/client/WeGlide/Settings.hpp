// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDate.hpp"

#include <cstdint>

/**
 * Settings for the WeGlide access for following tasks:
 * - Uploading IGC files to the server
 * - Get the active task from server
 */
struct WeGlideSettings {
  /**
   * Enable the general communication to the WeGlide server
   */
  bool enabled;

  /**
   * Automatic upload IGC file to WeGlide server after downloading an IGC file
   * from logger/
   */
  bool automatic_upload;

  /**
   * hard coded yet, maybe WeGlide change this in the future, then you have to
   * make it settable!
   * The documentation of the WeGlide API you can find:
   * https://api.weglide.org/docs
  */
  static constexpr char default_url[] = "https://api.weglide.org/v1";
  static constexpr char gliderlist_uri[] = "https://raw.githubusercontent.com/"
    "weglide/GliderList/master/gliderlist.csv";

  uint32_t pilot_id;
  BrokenDate pilot_birthdate;

  void SetDefaults() noexcept {
    pilot_id = 0;
    pilot_birthdate.Clear();

    enabled = false;
    automatic_upload = true; // after enabling WeGlide!
  }
};
