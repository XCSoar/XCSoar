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

#ifndef AIRSPACE_PREDICATE_HPP
#define AIRSPACE_PREDICATE_HPP

#include "Compiler.h"

#include <utility>

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

  /** Convenience condition, useful for default conditions */
  static const AirspacePredicateTrue always_true;
};

/**
 * Convenience predicate for conditions always true
 */
class AirspacePredicateTrue final : public AirspacePredicate {
public:
  bool operator()(gcc_unused const AbstractAirspace& t) const override {
    return true;
  }
};

/**
 * A template class that wraps a generic C++ object into an
 * #AirspacePredicate.
 */
template<typename P>
class WrappedAirspacePredicate final : public AirspacePredicate, private P {
public:
  template<typename... Args>
  WrappedAirspacePredicate(Args&&... args):P(args...) {}

  bool operator()(const AbstractAirspace& t) const override {
    return static_cast<const P &>(*this)(t);
  }
};

template<typename P>
static inline WrappedAirspacePredicate<P>
WrapAirspacePredicate(const P &p)
{
  return WrappedAirspacePredicate<P>(p);
}

class AirspacePredicateRef {
  const AirspacePredicate &p;

public:
  explicit AirspacePredicateRef(const AirspacePredicate &_p):p(_p) {}

  bool operator()(const AbstractAirspace &t) const {
    return p(t);
  }
};

/**
 * A class that combines two unary functions with logical "and".
 */
template<typename A, typename B>
class AndPredicate : A, B {
public:
  template<typename A_, typename B_>
  constexpr AndPredicate(A_ &&a, B_ &&b)
    :A(std::forward<A_>(a)), B(std::forward<B_>(b)) {}

  template<typename T>
  gcc_pure
  bool operator()(const T &t) const {
    return A::operator()(t) && B::operator()(t);
  }
};

template<typename A, typename B>
static inline AndPredicate<A, B>
MakeAndPredicate(const A &a, const B &b)
{
  return AndPredicate<A, B>(a, b);
}

#endif
