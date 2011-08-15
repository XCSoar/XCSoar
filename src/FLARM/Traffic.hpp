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

#ifndef XCSOAR_FLARM_TRAFFIC_HPP
#define XCSOAR_FLARM_TRAFFIC_HPP

#include "Util/TinyEnum.hpp"
#include "FlarmId.hpp"
#include "Navigation/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "Util/StaticString.hpp"

#include <tchar.h>

struct FLARM_TRAFFIC {
  enum AlarmType {
    ALARM_NONE = 0,
    ALARM_LOW = 1,
    ALARM_IMPORTANT = 2,
    ALARM_URGENT = 3,
  };

  /**
   * FLARM aircraft types
   * @see http://www.flarm.com/support/manual/FLARM_DataportManual_v4.06E.pdf
   * Page 8
   */
  enum AircraftType {
    acUnknown = 0,         //!< unknown
    acGlider = 1,          //!< glider / motor-glider
    acTowPlane = 2,        //!< tow / tug plane
    acHelicopter = 3,      //!< helicopter / rotorcraft
    acParachute = 4,       //!< parachute
    acDropPlane = 5,       //!< drop plane for parachutes
    acHangGlider = 6,      //!< hang-glider (hard)
    acParaGlider = 7,      //!< para-glider (soft)
    acPoweredAircraft = 8, //!< powered aircraft
    acJetAircraft = 9,     //!< jet aircraft
    acFlyingSaucer = 10,   //!< flying saucer (UFO)
    acBalloon = 11,        //!< balloon
    acAirship = 12,        //!< airship
    acUAV = 13,            //!< unmanned aerial vehicle
    acStaticObject = 15    //!< static object
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
  fixed distance;

  /** TrackBearing of the FLARM target */
  Angle track;

  /** Speed of the FLARM target */
  fixed speed;

  /** Altitude of the FLARM target */
  fixed altitude;

  /** Turnrate of the FLARM target */
  fixed turn_rate;

  /** Climbrate of the FLARM target */
  fixed climb_rate;

  /** Latitude-based distance of the FLARM target */
  fixed relative_north;

  /** Longitude-based distance of the FLARM target */
  fixed relative_east;

  /** Altidude-based distance of the FLARM target */
  fixed relative_altitude;

  /** FLARM id of the FLARM target */
  FlarmId id;

  /** (if exists) Name of the FLARM target */
  StaticString<10> name;

  TinyEnum<AlarmType> alarm_level;

  /** Type of the aircraft */
  TinyEnum<AircraftType> type;

  /** Average climb rate over 30s */
  fixed climb_rate_avg30s;

  bool IsDefined() const {
    return valid;
  }

  bool HasAlarm() const {
    return alarm_level != ALARM_NONE;
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
    return Angle::from_xy(relative_north, relative_east);
  }

  bool IsPowered() const {
    return type != acGlider && type != acHangGlider && type != acParaGlider;
  }

  bool IsPassive() const {
    return IsPowered() || speed < fixed_four;
  }

  /**
   * Clear this object if its data has expired.
   *
   * @return true if the object is still valid
   */
  bool Refresh(fixed Time) {
    valid.Expire(Time, fixed_two);
    return valid;
  }

  static const TCHAR* GetTypeString(AircraftType type);

  void Update(const FLARM_TRAFFIC &other);
};

#endif
