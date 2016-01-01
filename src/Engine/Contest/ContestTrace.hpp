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

#ifndef XCSOAR_CONTEST_TRACE_HPP
#define XCSOAR_CONTEST_TRACE_HPP

#include "Util/TrivialArray.hxx"
#include "Util/TypeTraits.hpp"
#include "Geo/GeoPoint.hpp"

class TracePoint;

/**
 * Similar to TracePoint, but without all the cruft that is not
 * necessary for the contest trace.
 */
struct ContestTracePoint {
  unsigned time;

  GeoPoint location;

  ContestTracePoint() = default;
  ContestTracePoint(const TracePoint &src);

  void Clear() {
    time = (unsigned)(0 - 1);
  }

  bool IsDefined() const {
    return time != (unsigned)(0 - 1);
  }

  unsigned GetTime() const {
    return time;
  }

  bool IsOlderThan(const ContestTracePoint &other) const {
    return time < other.time;
  }

  bool IsNewerThan(const ContestTracePoint &other) const {
    return time > other.time;
  }

  unsigned DeltaTime(const ContestTracePoint &previous) const {
    assert(!IsOlderThan(previous));

    return time - previous.time;
  }

  const GeoPoint &GetLocation() const {
    return location;
  }

  double DistanceTo(const GeoPoint &other) const {
    return location.Distance(other);
  }
};

class ContestTraceVector : public TrivialArray<ContestTracePoint, 10> {};

static_assert(is_trivial_ndebug<ContestTraceVector>::value, "type is not trivial");

#endif
