// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct GeoPoint;

struct UTM {
  uint_least8_t zone_number;
  char zone_letter;

  double easting, northing;

  UTM() = default;
  constexpr UTM(uint_least8_t _zone_number, char _zone_letter,
                double _easting, double _northing) noexcept
    :zone_number(_zone_number), zone_letter(_zone_letter),
     easting(_easting), northing(_northing) {}

  [[gnu::const]]
  static UTM FromGeoPoint(GeoPoint p) noexcept;

  [[gnu::pure]]
  GeoPoint ToGeoPoint() const noexcept;
};
