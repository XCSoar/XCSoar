/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#ifndef ANGLE_HPP
#define ANGLE_HPP

#include "Math/fixed.hpp"
#include "Math/FastMath.h"
#include "Compiler.h"

#ifdef DO_PRINT
#include <iostream>
#endif

class Angle
{
  fixed value;

  gcc_constexpr_ctor
  explicit Angle(const fixed &_value): value(_value) {};

public:
  /**
   * The default constructor does not initialize the value.  It must
   * not be used until it is assigned.
   */
  Angle() {}

  gcc_constexpr_function
  static Angle Native(const fixed _value) {
    return Angle(_value);
  }

  gcc_constexpr_method
  fixed Native() const {
    return value;
  }

  gcc_constexpr_method
  static Angle Zero() {
    return Native(fixed_zero);
  }

#ifdef RADIANS
  gcc_const
  static Angle Degrees(const fixed _value) {
    return Angle(_value * fixed_deg_to_rad);
  }

  gcc_constexpr_function
  static Angle Radians(const fixed _value) {
    return Angle(_value);
  }

  gcc_pure
  fixed Degrees() const {
    return value * fixed_rad_to_deg;
  }

  gcc_constexpr_method
  fixed Radians() const {
    return value;
  }

  gcc_pure
  fixed Hours() const {
    return value * fixed(24) / fixed_two_pi;
  }
#else
  gcc_constexpr_function
  static Angle Degrees(const fixed _value) {
    return Angle(_value);
  }

  gcc_const
  static Angle Radians(const fixed _value) {
    return Angle(_value * fixed_rad_to_deg);
  }

  gcc_constexpr_method
  fixed Degrees() const {
    return value;
  }

  gcc_pure
  fixed Radians() const {
    return value * fixed_deg_to_rad;
  }

  gcc_pure
  fixed Hours() const {
    return value * fixed(24. / 360.);
  }
#endif

  gcc_const
  static Angle DMS(const fixed d, const fixed m, const fixed s) {
    return Angle::Degrees(d + m / 60 + s / 3600);
  }

  gcc_pure
  inline fixed tan() const {
    return ::tan(Radians());
  }

  gcc_pure
  inline fixed sin() const {
    return ::sin(Radians());
  }

  gcc_pure
  inline fixed accurate_half_sin() const {
    return ::accurate_half_sin(Radians());
  }

  gcc_pure
  inline fixed cos() const {
    return ::cos(Radians());
  }

  gcc_pure
  inline fixed fastsine() const {
    return (::fastsine(Native()));
  }

  gcc_pure
  inline fixed fastcosine() const {
    return (::fastcosine(Native()));
  }

  gcc_pure
  inline fixed invfastcosine() const {
    return (::invfastcosine(Native()));
  }

  gcc_pure
  inline int ifastsine() const {
    return (::ifastsine(Native()));
  }

  gcc_pure
  inline int ifastcosine() const {
    return (::ifastcosine(Native()));
  }

  gcc_pure
  int Sign() const;

  gcc_pure
  int Sign(const fixed &tolerance) const;

  inline void SinCos(fixed& s, fixed& c) const {
    ::sin_cos(Radians(), &s, &c);
  }
  

  gcc_pure
  fixed AbsoluteDegrees() const;

  gcc_pure
  fixed AbsoluteRadians() const;

  void Flip();

  Angle Flipped() const;

  /**
   * Limits the angle (theta) to -180 - +180 degrees
   * @return Output angle (-180 - +180 degrees)
   */
  gcc_pure
  Angle AsDelta() const;

  /**
   * Limits the angle (theta) to 0 - 360 degrees
   * @return Output angle (0-360 degrees)
   */
  gcc_pure
  Angle AsBearing() const;

  /**
   * Returns half of this angle.  This is only useful (and valid) when
   * the angle has been normalized with AsDelta().
   */
  gcc_constexpr_method
  Angle Half() const {
    return Angle(::half(value));
  }

  /**
   * Rotate angle by 180 degrees and limit to 0 - 360 degrees
   * @return Output angle (0 - 360 degrees)
   */
  gcc_pure
  Angle Reciprocal() const;

  gcc_pure
  Angle BiSector(const Angle &out_bound) const;

  gcc_pure
  Angle HalfAngle(const Angle &end) const;

  gcc_pure
  Angle Fraction(const Angle &end, const fixed fraction) const;

  gcc_pure
  Angle
  operator*(const Angle x) const
  {
    return Angle(value * x.value);
  }

  gcc_pure
  Angle
  operator*(const fixed x) const
  {
    return Angle(value * x);
  }

  gcc_pure
  Angle
  operator/(const fixed x) const
  {
    return Angle(value / x);
  }

  gcc_pure
  Angle
  operator+(const Angle &x) const
  {
    return Angle(value + x.value);
  }

  gcc_pure
  Angle
  operator-(const Angle &x) const
  {
    return Angle(value - x.value);
  }

  gcc_pure
  Angle
  operator-() const
  {
    return Angle(-value);
  }

  const Angle&
  operator*=(const fixed x)
  {
    value *= x;
    return *this;
  }

  const Angle&
  operator+=(const Angle& x)
  {
    value += x.value;
    return *this;
  }

  const Angle&
  operator-=(const Angle& x)
  {
    value -= x.value;
    return *this;
  }

  gcc_constexpr_method bool
  operator==(const Angle x) const
  {
    return value == x.value;
  }

  gcc_constexpr_method bool
  operator!=(const Angle x) const
  {
    return value != x.value;
  }

  gcc_constexpr_method bool
  operator<(const Angle x) const
  {
    return value < x.value;
  }

  gcc_constexpr_method bool
  operator>(const Angle x) const
  {
    return value > x.value;
  }

  gcc_constexpr_method bool
  operator<=(const Angle x) const
  {
    return value <= x.value;
  }

  gcc_constexpr_method bool
  operator>=(const Angle x) const
  {
    return value >= x.value;
  }

  /**
   * Is this angle between the other two values?  If "end" is smaller
   * than "start", then wraparound is calculated correctly.
   */
  gcc_pure
  bool Between(const Angle &start, const Angle &end) const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, const Angle& a);
#endif

  static Angle FromXY(const fixed& x, const fixed& y) {
    return Angle::Radians(atan2(y,x));
  }
};

#endif
