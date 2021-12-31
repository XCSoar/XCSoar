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

#ifndef XCSOAR_CONTEST_TRACE_HPP
#define XCSOAR_CONTEST_TRACE_HPP

#include "util/TrivialArray.hxx"
#include "Geo/GeoPoint.hpp"

#include <type_traits>

class TracePoint;

/**
 * Similar to TracePoint, but without all the cruft that is not
 * necessary for the contest trace.
 */
struct ContestTracePoint {
  using Duration = std::chrono::duration<unsigned>;
  Duration time;

  GeoPoint location;

  ContestTracePoint() = default;
  ContestTracePoint(const TracePoint &src) noexcept;

  void Clear() noexcept {
    time = Duration::max();
  }

  constexpr bool IsDefined() const noexcept {
    return time != Duration::max();
  }

  constexpr Duration GetTime() const noexcept {
    return time;
  }

  constexpr bool IsOlderThan(const ContestTracePoint &other) const noexcept {
    return time < other.time;
  }

  constexpr bool IsNewerThan(const ContestTracePoint &other) const noexcept {
    return time > other.time;
  }

  constexpr Duration DeltaTime(const ContestTracePoint &previous) const noexcept {
    assert(!IsOlderThan(previous));

    return time - previous.time;
  }

  constexpr const GeoPoint &GetLocation() const noexcept {
    return location;
  }

  [[gnu::pure]]
  double DistanceTo(const GeoPoint &other) const noexcept {
    return location.Distance(other);
  }
};

class ContestTraceVector : public TrivialArray<ContestTracePoint, 10> {};

static_assert(std::is_trivial_v<ContestTraceVector>, "type is not trivial");

#endif
