// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
