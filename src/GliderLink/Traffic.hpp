/*
 Copyright_License {

 XCSoar Glide Computer - http://www.xcsoar.org/
 Copyright (C) 2000-2018 The XCSoar Project
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

#ifndef XCSOAR_GLIDER_LINK_TRAFFIC_HPP
#define XCSOAR_GLIDER_LINK_TRAFFIC_HPP

#include "GliderLinkId.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "Util/StaticString.hxx"
#include "Rough/RoughAltitude.hpp"
#include "Rough/RoughSpeed.hpp"
#include "Rough/RoughAngle.hpp"

#include <type_traits>

struct GliderLinkTraffic {
  /** Was the direction of the target received? */
  bool track_received;

  /** Was the speed of the target received? */
  bool speed_received;

  /** Was the absolute altitude of the target received? */
  bool altitude_received;

  /** Was the climb_rate of the target received? */
  bool climb_rate_received;

  /** Is this object valid, or has it expired already? */
  Validity valid;

  /** Location of the GliderLink target */
  GeoPoint location;

  /** TrackBearing of the GliderLink target */
  RoughAngle track;

  /** Speed of the GliderLink target */
  RoughSpeed speed;

  /** Altitude of the GliderLink target */
  RoughAltitude altitude;

  /** Climb rate of the GliderLink target */
  double climb_rate;

  /** GliderLink id of the GliderLink target */
  GliderLinkId id;

  /** (if exists) Name of the GliderLink target */
  StaticString<10> name;


  bool IsDefined() const {
    return valid;
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

  /**
   * Clear this object if its data has expired.
   *
   * @return true if the object is still valid
   */
  bool Refresh(double Time) {
    valid.Expire(Time, double(5*60));
    return valid;
  }
};

static_assert(std::is_trivial<GliderLinkTraffic>::value, "type is not trivial");

#endif
