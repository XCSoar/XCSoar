// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Airspaces.hpp"

#if defined(__GNUC__) && !defined(__clang__)
/* this warning is bogus because GCC is not clever enough to
   understand that NearestAirspace::distance is always valid when also
   NearestAirspace::airspace != nullptr */
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

template<typename Predicate, typename Func,
         typename Result=decltype(((Func *)nullptr)->operator()(ConstAirspacePtr{})),
         class Cmp=std::less<Result>>
[[gnu::pure]]
static inline Result
FindMinimum(const Airspaces &airspaces, const GeoPoint &location, double range,
            Predicate &&predicate,
            Func &&func,
            Cmp &&cmp=Cmp())
{
  Result minimum;
  for (const auto &i : airspaces.QueryWithinRange(location, range)) {
    auto aa = i.GetAirspacePtr();
    if (!predicate(*aa))
      continue;

    Result result = func(std::move(aa));
    if (cmp(result, minimum))
      minimum = result;
  }

  return minimum;
}
