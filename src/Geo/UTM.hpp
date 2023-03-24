// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;

struct UTM {
  unsigned char zone_number;
  char zone_letter;

  double easting, northing;

  UTM() = default;
  constexpr UTM(unsigned char _zone_number, char _zone_letter,
                double _easting, double _northing)
    :zone_number(_zone_number), zone_letter(_zone_letter),
     easting(_easting), northing(_northing) {}

  [[gnu::const]]
  static UTM FromGeoPoint(GeoPoint p);

  [[gnu::pure]]
  GeoPoint ToGeoPoint() const;
};
