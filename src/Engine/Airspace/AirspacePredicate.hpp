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
  virtual bool operator()(const AbstractAirspace& t) const = 0;

  /**
   * Test condition (calls operator() method)
   * @param t Airspace to test
   * @return True if condition met
   */
  gcc_pure
  bool condition(const AbstractAirspace&t) const {
    return (*this)(t);
  }

  /** Convenience condition, useful for default conditions */
  static const AirspacePredicateTrue always_true;
};

/**
 * Convenience predicate for conditions always true
 */
class AirspacePredicateTrue: public AirspacePredicate
{
public:
  bool operator()(const AbstractAirspace& t) const {
    return true;
  }
};

/**
 * Convenience predicate for conditions always true
 */
class AirspacePredicateAircraftInside: public AirspacePredicate
{
  const AircraftState& m_state;

public:
  /**
   * Constructor
   *
   * @param state State to check interior
   *
   * @return Initialised object
   */
  AirspacePredicateAircraftInside(const AircraftState& state);

  bool operator()(const AbstractAirspace& t) const;
};

/**
 * Convenience predicate for height within a specified range
 */
class AirspacePredicateHeightRange: public AirspacePredicate
{
  const short h_min;
  const short h_max;

public:
  /**
   * Constructor
   *
   * @param _h_min Lower bound on airspace (m)
   * @param _h_max Upper bound on airspace (m)
   *
   * @return Initialised object
   */
  AirspacePredicateHeightRange(const short _h_min, const short _h_max):
    h_min(_h_min), h_max(_h_max) {};

  bool operator()( const AbstractAirspace& t ) const {
    return check_height(t);
  }
protected:
  bool check_height(const AbstractAirspace& t) const;
};

/**
 * Convenience predicate for height within a specified range, excluding
 * airspaces enclosing two points
 */
class AirspacePredicateHeightRangeExcludeTwo: public AirspacePredicateHeightRange
{
  const AGeoPoint p1;
  const AGeoPoint p2;

public:
  /**
   * Constructor
   *
   * @param _h_min Lower bound on airspace (m)
   * @param _h_max Upper bound on airspace (m)
   *
   * @return Initialised object
   */
  AirspacePredicateHeightRangeExcludeTwo(const short _h_min, const short _h_max,
                                         const AGeoPoint& _p1, const AGeoPoint& _p2):
    AirspacePredicateHeightRange(_h_min, _h_max), p1(_p1), p2(_p2) {};

  bool operator()( const AbstractAirspace& t ) const;
};

#endif
