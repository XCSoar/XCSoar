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

  /** Location of the FLARM target */
  GeoPoint Location;
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
  TCHAR Name[10];
  unsigned short IDType;
  unsigned short AlarmLevel;
  /** Last time the FLARM target was seen */
  fixed Time_Fix;
  AircraftType Type;
  fixed Average30s;

  bool defined() const {
    return ID.defined();
  }

  bool HasAlarm() const {
    return (AlarmLevel > 0 && AlarmLevel < 4);
  }

  bool HasName() const {
    return Name[0] != _T('\0');
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
    ID.clear();
    Name[0] = 0;
  }

  /**
   * Clear this object if its data has expired.
   *
   * @return true if the object is still valid
   */
  bool Refresh(fixed Time) {
    if (!defined())
      return false;

    // if (FLARM target is too old or time has gone backwards)
    if (Time > fixed(Time_Fix) + fixed_two || Time < fixed(Time_Fix)) {
      // clear this slot if it is too old (2 seconds), or if
      // time has gone backwards (due to replay)
      Clear();
      return false;
    } else {
      // FLARM data is present
      return true;
    }
  }

  static const TCHAR* GetTypeString(AircraftType type);
};

#endif
