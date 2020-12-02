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

#ifndef XCSOAR_FANET_STATIONS_HPP
#define XCSOAR_FANET_STATIONS_HPP

#include "util/StaticString.hxx"
#include <type_traits>
#include <tchar.h>
#include "FanetAddress.hpp"
#include "NMEA/Validity.hpp"
#include "Geo/GeoPoint.hpp"

struct FanetStation {
  /** Station info */
  float temperature;
  float wind_dir_deg;
  float wind_speed_kmph;
  float wind_gust_kmph;
  float rel_humidity;
  double preasure_hpa;
  float soc_percent;

  /** Has the geographical location been calculated yet? */
  bool location_available;

  /** Is this object valid, or has it expired already? */
  Validity valid;

  /** Location of the FANET station */
  GeoPoint location;

  /** FANET address of the FANET station */
  FanetAddress address;

  void Clear() {
    valid.Clear();
  }

  /**
   * Check if this station data has expired.
   *
   * @return true if the data is still valid
   */
  bool IsValid(double clock) {
    /*
     * Recommended interval for FANET type 4 packet is 40sec
     * We use 60sec accounting for differences in implementations but its still
     * short enough period to be relevant considering airports use 2min average
     */
    valid.Expire(clock, std::chrono::seconds(60));
    return valid;
  }

  /**
   * Updates weather data in this FanetStation with data from other FanetStation
   *
   * @param other FanetStation to update data FROM
   */
  void Update(const FanetStation &other);
};

static_assert(std::is_trivial<FanetStation>::value, "type is not trivial");

#endif //XCSOAR_FANET_STATIONS_HPP
