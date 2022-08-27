/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
