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

#include "FlarmId.hpp"
#include "Navigation/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "Util/StaticString.hpp"

#include <tchar.h>

struct FLARM_TRAFFIC {
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

  /**
   * Is this object valid, or has it expired already?
   */
  Validity Valid;

  /** Location of the FLARM target */
  GeoPoint Location;

  /** Is the target in stealth mode */
  bool Stealth;

  /** TrackBearing of the FLARM target */
  Angle TrackBearing;
  /** Speed of the FLARM target */
  fixed Speed;
  /** Altitude of the FLARM target */
  fixed Altitude;
  /** Turnrate of the FLARM target */
  fixed TurnRate;
  /** Climbrate of the FLARM target */
  fixed ClimbRate;
  /** Latitude-based distance of the FLARM target */
  fixed RelativeNorth;
  /** Longitude-based distance of the FLARM target */
  fixed RelativeEast;
  /** Altidude-based distance of the FLARM target */
  fixed RelativeAltitude;
  /** FLARM id of the FLARM target */
  FlarmId ID;
  /** (if exists) Name of the FLARM target */
  StaticString<10> Name;
  unsigned short IDType;
  unsigned short AlarmLevel;
  AircraftType Type;
  fixed Average30s;

  bool defined() const {
    return Valid;
  }

  bool HasAlarm() const {
    return (AlarmLevel > 0 && AlarmLevel < 4);
  }

  bool HasName() const {
    return !Name.empty();
  }

  /**
   * Returns the squared distance.  When comparing distances, not
   * taking the square root saves a good amount of CPU cycles, and
   * has no effect on the result.
   */
  fixed SquareDistance() const {
    return RelativeAltitude * RelativeAltitude +
      RelativeEast * RelativeEast +
      RelativeNorth * RelativeNorth;
  }

  fixed Distance() const {
    return sqrt(SquareDistance());
  }

  void Clear() {
    Valid.Clear();
    Name.clear();
  }

  /**
   * Clear this object if its data has expired.
   *
   * @return true if the object is still valid
   */
  bool Refresh(fixed Time) {
    Valid.Expire(Time, fixed_two);
    return Valid;
  }

  static const TCHAR* GetTypeString(AircraftType type);

  void Update(const FLARM_TRAFFIC &other);
};

#endif
