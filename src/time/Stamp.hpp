// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FloatDuration.hxx"

struct NMEAInfo;

/**
 * This class describes a time point during a flight.  It is used for
 * XCSoar's calculations.  Internally, it is the duration since
 * midnight of the day the flight started.
 */
class TimeStamp {
  FloatDuration value;

public:
  TimeStamp() noexcept = default;

  explicit constexpr TimeStamp(FloatDuration _value) noexcept
    :value(_value) {}

  static constexpr TimeStamp Undefined() noexcept {
    return TimeStamp{FloatDuration{-1}};
  }

  constexpr bool IsDefined() const noexcept {
    return value.count() >= 0;
  }

  constexpr FloatDuration ToDuration() const noexcept {
    return value;
  }

  template<typename T>
  constexpr auto Cast() const noexcept {
    return std::chrono::duration_cast<T>(value);
  }

  constexpr bool operator==(TimeStamp other) const noexcept {
    return value == other.value;
  }

  constexpr bool operator!=(TimeStamp other) const noexcept {
    return value != other.value;
  }

  constexpr bool operator<(TimeStamp other) const noexcept {
    return value < other.value;
  }

  constexpr bool operator<=(TimeStamp other) const noexcept {
    return value <= other.value;
  }

  constexpr bool operator>(TimeStamp other) const noexcept {
    return value > other.value;
  }

  constexpr bool operator>=(TimeStamp other) const noexcept {
    return value >= other.value;
  }

  constexpr FloatDuration operator-(TimeStamp other) const noexcept {
    return value - other.value;
  }

  constexpr auto operator+(FloatDuration delta) const noexcept {
    return TimeStamp{value + delta};
  }

  constexpr auto operator-(FloatDuration delta) const noexcept {
    return TimeStamp{value - delta};
  }

  auto &operator+=(FloatDuration delta) noexcept {
    value += delta;
    return *this;
  }
};

static inline FloatDuration
fdim(TimeStamp a, TimeStamp b) noexcept
{
  return fdim(a.ToDuration(), b.ToDuration());
}
