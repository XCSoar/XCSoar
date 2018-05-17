/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_TEAM_CODE_SETTINGS_HPP
#define XCSOAR_TEAM_CODE_SETTINGS_HPP

#include "Util/StaticString.hxx"
#include "TeamCode.hpp"
#include "FLARM/FlarmId.hpp"

#include <type_traits>

/**
 * Settings for teamcode calculations
 */
struct TeamCodeSettings {
  /** Reference waypoint id for code origin */
  int team_code_reference_waypoint;

  /** CN of the glider to track */
  StaticString<4> team_flarm_callsign;

  /**
   * Check TeamCode::IsDefined() before using this attribute.
   */
  TeamCode team_code;

  /**
   * FlarmId of the glider to track.  Check FlarmId::IsDefined()
   * before using this attribute.
   */
  FlarmId team_flarm_id;

  void SetDefaults();

  /**
   * Track a specific FLARM.
   *
   * Don't use this method directory, use TeamActions::TrackFlarm()
   * instead.
   */
  void TrackFlarm(FlarmId id, const TCHAR *callsign);
};

static_assert(std::is_trivial<TeamCodeSettings>::value, "type is not trivial");

#endif
