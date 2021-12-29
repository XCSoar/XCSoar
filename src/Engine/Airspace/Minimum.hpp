/*
Copyright_License {

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

#ifndef AIRSPACE_MINIMUM_HPP
#define AIRSPACE_MINIMUM_HPP

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

#endif
