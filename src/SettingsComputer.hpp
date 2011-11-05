/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_SETTINGS_COMPUTER_HPP
#define XCSOAR_SETTINGS_COMPUTER_HPP

#include "FLARM/FlarmId.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Atmosphere/Pressure.hpp"
#include "Engine/Route/Config.hpp"
#include "Util/StaticString.hpp"
#include "Util/TypeTraits.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "Engine/Navigation/SpeedVector.hpp"
#include "NMEA/Validity.hpp"

#include "Airspace/AirspaceComputerSettings.hpp"
#include "TeamCodeCalculation.hpp"
#include "Plane/Plane.hpp"

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
   * AutoWind calculation mode
   * 0: Manual
   * 1: Circling
   * 2: ZigZag
   * 3: Both
   */
  uint8_t auto_wind_mode;

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
};

static_assert(is_trivial_clang<WindSettings>::value, "type is not trivial");

/**
 * Logger settings
 */
struct LoggerSettings {
  /** Logger interval in cruise mode */
  uint16_t logger_time_step_cruise;
  /** Logger interval in circling mode */
  uint16_t logger_time_step_circling;
  /** Use short IGC filenames for the logger files */
  bool logger_short_name;
  bool auto_logger_disabled;

  void SetDefaults();
};

/**
 * Glide polar settings
 */
struct PolarSettings {
  /** Whether the ballast countdown timer is active */
  bool ballast_timer_active;

  void SetDefaults();
};

struct SoundSettings {
  // sound stuff not used?
  bool sound_vario_enabled;
  bool sound_task_enabled;
  bool sound_modes_enabled;
  uint8_t sound_volume;
  uint8_t sound_deadband;

  void SetDefaults();
};

/** 
 * Settings for teamcode calculations
 */
struct TeamCodeSettings {
  /** Reference waypoint id for code origin */
  int team_code_reference_waypoint;
  /** Whether to enable tracking by FLARM */
  bool team_flarm_tracking;
  /** Whether the teammate code is valid */
  bool team_code_valid;

  /** CN of the glider to track */
  StaticString<4> team_flarm_callsign;
  /** auto-detected, see also in Info.h */
  TeamCode team_code;

  /** FlarmId of the glider to track */
  FlarmId team_flarm_id;

  void SetDefaults();
};

static_assert(is_trivial_clang<TeamCodeSettings>::value,
              "type is not trivial");

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
  enum FinalGlideTerrain {
    FGT_OFF,
    FGT_LINE,
    FGT_SHADE,
  } final_glide_terrain;

  /** block speed to fly instead of dolphin */
  bool block_stf_enabled;

  /** Navigate by baro altitude instead of GPS altitude */
  bool nav_baro_altitude_enabled;

  void SetDefaults();
};

enum AverageEffTime {
  ae15seconds,
  ae30seconds,
  ae60seconds,
  ae90seconds,
  ae2minutes,
  ae3minutes,
};

struct SETTINGS_COMPUTER: 
  public WindSettings,
  public LoggerSettings,
  public PolarSettings,
  public SoundSettings,
  public TeamCodeSettings,
  public VoiceSettings,
  public PlacesOfInterestSettings,
  public FeaturesSettings
{
  bool external_trigger_cruise_enabled;

  AverageEffTime average_eff_time;

  /** Update system time from GPS time */
  bool set_system_time_from_gps;

  /** local time adjustment */
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

  /** Glide polar used for task calculations */
  GlidePolar glide_polar_task;

  AirspaceComputerSettings airspace;
  Plane plane;

  TaskBehaviour task;

#ifdef HAVE_TRACKING
  TrackingSettings tracking;
#endif

  void SetDefaults();
};

static_assert(is_trivial_clang<SETTINGS_COMPUTER>::value,
              "type is not trivial");

#endif

