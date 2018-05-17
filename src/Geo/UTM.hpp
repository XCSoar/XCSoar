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

#ifndef UTM_HPP
#define UTM_HPP

#include "Compiler.h"

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

  gcc_const
  static UTM FromGeoPoint(GeoPoint p);

  gcc_pure
  GeoPoint ToGeoPoint() const;
};

#endif
