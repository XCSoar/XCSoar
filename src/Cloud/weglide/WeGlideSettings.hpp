/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_CLOUD_WEGLIDE_WEGLIDESETTINGS_HPP
#define XCSOAR_CLOUD_WEGLIDE_WEGLIDESETTINGS_HPP

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

  void SetDefaults() noexcept;
};

#endif  // XCSOAR_CLOUD_WEGLIDE_WEGLIDESETTINGS_HPP
