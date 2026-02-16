// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Id.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "util/StaticString.hxx"
#include "Rough/RoughAltitude.hpp"
#include "Rough/RoughDistance.hpp"
#include "Rough/RoughSpeed.hpp"
#include "Rough/RoughAngle.hpp"

#include <type_traits>
#include <tchar.h>

struct FlarmTraffic {
  enum class AlarmType: uint8_t {
    NONE = 0,
    LOW = 1,
    IMPORTANT = 2,
    URGENT = 3,
    INFO_ALERT = 4,
  };

  /**
   * FLARM aircraft types
   * @see http://www.flarm.com/support/manual/FLARM_DataportManual_v4.06E.pdf
   * Page 8
   */
  enum class AircraftType: uint8_t {
    UNKNOWN = 0,          //!< unknown
    GLIDER = 1,           //!< glider / motor-glider
    TOW_PLANE = 2,        //!< tow / tug plane
    HELICOPTER = 3,       //!< helicopter / rotorcraft
    PARACHUTE = 4,        //!< parachute
    DROP_PLANE = 5,       //!< drop plane for parachutes
    HANG_GLIDER = 6,      //!< hang-glider (hard)
    PARA_GLIDER = 7,      //!< para-glider (soft)
    POWERED_AIRCRAFT = 8, //!< powered aircraft
    JET_AIRCRAFT = 9,     //!< jet aircraft
    FLYING_SAUCER = 10,   //!< flying saucer (UFO)
    BALLOON = 11,         //!< balloon
    AIRSHIP = 12,         //!< airship
    UAV = 13,             //!< unmanned aerial vehicle
    STATIC_OBJECT = 15    //!< static object
  };

  /** Location of the FLARM target */
  GeoPoint location;

  /** Turnrate of the FLARM target */
  double turn_rate;

  /** Climbrate of the FLARM target */
  double climb_rate;

  /** Average climb rate over 30s */
  double climb_rate_avg30s;

  /** Latitude-based distance of the FLARM target */
  double relative_north;

  /** Longitude-based distance of the FLARM target */
  double relative_east;

  /** Is this object valid, or has it expired already? */
  Validity valid;

  /** FLARM id of the FLARM target */
  FlarmId id;

  /** Distance from our plane to the FLARM target */
  RoughDistance distance;

  /** TrackBearing of the FLARM target */
  RoughAngle track;

  /** Speed of the FLARM target */
  RoughSpeed speed;

  /** Altitude of the FLARM target */
  RoughAltitude altitude;

  /** Altidude-based distance of the FLARM target */
  RoughAltitude relative_altitude;

  /** (if exists) Name of the FLARM target */
  StaticString<10> name;

  AlarmType alarm_level;

  /** Type of the aircraft */
  AircraftType type;

  /** Is the target in stealth mode */
  bool stealth;

  /** Has the geographical location been calculated yet? */
  bool location_available;

  /** Was the direction of the target received from the flarm or calculated? */
  bool track_received;

  /** Was the speed of the target received from the flarm or calculated? */
  bool speed_received;

  /** Has the absolute altitude of the target been calculated yet? */
  bool altitude_available;

  /** Was the turn rate of the target received from the flarm or calculated? */
  bool turn_rate_received;

  /** Was the climb_rate of the target received from the flarm or calculated? */
  bool climb_rate_received;

  /** Has the averaged climb rate of the target been calculated yet? */
  bool climb_rate_avg30s_available;

  bool IsDefined() const noexcept {
    return valid;
  }

  bool HasAlarm() const noexcept {
    return alarm_level != AlarmType::NONE;
  }

  /**
   * Does the target have a name?
   * @return True if a name has been assigned to the target
   */
  bool HasName() const noexcept {
    return !name.empty();
  }

  void Clear() noexcept {
    valid.Clear();
    name.clear();
  }

  Angle Bearing() const noexcept {
    return Angle::FromXY(relative_north, relative_east);
  }

  bool IsPowered() const noexcept {
    return type != AircraftType::GLIDER &&
           type != AircraftType::HANG_GLIDER &&
           type != AircraftType::PARA_GLIDER;
  }

  bool IsPassive() const noexcept {
    return IsPowered() || speed < 4;
  }

  /**
   * Clear this object if its data has expired.
   *
   * @return true if the object is still valid
   */
  bool Refresh(TimeStamp Time) noexcept {
    valid.Expire(Time, std::chrono::seconds(2));
    return valid;
  }

  [[gnu::const]]
  static const char *GetTypeString(AircraftType type) noexcept;

  void Update(const FlarmTraffic &other) noexcept;
};

static_assert(std::is_trivial<FlarmTraffic>::value, "type is not trivial");
