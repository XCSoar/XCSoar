// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Point.hpp"
#include "time/Stamp.hpp"

/**
 * The wind drift of a #TracePoint as a linear function of the
 * current time: TracePoint::CalculateDrift() is
 * "now * rate - offset".  A weighted blend of these terms is the
 * same blend of the points' drift at any time, so a curve
 * interpolated between trace points can be drifted later without
 * storing a time for each of its points.
 */
struct TraceDriftTerms {
  /** Sample time [s] multiplied by #rate. */
  double offset{};
  /** Fraction of the wind speed the sample drifts with. */
  double rate{};

  static constexpr TraceDriftTerms
  FromTracePoint(const TracePoint &tp) noexcept {
    const double rate = tp.GetDriftFactor() / 256.;
    return {tp.GetTime().count() * rate, rate};
  }

  /** The drift parameter at the given time. */
  constexpr double At(TimeStamp now) const noexcept {
    return now.ToDuration().count() * rate - offset;
  }

  constexpr TraceDriftTerms
  operator+(const TraceDriftTerms &other) const noexcept {
    return {offset + other.offset, rate + other.rate};
  }

  constexpr TraceDriftTerms operator*(double weight) const noexcept {
    return {offset * weight, rate * weight};
  }
};
