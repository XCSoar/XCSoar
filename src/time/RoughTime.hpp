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

#ifndef XCSOAR_ROUGH_TIME_SPAN_HPP
#define XCSOAR_ROUGH_TIME_SPAN_HPP

#include "Stamp.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>

/**
 * This data type stores a time of day with minute granularity.
 */
class RoughTime {
  using Duration = std::chrono::duration<uint16_t,
                                         std::chrono::minutes::period>;

  static constexpr auto INVALID = Duration::max();
  static constexpr Duration MAX = std::chrono::hours{24};

  /**
   * Minute of day.  Must be smaller than 24*60.  The only exception
   * is the special value #INVALID.
   */
  Duration value;

  constexpr RoughTime(Duration _value) noexcept
    :value(_value) {}

public:
  RoughTime() noexcept = default;

  constexpr RoughTime(unsigned hour, unsigned minute) noexcept
    :value(std::chrono::duration_cast<Duration>(std::chrono::hours{hour}) +
           std::chrono::duration_cast<Duration>(std::chrono::minutes{minute}))
  {
  }

  static constexpr RoughTime FromMinuteOfDay(unsigned mod) noexcept {
    assert(std::chrono::minutes{mod} < MAX);

    return RoughTime(std::chrono::minutes{mod});
  }

  /**
   * Construct a #RoughTime object from the specified duration since
   * midnight.
   */
  template<class Rep, class Period>
  static constexpr RoughTime FromSinceMidnight(const std::chrono::duration<Rep,Period> &since_midnight) noexcept {
    return FromMinuteOfDay(std::chrono::duration_cast<std::chrono::minutes>(since_midnight).count());
  }

  static constexpr RoughTime FromMinuteOfDayChecked(int mod) noexcept {
    while (mod < 0)
      mod += MAX.count();

    return FromMinuteOfDayChecked(unsigned(mod));
  }

  /**
   * A wrapper for FromSinceMidnight() which allows values bigger than
   * one day.
   */
  template<class Rep, class Period>
  static constexpr RoughTime FromSinceMidnightChecked(const std::chrono::duration<Rep,Period> &since_midnight) noexcept {
    return FromMinuteOfDayChecked((int)std::chrono::duration_cast<std::chrono::minutes>(since_midnight).count());
  }

  static constexpr RoughTime FromMinuteOfDayChecked(unsigned mod) noexcept {
    return RoughTime(std::chrono::minutes{mod % MAX.count()});
  }

  static constexpr RoughTime FromSecondOfDayChecked(unsigned sod) noexcept {
    return FromMinuteOfDayChecked(sod / 60);
  }

  explicit constexpr RoughTime(TimeStamp t) noexcept
    :RoughTime(FromSinceMidnightChecked(t.ToDuration())) {}

  static constexpr RoughTime Invalid() noexcept {
    return RoughTime(INVALID);
  }

  void SetInvalid() noexcept {
    value = INVALID;
  }

  constexpr bool IsValid() const noexcept {
    return value != INVALID;
  }

  constexpr bool operator ==(RoughTime other) const noexcept {
    return value == other.value;
  }

  constexpr bool operator !=(RoughTime other) const noexcept {
    return value != other.value;
  }

  constexpr bool operator <(RoughTime other) const noexcept {
    /* this formula supports midnight wraparound */
    return (MAX - Duration{1} + other.value - value) % MAX < MAX / 2;
  }

  constexpr bool operator >(RoughTime other) const noexcept {
    return other < *this;
  }

  constexpr bool operator <=(RoughTime other) const noexcept {
    return !(*this > other);
  }

  constexpr bool operator >=(RoughTime other) const noexcept {
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

  RoughTime &operator++() noexcept {
    assert(IsValid());
    assert(value < MAX);

    value = (value + Duration{1}) % MAX;
    return *this;
  }

  RoughTime &operator--() noexcept {
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
  static RoughTimeDelta FromDuration(const std::chrono::duration<Rep,Period> &d) noexcept {
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

#endif
