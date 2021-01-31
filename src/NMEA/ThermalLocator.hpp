/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_NMEA_THERMAL_LOCATOR_HPP
#define XCSOAR_NMEA_THERMAL_LOCATOR_HPP

#include "Geo/GeoPoint.hpp"
#include "util/TrivialArray.hxx"

#include <type_traits>

struct SpeedVector;

/** Structure to hold information on identified thermal sources on the ground */
struct ThermalSource
{
  GeoPoint location;
  double ground_height;
  double lift_rate;
  double time;

  GeoPoint CalculateAdjustedLocation(double altitude,
                                     const SpeedVector &wind) const;
};

/** Structure for current thermal estimate from ThermalLocator */
struct ThermalLocatorInfo
{
  static constexpr unsigned MAX_SOURCES = 20;

  /** Location of thermal at aircraft altitude */
  GeoPoint estimate_location;
  /** Is thermal estimation valid? */
  bool estimate_valid;

  /** Position and data of the last thermal sources */
  TrivialArray<ThermalSource, MAX_SOURCES> sources;

  void Clear();

  /**
   * Allocate a new #THERMAL_SOURCE_INFO slot; discard the oldest one
   * if the list is full.
   */
  ThermalSource &AllocateSource();
};

static_assert(std::is_trivial<ThermalLocatorInfo>::value,
              "type is not trivial");

#endif
