// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Stamp.hpp"
#include "RoughTimeDecl.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>

/**
 * Data type to define a time of day with parameterized granularity.
 */
template <typename Duration>
class TimeSinceMidnight {

  static constexpr auto INVALID = Duration::max();
  static constexpr Duration MAX = std::chrono::hours{24};

  /**
   * std::chrono::duration since midnight.  
   * Must be smaller than the equivalet of 24 hours.
   * The only exception is the special value #INVALID.
   */
  Duration value;

  constexpr TimeSinceMidnight(Duration _value) noexcept
    :value(_value) {}

  /**
   * Normalizes any duration since midnight into interval [0, 24h)
   */
  template <typename D>
  static constexpr D NormalizedDuration(D since_midnight) noexcept {
    constexpr D _24H = std::chrono::duration_cast<D>(MAX);
    while (since_midnight < D(0) )
      since_midnight += _24H;

    if constexpr (std::is_floating_point_v<typename D::rep>)
      since_midnight= D( std::fmod(since_midnight.count(), _24H.count()) );
    else
      since_midnight = since_midnight % _24H;

    return since_midnight;
  }

public:
  TimeSinceMidnight() noexcept = default;

  constexpr TimeSinceMidnight(unsigned hour, unsigned minute) noexcept
    :value(std::chrono::duration_cast<Duration>(std::chrono::hours{hour}) +
           std::chrono::duration_cast<Duration>(std::chrono::minutes{minute}))
  {
  }

  static constexpr TimeSinceMidnight FromMinuteOfDayChecked(int mod) noexcept {
    return TimeSinceMidnight( NormalizedDuration(std::chrono::minutes(mod)) );
  }

  explicit constexpr TimeSinceMidnight(TimeStamp t) noexcept 
    :value( std::chrono::duration_cast<Duration>(NormalizedDuration(t.ToDuration())) )
  { 
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
    return std::chrono::duration_cast<std::chrono::minutes>(value).count() % 60;
  }

  constexpr unsigned GetMinuteOfDay() const noexcept {
    return std::chrono::duration_cast<std::chrono::minutes>(value).count();
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

  // Conversions
  friend constexpr inline FineTime ToFineTime(const RoughTime &t) noexcept;
  friend constexpr inline RoughTime ToRoughTime(const FineTime &t) noexcept;
};

constexpr inline FineTime ToFineTime(const RoughTime &t) noexcept {
  return t.IsValid() ? FineTime( t.value ) : FineTime::Invalid();
}

constexpr inline RoughTime ToRoughTime(const FineTime &t) noexcept {
  return t.IsValid() ? RoughTime( std::chrono::duration_cast<UnsignedMinutes>(t.value) ) :
                       RoughTime::Invalid();
}

/**
 * A data type that stores a time span: start and end time of day.
 * This object may be "undefined", i.e. no time span was specified.
 * Either start or end may be "invalid", i.e. there is no limitation on that side.
 */
class TimeSpan {
  FineTime start;

  /**
   * The end of the span (excluding).  This may be bigger than #start
   * if there's a midnight wraparound.
   */
  FineTime end;

public:
  TimeSpan() noexcept = default;

  constexpr TimeSpan(FineTime _start, FineTime _end) noexcept
    :start(_start), end(_end) {}

  static constexpr TimeSpan FromRoughTimes(RoughTime _start, RoughTime _end) noexcept {
    return TimeSpan( ToFineTime(_start), ToFineTime(_end) );
  }

  static constexpr TimeSpan Invalid() noexcept {
    return TimeSpan(FineTime::Invalid(), FineTime::Invalid());
  }

  constexpr FineTime GetStart() const noexcept {
    return start;
  }

  constexpr FineTime GetEnd() const noexcept {
    return end;
  }

  constexpr RoughTime GetRoughStart() const noexcept {
    return ToRoughTime(start);
  }

  constexpr RoughTime GetRoughEnd() const noexcept {
    return ToRoughTime(end);
  }

  constexpr bool IsDefined() const noexcept {
    return start.IsValid() || end.IsValid();
  }

  constexpr bool HasBegun(FineTime now) const noexcept {
    /* if start is invalid, we assume the time span has always already
       begun */
    return !start.IsValid() || now >= start;
  }

  constexpr bool HasEnded(FineTime now) const noexcept {
    /* if end is invalid, the time span is open-ended, i.e. it will
       never end */
    return end.IsValid() && now >= end;
  }

  constexpr bool IsInside(FineTime now) const noexcept {
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

