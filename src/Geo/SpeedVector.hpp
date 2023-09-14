// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"

/**
 * An object describing the speed vector in a two dimensional surface.
 */
struct SpeedVector {
  /**
   * The direction of the vector.
   */
  Angle bearing;

  /**
   * The norm of the vector [m/s].
   */
  double norm;

  /** 
   * Non-initialising default constructor.
   */
  SpeedVector() noexcept = default;

  /** 
   * Constructor given bearing and magnitude
   * 
   * @param _bearing the direction of vector
   * @param _norm Magnitude of vector (m/s)
   * @return Initialised object
   */
  constexpr SpeedVector(Angle _bearing, double _norm) noexcept
    :bearing(_bearing), norm(_norm) {}

  /** 
   * Constructor given two magnitudes (east and north)
   * 
   * @param x East speed
   * @param y North speed
   * @return Initialised object
   */
  SpeedVector(double x, double y) noexcept
    :bearing(Angle::FromXY(y,x).AsBearing()), norm(hypot(x, y)) {}

  /**
   * Returns the null vector.
   */
  static constexpr SpeedVector Zero() noexcept {
    return SpeedVector(Angle::Zero(), 0);
  }

  /**
   * Returns true if the norm of the vector is zero.
   */
  constexpr bool IsZero() const noexcept {
    return !IsNonZero();
  }

  /**
   * Returns true if the norm of the vector is non-zero.
   */
  constexpr bool IsNonZero() const noexcept {
    return norm > 0;
  }

  /**
   * Return the vector with the bearing rotated by 180 degrees.
   */
  [[gnu::pure]]
  SpeedVector Reciprocal() const noexcept {
    return SpeedVector(bearing.Reciprocal(), norm);
  }
};

static_assert(std::is_trivial<SpeedVector>::value, "type is not trivial");
