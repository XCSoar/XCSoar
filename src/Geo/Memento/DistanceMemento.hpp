/* Copyright_License {

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

#ifndef DISTANCE_MEMENTO_HPP
#define DISTANCE_MEMENTO_HPP

#include "Geo/GeoPoint.hpp"
#include "Compiler.h"

/** Memento object to store results of previous distance calculations. */
class DistanceMemento
{
  /** Origin point of saved query */
  mutable GeoPoint origin;

  /** Destination point of previous query */
  mutable GeoPoint destination;

  /** Distance in meters saved from previous query */
  mutable double value;

public:
  /** Constructor, initialises to trigger update on first call. */
  DistanceMemento(): value(-1.0) {}

  /**
   * Returns the distance from the origin to destination in meters, 
   * from previously saved value if input arguments are identical. 
   */
  gcc_pure
  double Distance(const GeoPoint& _origin, const GeoPoint& _destination) const;
};

#endif
