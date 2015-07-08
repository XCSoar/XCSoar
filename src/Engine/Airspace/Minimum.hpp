/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "AirspaceVisitor.hpp"

template<class Func,
         typename Result=decltype(((Func *)nullptr)->operator()(*(const AbstractAirspace *)nullptr)),
         class Cmp=std::less<Result>>
gcc_pure
static inline Result
FindMinimum(const Airspaces &airspaces, const GeoPoint &location, fixed range,
            const AirspacePredicate &predicate,
            Func &&func,
            Cmp &&cmp=Cmp())
{
  struct FindMinimumVisitor final : AirspaceVisitor, Func, Cmp {
    Result minimum;

    FindMinimumVisitor(Func &&_func, Cmp &&_cmp)
      :Func(std::move(_func)), Cmp(std::move(_cmp)) {}

    void Visit(const AbstractAirspace &aa) override {
      Result result = Func::operator()(aa);
      if (Cmp::operator()(result, minimum))
        minimum = result;
    }
  };

  FindMinimumVisitor visitor(std::move(func), std::move(cmp));
  airspaces.VisitWithinRange(location, range, visitor, predicate);
  return visitor.minimum;
}

#endif
