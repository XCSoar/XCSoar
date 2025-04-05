// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Stamp.hpp"
#include "RoughTimeDecl.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>

/**
 * This data type stores a time of day with minute granularity.
 */
class TimeSinceMidnight {
  using Duration = std::chrono::duration<uint16_t,
                                         std::chrono::minutes::period>;

  static constexpr auto INVALID = Duration::max();
  static constexpr Duration MAX = std::chrono::hours{24};

  /**
   * Minute of day.  Must be smaller than 24*60.  The only exception
   * is the special value #INVALID.
   */
  Duration value;

  constexpr TimeSinceMidnight(Duration _value) noexcept
    :value(_value) {}

public:
  TimeSinceMidnight() noexcept = default;

  constexpr TimeSinceMidnight(unsigned hour, unsigned minute) noexcept
    :value(std::chrono::duration_cast<Duration>(std::chrono::hours{hour}) +
           std::chrono::duration_cast<Duration>(std::chrono::minutes{minute}))
  {
  }

  static constexpr TimeSinceMidnight FromMinuteOfDayChecked(int mod) noexcept {
    while (mod < 0)
      mod += MAX.count();

    return TimeSinceMidnight(std::chrono::minutes{(unsigned)mod % MAX.count()});
  }

  explicit constexpr TimeSinceMidnight(TimeStamp t) noexcept {
    constexpr auto MAX_FLOAT = std::chrono::duration_cast<FloatDuration>(MAX);
    
    FloatDuration since_midnight = t.ToDuration();
    while (since_midnight < FloatDuration(0))
      since_midnight += MAX_FLOAT;
    since_midnight = FloatDuration(std::fmod(since_midnight.count(), MAX_FLOAT.count()));

    value = std::chrono::duration_cast<Duration>(since_midnight);
  }

  static constexpr TimeSinceMidnight Invalid() noexcept {
    return TimeSinceMidnight(INVALID);
  }

  void SetInvalid() noexcept {
    value = INVALID;
  }

  constexpr bool IsValid() const noexcept {
    return value != INVALID;
  }

  constexpr bool operator ==(TimeSinceMidnight other) const noexcept {
    return value == other.value;
  }

  constexpr bool operator !=(TimeSinceMidnight other) const noexcept {
    return value != other.value;
  }

  constexpr bool operator <(TimeSinceMidnight other) const noexcept {
    /* this formula supports midnight wraparound */
    return (MAX - Duration{1} + other.value - value) % MAX < MAX / 2;
  }

  constexpr bool operator >(TimeSinceMidnight other) const noexcept {
    return other < *this;
  }

  constexpr bool operator <=(TimeSinceMidnight other) const noexcept {
    return !(*this > other);
  }

  constexpr bool operator >=(TimeSinceMidnight other) const noexcept {
    return !(*this < other);
  }

  constexpr unsigned GetHour() const noexcept {
    return std::chrono::duration_cast<std::chrono::hours>(value).count();
  }

  constexpr unsigned GetMinute() const noexcept {
    return value.count() % 60;
  }

  constexpr unsigned GetMinuteOfDay() const noexcept {
    return value.count();
  }

  TimeSinceMidnight &operator++() noexcept {
    assert(IsValid());
    assert(value < MAX);

    value = (value + Duration{1}) % MAX;
    return *this;
  }

  TimeSinceMidnight &operator--() noexcept {
    assert(IsValid());
    assert(value < MAX);

    value = (value + MAX - Duration{1}) % MAX;
    return *this;
  }

  constexpr operator TimeStamp() const noexcept {
    return TimeStamp{value};
  }
};

/**
 * A data type that stores a time span: start end end time of day with
 * minute granularity.  This object may be "undefined", i.e. no time
 * span was specified.  Either start or end may be "invalid",
 * i.e. there is no limitation on that side.
 */
class RoughTimeSpan {
  RoughTime start;

  /**
   * The end of the span (excluding).  This may be bigger than #start
   * if there's a midnight wraparound.
   */
  RoughTime end;

public:
  RoughTimeSpan() noexcept = default;

  constexpr RoughTimeSpan(RoughTime _start, RoughTime _end) noexcept
    :start(_start), end(_end) {}

  static constexpr RoughTimeSpan Invalid() noexcept {
    return RoughTimeSpan(RoughTime::Invalid(), RoughTime::Invalid());
  }

  constexpr const RoughTime &GetStart() const noexcept {
    return start;
  }

  constexpr const RoughTime &GetEnd() const noexcept {
    return end;
  }

  constexpr bool IsDefined() const noexcept {
    return start.IsValid() || end.IsValid();
  }

  constexpr bool HasBegun(RoughTime now) const noexcept {
    /* if start is invalid, we assume the time span has always already
       begun */
    return !start.IsValid() || now >= start;
  }

  constexpr bool HasEnded(RoughTime now) const noexcept {
    /* if end is invalid, the time span is open-ended, i.e. it will
       never end */
    return end.IsValid() && now >= end;
  }

  constexpr bool IsInside(RoughTime now) const noexcept {
    return HasBegun(now) && !HasEnded(now);
  }
};

/**
 * This data type stores a (signed) time difference with minute
 * granularity.  It can be used to store time zone offsets.
 */
class RoughTimeDelta {
  /**
   * Relative minutes.
   */
  using Duration = std::chrono::duration<int16_t,
                                         std::chrono::minutes::period>;

  Duration value;

  constexpr RoughTimeDelta(Duration _value) noexcept
    :value(_value) {}

public:
  RoughTimeDelta() noexcept = default;

  constexpr
  static RoughTimeDelta FromMinutes(int _value) noexcept {
    return RoughTimeDelta(Duration{_value});
  }

  constexpr
  static RoughTimeDelta FromSeconds(int _value) noexcept {
    return RoughTimeDelta(Duration{_value / 60});
  }

  constexpr
  static RoughTimeDelta FromHours(int _value) noexcept {
    return RoughTimeDelta(Duration{_value * 60});
  }

  template<class Rep, class Period>
  [[gnu::const]]
  static RoughTimeDelta FromDuration(const std::chrono::duration<Rep,Period> d) noexcept {
    return RoughTimeDelta{std::chrono::duration_cast<Duration>(d)};
  }

  constexpr std::chrono::minutes ToDuration() const noexcept {
    return value;
  }

  constexpr int AsMinutes() const noexcept {
    return value.count();
  }

  constexpr int AsSeconds() const noexcept {
    return value.count() * 60;
  }

  constexpr bool operator==(RoughTimeDelta other) const noexcept {
    return value == other.value;
  }

  constexpr bool operator!=(RoughTimeDelta other) const noexcept {
    return value != other.value;
  }

  constexpr RoughTimeDelta operator-() const noexcept {
    return RoughTimeDelta(-value);
  }
};

static constexpr RoughTime
operator+(RoughTime t, RoughTimeDelta delta) noexcept
{
  if (!t.IsValid())
    return t;

  int value = t.GetMinuteOfDay() + delta.AsMinutes();
  return RoughTime::FromMinuteOfDayChecked(value);
}

static constexpr RoughTime
operator-(RoughTime t, RoughTimeDelta delta) noexcept
{
  return t + (-delta);
}
