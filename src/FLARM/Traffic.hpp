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

#ifndef XCSOAR_FLARM_TRAFFIC_HPP
#define XCSOAR_FLARM_TRAFFIC_HPP

#include "FlarmId.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "Util/StaticString.hxx"
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

  /** Is this object valid, or has it expired already? */
  Validity valid;

  /** Location of the FLARM target */
  GeoPoint location;

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

  /** Turnrate of the FLARM target */
  double turn_rate;

  /** Climbrate of the FLARM target */
  double climb_rate;

  /** Latitude-based distance of the FLARM target */
  double relative_north;

  /** Longitude-based distance of the FLARM target */
  double relative_east;

  /** FLARM id of the FLARM target */
  FlarmId id;

  /** (if exists) Name of the FLARM target */
  StaticString<10> name;

  AlarmType alarm_level;

  /** Type of the aircraft */
  AircraftType type;

  /** Average climb rate over 30s */
  double climb_rate_avg30s;

  bool IsDefined() const {
    return valid;
  }

  bool HasAlarm() const {
    return alarm_level != AlarmType::NONE;
  }

  /**
   * Does the target have a name?
   * @return True if a name has been assigned to the target
   */
  bool HasName() const {
    return !name.empty();
  }

  void Clear() {
    valid.Clear();
    name.clear();
  }

  Angle Bearing() const {
    return Angle::FromXY(relative_north, relative_east);
  }

  bool IsPowered() const {
    return type != AircraftType::GLIDER &&
           type != AircraftType::HANG_GLIDER &&
           type != AircraftType::PARA_GLIDER;
  }

  bool IsPassive() const {
    return IsPowered() || speed < 4;
  }

  /**
   * Clear this object if its data has expired.
   *
   * @return true if the object is still valid
   */
  bool Refresh(double Time) {
    valid.Expire(Time, 2);
    return valid;
  }

  static const TCHAR* GetTypeString(AircraftType type);

  void Update(const FlarmTraffic &other);
};

static_assert(std::is_trivial<FlarmTraffic>::value, "type is not trivial");

#endif
