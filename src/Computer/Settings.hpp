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

#ifndef XCSOAR_COMPUTER_SETTINGS_HPP
#define XCSOAR_COMPUTER_SETTINGS_HPP

#include "Geo/GeoPoint.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Engine/Contest/Settings.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "Weather/Settings.hpp"
#include "NMEA/Validity.hpp"
#include "Logger/Settings.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "TeamCode/Settings.hpp"
#include "Plane/Plane.hpp"
#include "Wind/Settings.hpp"
#include "WaveSettings.hpp"

#include <type_traits>

#include <stdint.h>

struct Waypoint;

// control of calculations, these only changed by user interface
// but are used read-only by calculations

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
  double degradation_factor;

  /**
   * Bugs ratio applied to polar.  Range is 0..1, where 1 means
   * "clean".  Unlike the "degradation_factor" attribute above, this
   * setting is not permanent.
   *
   * When you modify this, you should update glide_polar_task and send
   * the new GlidePolar to the TaskManager.
   */
  double bugs;

  /** Glide polar used for task calculations */
  GlidePolar glide_polar_task;

  /** Whether the ballast countdown timer is active */
  bool ballast_timer_active;

  /**
   * Add 1% bugs each full hour?
   */
  bool auto_bugs;

  void SetDefaults();

  void SetDegradationFactor(double _degradation_factor) {
    degradation_factor = _degradation_factor;
    glide_polar_task.SetBugs(degradation_factor * bugs);
  }

  void SetBugs(double _bugs) {
    bugs = _bugs;
    glide_polar_task.SetBugs(degradation_factor * bugs);
  }
};

/**
 * Options for tracking places of interest as alternates
 */
struct PlacesOfInterestSettings {
  /** Array index of the home waypoint */
  int home_waypoint;

  bool home_location_available;

  GeoPoint home_location;

  /**
   * The reference location for the "ATC radial" InfoBox.
   */
  GeoPoint atc_reference;

  void SetDefaults() {
    ClearHome();
    atc_reference.SetInvalid();
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
    TERRAIN_LINE,
    TERRAIN_SHADE,
    WORKING,
    WORKING_TERRAIN_LINE,
    WORKING_TERRAIN_SHADE,
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

  PlacesOfInterestSettings poi;

  FeaturesSettings features;

  CirclingSettings circling;

  WaveSettings wave;

  AverageEffTime average_eff_time;

  /** Update system time from GPS time */
  bool set_system_time_from_gps;

  /** local time adjustment (in seconds) */
  RoughTimeDelta utc_offset;

  /**
   * The forecasted maximum ground temperature [Kelvin].
   */
  Temperature forecast_temperature;

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

  WeatherSettings weather;

  void SetDefaults();
};

static_assert(std::is_trivial<ComputerSettings>::value,
              "type is not trivial");

#endif

