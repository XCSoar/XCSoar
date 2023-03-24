// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/TypeTraits.hpp"
#include "Geo/SearchPoint.hpp"
#include "Rough/RoughAltitude.hpp"
#include "Rough/RoughVSpeed.hpp"
#include "time/Stamp.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>

struct MoreData;
struct AircraftState;

/**
 * Class for points used in traces (snail trail, contest scans)
 * Internally, keeps track of predecessors as a kind of a linked-list
 */
class TracePoint : public SearchPoint
{
public:
  using Time = std::chrono::duration<unsigned>;

  static constexpr Time INVALID_TIME = Time::max();

private:
  /** Time of sample */
  Time time;

  /**
   * The NavAltitude [m].
   */
  RoughAltitude altitude;

  /**
   * The NettoVario value [m/s].
   */
  RoughVSpeed vario;

  /**
   * The engine noise level (0..999).
   */
  uint16_t engine_noise_level;

  /**
   * Thermal drift factor:
   * 256 indicates drift rate equal to wind speed
   * 0 indicates no drift.
   */
  uint16_t drift_factor;

public:
  /**
   * Non-initialising constructor.
   */
  TracePoint() = default;

  template<typename A, typename V>
  TracePoint(const GeoPoint &location, std::chrono::duration<unsigned> _time,
             const A &_altitude, const V &_vario,
             unsigned _drift_factor)
    :SearchPoint(location), time(_time),
     altitude(_altitude), vario(_vario),
     engine_noise_level(0), drift_factor(_drift_factor) {}

  explicit TracePoint(const MoreData &basic);

  /**
   * Constructor for actual trace points
   *
   * @param state State of aircraft
   * @param tp Projection used internally
   *
   * @return Initialised object
   */
  explicit TracePoint(const AircraftState &state);

  [[gnu::const]]
  static TracePoint Invalid() {
    TracePoint point;
    point.Clear();
    ((SearchPoint &)point).SetInvalid();
    return point;
  }

  void Clear() {
    time = INVALID_TIME;
  }

  bool IsDefined() const {
    return time != INVALID_TIME;
  }

  Time GetTime() const noexcept {
    return time;
  }

  bool IsOlderThan(const TracePoint &other) const {
    return time < other.time;
  }

  bool IsNewerThan(const TracePoint &other) const {
    return time > other.time;
  }

  Time DeltaTime(const TracePoint &previous) const {
    assert(!IsOlderThan(previous));

    return time - previous.time;
  }

  double CalculateDrift(TimeStamp now) const noexcept {
    const double dt = (now.ToDuration() - std::chrono::duration_cast<FloatDuration>(time)).count();
    return dt * drift_factor / 256;
  }

  double GetAltitude() const {
    return altitude;
  }

  unsigned GetEngineNoiseLevel() const {
    return engine_noise_level;
  }

  /**
   * Returns the altitude as an integer.  Some calculations may not
   * need the fractional part.
   */
  int GetIntegerAltitude() const {
    return (int)altitude;
  }

  double GetVario() const {
    return vario;
  }
};

static_assert(is_trivial_ndebug<TracePoint>::value, "type is not trivial");
