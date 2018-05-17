/*
Copyright_License {

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

#ifndef XCSOAR_ROUGH_TIME_SPAN_HPP
#define XCSOAR_ROUGH_TIME_SPAN_HPP

#include "Compiler.h"

#include <assert.h>
#include <stdint.h>

/**
 * This data type stores a time of day with minute granularity.
 */
class RoughTime {
  static constexpr uint16_t INVALID = -1;
  static constexpr uint16_t MAX = 24 * 60;

  /**
   * Minute of day.  Must be smaller than 24*60.  The only exception
   * is the special value #INVALID.
   */
  uint16_t value;

  constexpr RoughTime(uint16_t _value)
    :value(_value) {}

public:
  RoughTime() = default;

  constexpr RoughTime(unsigned hour, unsigned minute)
    :value(hour * 60 + minute) {}

  gcc_const
  static RoughTime FromMinuteOfDay(unsigned mod) {
    assert(mod < MAX);

    return RoughTime(mod);
  }

  gcc_const
  static RoughTime FromMinuteOfDayChecked(int mod) {
    while (mod < 0)
      mod += MAX;

    return FromMinuteOfDayChecked(unsigned(mod));
  }

  gcc_const
  static RoughTime FromMinuteOfDayChecked(unsigned mod) {
    return RoughTime(mod % MAX);
  }

  gcc_const
  static RoughTime FromSecondOfDayChecked(unsigned sod) {
    return FromMinuteOfDayChecked(sod / 60);
  }

  static constexpr RoughTime Invalid() {
    return RoughTime(INVALID);
  }

  void SetInvalid() {
    value = INVALID;
  }

  constexpr bool IsValid() const {
    return value != INVALID;
  }

  constexpr bool operator ==(RoughTime other) const {
    return value == other.value;
  }

  constexpr bool operator !=(RoughTime other) const {
    return value != other.value;
  }

  constexpr bool operator <(RoughTime other) const {
    /* this formula supports midnight wraparound */
    return (MAX - 1 + other.value - value) % MAX < MAX / 2;
  }

  constexpr bool operator >(RoughTime other) const {
    return other < *this;
  }

  constexpr bool operator <=(RoughTime other) const {
    return !(*this > other);
  }

  constexpr bool operator >=(RoughTime other) const {
    return !(*this < other);
  }

  constexpr unsigned GetHour() const {
    return value / 60;
  }

  constexpr unsigned GetMinute() const {
    return value % 60;
  }

  constexpr unsigned GetMinuteOfDay() const {
    return value;
  }

  RoughTime &operator++() {
    assert(IsValid());
    assert(value < MAX);

    value = (value + 1) % MAX;
    return *this;
  }

  RoughTime &operator--() {
    assert(IsValid());
    assert(value < MAX);

    value = (value + MAX - 1) % MAX;
    return *this;
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
  RoughTimeSpan() = default;

  constexpr RoughTimeSpan(RoughTime _start, RoughTime _end)
    :start(_start), end(_end) {}

  static constexpr RoughTimeSpan Invalid() {
    return RoughTimeSpan(RoughTime::Invalid(), RoughTime::Invalid());
  }

  constexpr const RoughTime &GetStart() const {
    return start;
  }

  constexpr const RoughTime &GetEnd() const {
    return end;
  }

  constexpr bool IsDefined() const {
    return start.IsValid() || end.IsValid();
  }

  constexpr bool HasBegun(RoughTime now) const {
    /* if start is invalid, we assume the time span has always already
       begun */
    return !start.IsValid() || now >= start;
  }

  constexpr bool HasEnded(RoughTime now) const {
    /* if end is invalid, the time span is open-ended, i.e. it will
       never end */
    return end.IsValid() && now >= end;
  }

  constexpr bool IsInside(RoughTime now) const {
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
  int16_t value;

  constexpr RoughTimeDelta(int16_t _value)
    :value(_value) {}

public:
  RoughTimeDelta() = default;

  constexpr
  static RoughTimeDelta FromMinutes(int _value) {
    return RoughTimeDelta(_value);
  }

  constexpr
  static RoughTimeDelta FromSeconds(int _value) {
    return RoughTimeDelta(_value / 60);
  }

  constexpr
  static RoughTimeDelta FromHours(int _value) {
    return RoughTimeDelta(_value * 60);
  }

  constexpr int AsMinutes() const {
    return value;
  }

  constexpr int AsSeconds() const {
    return value * 60;
  }

  constexpr bool operator==(RoughTimeDelta other) const {
    return value == other.value;
  }

  constexpr bool operator!=(RoughTimeDelta other) const {
    return value != other.value;
  }

  constexpr RoughTimeDelta operator-() const {
    return RoughTimeDelta(-value);
  }
};

gcc_const
static inline RoughTime
operator+(RoughTime t, RoughTimeDelta delta)
{
  if (!t.IsValid())
    return t;

  int value = t.GetMinuteOfDay() + delta.AsMinutes();
  return RoughTime::FromMinuteOfDayChecked(value);
}

gcc_const
static inline RoughTime
operator-(RoughTime t, RoughTimeDelta delta)
{
  return t + (-delta);
}

#endif
