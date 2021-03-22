/* Copyright_License {

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

#ifndef AIRSPACE_PREDICATE_HPP
#define AIRSPACE_PREDICATE_HPP

#include <functional>
#include <utility>

class AbstractAirspace;

using AirspacePredicate = std::function<bool(const AbstractAirspace &)>;

/**
 * Convenience predicate for conditions always true
 */
constexpr bool
AirspacePredicateTrue(const AbstractAirspace &) noexcept
{
  return true;
}

template<typename P>
static inline AirspacePredicate
WrapAirspacePredicate(P &&p)
{
  return [q = std::forward<P>(p)](const auto &a){
    return q(a);
  };
}

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
  [[gnu::pure]]
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
