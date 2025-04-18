// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/FloatDuration.hxx"

#include <algorithm>
#include <cassert>

/**
 * Track how much time the aircraft was in a certain state
 * (e.g. "moving").  It provides tools to decrement the measured
 * duration while the aircraft was not in that state.
 *
 * @param max_value the maximum value; at this peak, the countdown
 * will start, and after this duration, the state will be cleared
 * @param max_delta the maximum delta accepted by Add() and
 * Subtract(), to avoid hiccups due to temporary GPS outage
 */
template<unsigned max_value, unsigned max_delta>
class StateClock {
  FloatDuration value;

public:
  void Clear() {
    value = {};
  }

  bool IsDefined() const {
    assert(value.count() >= 0);

    return value.count() > 0;
  }

private:
  static constexpr FloatDuration LimitDelta(FloatDuration delta) noexcept {
    assert(delta.count() >= 0);

    return std::min(delta, FloatDuration{max_delta});
  }

public:
  void Add(FloatDuration delta) noexcept {
    assert(value.count() >= 0);
    assert(value <= FloatDuration{max_value});

    value += LimitDelta(delta);
    if (value > FloatDuration{max_value})
      value = FloatDuration{max_value};
  }

  void Subtract(FloatDuration delta) noexcept {
    assert(value.count() >= 0);
    assert(value <= FloatDuration(max_value));

    value -= LimitDelta(delta);
    if (value.count() < 0)
      value = {};
  }

  constexpr bool operator>=(FloatDuration other) const noexcept {
    assert(other.count() > 0);
    assert(value.count() >= 0);
    assert(value <= FloatDuration{max_value});

    return value >= other;
  }
};
