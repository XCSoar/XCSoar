// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "Geo/GeoPoint.hpp"
#include "Atmosphere/Pressure.hpp"
#include "util/StaticString.hxx"

#include <cstdint>

struct ParsedMETAR
{
  StaticString<5> icao_code;

  uint8_t day_of_month;

  /** Hour and minute of the METAR release time in UTC */
  uint8_t hour, minute;

  bool name_available;
  bool qnh_available;
  bool wind_available;
  bool temperatures_available;
  bool visibility_available;
  bool location_available;

  bool cavok;

  StaticString<128> name;
  AtmosphericPressure qnh;
  SpeedVector wind;
  double temperature, dew_point;
  unsigned visibility;
  GeoPoint location;

  void Reset() {
    name_available = false;
    qnh_available = false;
    wind_available = false;
    temperatures_available = false;
    visibility_available = false;
    location_available = false;
    cavok = false;
  }
};
