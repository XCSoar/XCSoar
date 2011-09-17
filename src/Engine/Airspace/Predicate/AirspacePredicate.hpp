/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#ifndef AIRSPACE_PREDICATE_HPP
#define AIRSPACE_PREDICATE_HPP

#include "Compiler.h"
#include "Navigation/GeoPoint.hpp"

class AbstractAirspace;
class AirspacePredicateTrue;
struct AircraftState;

/**
 *  Functor class for conditions to be applied to airspace queries
 */
class AirspacePredicate
{
public:
  /**
   * Test condition
   * @param t Airspace to test
   * @return True if condition met
   */
  gcc_pure
  bool operator()(const AbstractAirspace& t) const {
    return condition(t);
  }

  /**
   * Test condition (calls operator() method)
   * @param t Airspace to test
   * @return True if condition met
   */
  gcc_pure
  virtual bool condition(const AbstractAirspace&t) const = 0;

  /** Convenience condition, useful for default conditions */
  static const AirspacePredicateTrue always_true;
};

/**
 * Convenience predicate for conditions always true
 */
class AirspacePredicateTrue: public AirspacePredicate
{
public:
  bool condition(const AbstractAirspace& t) const {
    return true;
  }
};

#endif
