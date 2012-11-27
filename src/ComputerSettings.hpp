/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_COMPUTER_SETTINGS_HPP
#define XCSOAR_COMPUTER_SETTINGS_HPP

#include "Geo/GeoPoint.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Engine/Route/Config.hpp"
#include "Engine/Contest/Settings.hpp"
#include "Util/StaticString.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "Geo/SpeedVector.hpp"
#include "NMEA/Validity.hpp"
#include "Logger/Settings.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "TeamCodeSettings.hpp"
#include "Plane/Plane.hpp"

#include <type_traits>

#include <stdint.h>

struct Waypoint;

// control of calculations, these only changed by user interface
// but are used read-only by calculations

/** AutoWindMode (not in use) */
enum AutoWindModeBits
{
  /** 0: Manual */
  AUTOWIND_NONE = 0,
  /** 1: Circling */
  AUTOWIND_CIRCLING,
  /** 2: ZigZag */
  AUTOWIND_ZIGZAG,
  /** 3: Both */
};

/**
 * Wind calculator settings
 */
struct WindSettings {
  /**
   * Use the circling algorithm to calculate the wind?
   */
  bool circling_wind;

  /**
   * Use the EKF algorithm to calculate the wind? (formerly known as
   * "zig zag")
   */
  bool zig_zag_wind;

  /**
   * If enabled, then the wind vector received from external devices
   * overrides XCSoar's internal wind calculation.
   */
  bool use_external_wind;

  /**
   * This is the manual wind set by the pilot. Validity is set when
   * changeing manual wind but does not expire.
   */
  SpeedVector manual_wind;
  Validity manual_wind_available;

  void SetDefaults();

  bool IsAutoWindEnabled() const {
    return circling_wind || zig_zag_wind;
  }

  bool CirclingWindEnabled() const {
    return circling_wind;
  }

  bool ZigZagWindEnabled() const {
    return zig_zag_wind;
  }

  unsigned GetLegacyAutoWindMode() const {
    return (circling_wind ? 0x1 : 0x0) | (zig_zag_wind ? 0x2 : 0x0);
  }

  void SetLegacyAutoWindMode(unsigned mode) {
    circling_wind = (mode & 0x1) != 0;
    zig_zag_wind = (mode & 0x2) != 0;
  }
};

static_assert(std::is_trivial<WindSettings>::value, "type is not trivial");

/**
 * Glide polar settings
 */
struct PolarSettings {
  /**
   * A global factor that is applied to the polar.  It is used to
   * degrade the polar permanently, to be multiplied with the current
   * "bugs" setting.  Range is 0..1, where 1 means "no degradation".
   *
   * When you modify this, you should update glide_polar_task and send
   * the new GlidePolar to the TaskManager.
   */
  fixed degradation_factor;

  /**
   * Bugs ratio applied to polar.  Range is 0..1, where 1 means
   * "clean".  Unlike the "degradation_factor" attribute above, this
   * setting is not permanent.
   *
   * When you modify this, you should update glide_polar_task and send
   * the new GlidePolar to the TaskManager.
   */
  fixed bugs;

  /** Glide polar used for task calculations */
  GlidePolar glide_polar_task;

  /** Whether the ballast countdown timer is active */
  bool ballast_timer_active;

  void SetDefaults();

  void SetDegradationFactor(fixed _degradation_factor) {
    degradation_factor = _degradation_factor;
    glide_polar_task.SetBugs(degradation_factor * bugs);
  }

  void SetBugs(fixed _bugs) {
    bugs = _bugs;
    glide_polar_task.SetBugs(degradation_factor * bugs);
  }
};

struct VoiceSettings {
  // vegavoice stuff
  bool voice_climb_rate_enabled;
  bool voice_terrain_enabled;
  bool voice_waypoint_distance_enabled;
  bool voice_task_altitude_difference_enabled;
  bool voice_mac_cready_enabled;
  bool voice_new_waypoint_enabled;
  bool voice_in_sector_enabled;
  bool voice_airspace_enabled;

  void SetDefaults();
};

/**
 * Options for tracking places of interest as alternates
 */
struct PlacesOfInterestSettings {
  /** Array index of the home waypoint */
  int home_waypoint;

  bool home_location_available;

  GeoPoint home_location;

  void SetDefaults() {
    ClearHome();
  }

  void ClearHome();
  void SetHome(const Waypoint &wp);
};


/**
 * Options for glide computer features
 */
struct FeaturesSettings {
  /** Calculate final glide over terrain */
  enum class FinalGlideTerrain : uint8_t {
    OFF,
    LINE,
    SHADE,
  } final_glide_terrain;

  /** block speed to fly instead of dolphin */
  bool block_stf_enabled;

  /** Navigate by baro altitude instead of GPS altitude */
  bool nav_baro_altitude_enabled;

  void SetDefaults();
};

struct CirclingSettings {
  bool external_trigger_cruise_enabled;

  void SetDefaults() {
    external_trigger_cruise_enabled = false;
  }
};

enum AverageEffTime {
  ae15seconds,
  ae30seconds,
  ae60seconds,
  ae90seconds,
  ae2minutes,
  ae3minutes,
};

struct ComputerSettings {
  WindSettings wind;

  PolarSettings polar;

  TeamCodeSettings team_code;

  VoiceSettings voice;

  PlacesOfInterestSettings poi;

  FeaturesSettings features;

  CirclingSettings circling;

  AverageEffTime average_eff_time;

  /** Update system time from GPS time */
  bool set_system_time_from_gps;

  /** local time adjustment (in seconds) */
  int utc_offset;

  /**
   * The forecasted maximum ground temperature [Kelvin].
   */
  fixed forecast_temperature;

  /**
   * Troposhere atmosphere model for QNH correction
   */
  AtmosphericPressure pressure;
  Validity pressure_available;

  AirspaceComputerSettings airspace;
  Plane plane;

  TaskBehaviour task;

  ContestSettings contest;

  LoggerSettings logger;

#ifdef HAVE_TRACKING
  TrackingSettings tracking;
#endif

  void SetDefaults();
};

static_assert(std::is_trivial<ComputerSettings>::value,
              "type is not trivial");

#endif

