// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cmath>
#include <compare> // for the defaulted spaceship operator

/**
 * The offset between 0 Kelvin and 0 degrees Celsius [K].
 */
static constexpr double CELSIUS_OFFSET(273.15);

/**
 * Convert a temperature from Kelvin to degrees Celsius.
 */
constexpr
static inline double
KelvinToCelsius(double kelvin) noexcept
{
  return kelvin - CELSIUS_OFFSET;
}

/**
 * Convert a temperature from degrees Celsius to Kelvin.
 */
constexpr
static inline double
CelsiusToKelvin(double celsius) noexcept
{
  return celsius + CELSIUS_OFFSET;
}

/**
 * A temperature.  Internally, this is stored as a floating point
 * value in the SI unit "Kelvin".
 */
class Temperature {
  double value;

  explicit constexpr Temperature(double kelvin_value) noexcept
    :value(kelvin_value) {}

public:
  Temperature() = default;

  static constexpr Temperature FromNative(double value) noexcept {
    return Temperature(value);
  }

  static constexpr Temperature FromKelvin(double kelvin_value) noexcept {
    return FromNative(kelvin_value);
  }

  static constexpr Temperature FromCelsius(double celsius_value) noexcept {
    return FromKelvin(CelsiusToKelvin(celsius_value));
  }

  constexpr double ToNative() const noexcept {
    return value;
  }

  constexpr double ToKelvin() const noexcept {
    return ToNative();
  }

  constexpr double ToCelsius() const noexcept {
    return KelvinToCelsius(ToKelvin());
  }

  friend constexpr auto operator<=>(const Temperature &,
                                    const Temperature &) noexcept = default;

  constexpr Temperature operator-() const noexcept {
    return Temperature(-value);
  }

  constexpr Temperature operator-(Temperature other) const noexcept {
    return Temperature(value - other.value);
  }

  Temperature &operator-=(Temperature other) noexcept {
    value -= other.value;
    return *this;
  }

  constexpr Temperature operator+(Temperature other) const noexcept {
    return Temperature(value + other.value);
  }

  Temperature &operator+=(Temperature other) noexcept {
    value += other.value;
    return *this;
  }

  constexpr Temperature operator*(double other) const noexcept {
    return Temperature(value * other);
  }

  Temperature &operator*=(double other) noexcept {
    value *= other;
    return *this;
  }

  constexpr Temperature operator/(double other) const noexcept {
    return Temperature(value / other);
  }

  Temperature &operator/=(double other) noexcept {
    value /= other;
    return *this;
  }

  Temperature Absolute() const noexcept {
    return FromKelvin(std::abs(value));
  }

  static Temperature FromUser(double value) noexcept;
  double ToUser() const noexcept;
};
