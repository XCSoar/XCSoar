// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
