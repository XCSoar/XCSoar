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

#ifndef XCSOAR_ATMOSPHERE_TEMPERATURE_HPP
#define XCSOAR_ATMOSPHERE_TEMPERATURE_HPP

#include <cmath>

/**
 * The offset between 0 Kelvin and 0 degrees Celsius [K].
 */
static constexpr double CELSIUS_OFFSET(273.15);

/**
 * Convert a temperature from Kelvin to degrees Celsius.
 */
constexpr
static inline double
KelvinToCelsius(double kelvin)
{
  return kelvin - CELSIUS_OFFSET;
}

/**
 * Convert a temperature from degrees Celsius to Kelvin.
 */
constexpr
static inline double
CelsiusToKelvin(double celsius)
{
  return celsius + CELSIUS_OFFSET;
}

/**
 * A temperature.  Internally, this is stored as a floating point
 * value in the SI unit "Kelvin".
 */
class Temperature {
  double value;

  explicit constexpr Temperature(double kelvin_value):value(kelvin_value) {}

public:
  Temperature() = default;

  static constexpr Temperature FromNative(double value) {
    return Temperature(value);
  }

  static constexpr Temperature FromKelvin(double kelvin_value) {
    return FromNative(kelvin_value);
  }

  static constexpr Temperature FromCelsius(double celsius_value) {
    return FromKelvin(CelsiusToKelvin(celsius_value));
  }

  constexpr double ToNative() const {
    return value;
  }

  constexpr double ToKelvin() const {
    return ToNative();
  }

  constexpr double ToCelsius() const {
    return KelvinToCelsius(ToKelvin());
  }

  constexpr bool operator==(Temperature other) const {
    return value == other.value;
  }

  constexpr bool operator!=(Temperature other) const {
    return value != other.value;
  }

  constexpr bool operator<(Temperature other) const {
    return value < other.value;
  }

  constexpr bool operator<=(Temperature other) const {
    return value <= other.value;
  }

  constexpr bool operator>(Temperature other) const {
    return value > other.value;
  }

  constexpr bool operator>=(Temperature other) const {
    return value >= other.value;
  }

  constexpr Temperature operator-() const {
    return Temperature(-value);
  }

  constexpr Temperature operator-(Temperature other) const {
    return Temperature(value - other.value);
  }

  Temperature &operator-=(Temperature other) {
    value -= other.value;
    return *this;
  }

  constexpr Temperature operator+(Temperature other) const {
    return Temperature(value + other.value);
  }

  Temperature &operator+=(Temperature other) {
    value += other.value;
    return *this;
  }

  constexpr Temperature operator*(double other) const {
    return Temperature(value * other);
  }

  Temperature &operator*=(double other) {
    value *= other;
    return *this;
  }

  constexpr Temperature operator/(double other) const {
    return Temperature(value / other);
  }

  Temperature &operator/=(double other) {
    value /= other;
    return *this;
  }

  Temperature Absolute() const {
    return FromKelvin(std::abs(value));
  }

  static Temperature FromUser(double value);
  double ToUser() const;
};

#endif
