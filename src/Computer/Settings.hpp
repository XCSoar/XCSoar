// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
#include "RadioFrequency.hpp"
#include "TransponderCode.hpp"
#include "net/client/WeGlide/Settings.hpp"

#include <cstdint>
#include <type_traits>

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
   * The reference location & declination for the "ATC radial" InfoBox.
   */
  GeoPoint atc_reference;
  Angle magnetic_declination;

  /**
   * elevation of home waypoint is available
   */
  bool home_elevation_available;

  /**
   * elevation (if available) of home waypoint
   */
  double home_elevation;

  void SetDefaults() {
    ClearHome();
    atc_reference.SetInvalid();
    magnetic_declination = Angle::Zero();
  }

  void ClearHome();
  void SetHome(const Waypoint &wp);
};

/**
 * Options for radio remote control
 */
struct RadioSettings {
  RadioFrequency active_frequency;
  RadioFrequency standby_frequency;

  StaticString<32> active_name;
  StaticString<32> standby_name;

  void SetDefaults() {
    active_frequency.Clear();
    standby_frequency.Clear();
    active_name.clear();
    standby_name.clear();
  }
};

/**
 * Options for transponder remote control
 */
struct TransponderSettings {
  TransponderCode transponder_code;

  void SetDefaults() {
    transponder_code.Clear();
  }
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
  FloatDuration cruise_to_circling_mode_switch_threshold;
  FloatDuration circling_to_cruise_mode_switch_threshold;

  void SetDefaults() {
    external_trigger_cruise_enabled = false;
    cruise_to_circling_mode_switch_threshold = std::chrono::seconds{15};
    circling_to_cruise_mode_switch_threshold = std::chrono::seconds{10};
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

  WeGlideSettings weglide;

#ifdef HAVE_TRACKING
  TrackingSettings tracking;
#endif

  WeatherSettings weather;

  RadioSettings radio;

  TransponderSettings transponder;

  void SetDefaults();
};

static_assert(std::is_trivial<ComputerSettings>::value,
              "type is not trivial");
