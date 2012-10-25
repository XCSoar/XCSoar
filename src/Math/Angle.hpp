/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Util/TypeTraits.hpp"
#include "Math/fixed.hpp"
#include "Math/FastMath.h"
#include "Compiler.h"

#ifdef DO_PRINT
#include <iostream>
#endif

class Angle
{
  fixed value;

  constexpr
  explicit Angle(const fixed &_value): value(_value) {};

public:
  /**
   * The default constructor does not initialize the value.  It must
   * not be used until it is assigned.
   */
  Angle() = default;

  constexpr
  static Angle Zero() {
    return Native(fixed_zero);
  }

  constexpr
  static Angle Native(const fixed _value) {
    return Angle(_value);
  }

  /**
   * Construct an instance that describes a "full circle" (360
   * degrees).
   */
  constexpr
  static Angle FullCircle() {
#ifdef RADIANS
    return Native(fixed_two_pi);
#else
    return Native(fixed_360);
#endif
  }

  /**
   * Construct an instance that describes a "half circle" (180
   * degrees).
   */
  constexpr
  static Angle HalfCircle() {
#ifdef RADIANS
    return Native(fixed_pi);
#else
    return Native(fixed_180);
#endif
  }

  /**
   * Construct an instance that describes a "quarter circle" 90
   * degrees).
   */
  constexpr
  static Angle QuarterCircle() {
#ifdef RADIANS
    return Native(fixed_half_pi);
#else
    return Native(fixed_90);
#endif
  }

  constexpr
  fixed Native() const {
    return value;
  }

#ifdef RADIANS
  constexpr
  static Angle Degrees(int value) {
    return Angle(fixed(value * DEG_TO_RAD));
  }

  constexpr
  static Angle Degrees(double value) {
    return Angle(fixed(value * DEG_TO_RAD));
  }

#ifdef FIXED_MATH
  gcc_const
  static Angle Degrees(const fixed _value) {
    return Angle(_value * fixed_deg_to_rad);
  }
#endif

  constexpr
  static Angle Radians(const fixed _value) {
    return Angle(_value);
  }

  gcc_pure
  fixed Degrees() const {
    return value * fixed_rad_to_deg;
  }

  constexpr
  fixed Radians() const {
    return value;
  }

  gcc_pure
  fixed Hours() const {
    return value * fixed(24) / fixed_two_pi;
  }
#else
  constexpr
  static Angle Degrees(const fixed _value) {
    return Angle(_value);
  }

  constexpr
  static Angle Radians(int value) {
    return Angle(fixed(value * RAD_TO_DEG));
  }

  constexpr
  static Angle Radians(double value) {
    return Angle(fixed(value * RAD_TO_DEG));
  }

#ifdef FIXED_MATH
  gcc_const
  static Angle Radians(const fixed _value) {
    return Angle(_value * fixed_rad_to_deg);
  }
#endif

  constexpr
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

  /**
   * Converts this Angle to degrees, minute, seconds and a
   * bool-based east/north variable
   *
   * @param dd Degrees (pointer)
   * @param mm Minutes (pointer)
   * @param ss Seconds (pointer)
   * @param east True if East, False if West (pointer)
   */
  void ToDMS(int &dd, int &mm, int &ss, bool &is_positive) const;

  gcc_pure
  Angle Absolute() const {
    return Angle(fabs(Native()));
  }

  /**
   * Calculates the tangent of the Angle.
   */
  gcc_pure
  inline fixed tan() const {
    return ::tan(Radians());
  }

  /**
   * Calculates the sine of the Angle.
   */
  gcc_pure
  inline fixed sin() const {
    return ::sin(Radians());
  }

  gcc_pure
  inline fixed accurate_half_sin() const {
    return ::accurate_half_sin(Radians());
  }

  /**
   * Calculates the cosine of the Angle.
   */
  gcc_pure
  inline fixed cos() const {
    return ::cos(Radians());
  }

  /**
   * Faster but more inaccurate version of sin()
   */
  gcc_pure
  inline fixed fastsine() const {
    return (::fastsine(Native()));
  }

  /**
   * Faster but more inaccurate version of cos()
   */
  gcc_pure
  inline fixed fastcosine() const {
    return (::fastcosine(Native()));
  }

  gcc_pure
  inline fixed invfastcosine() const {
    return (::invfastcosine(Native()));
  }

  /**
   * Returns the sine of the Angle as an integer
   * in the range between -1024 and 1024.
   */
  gcc_pure
  inline int ifastsine() const {
    return (::ifastsine(Native()));
  }

  /**
   * Returns the cosine of the Angle as an integer
   * in the range between -1024 and 1024.
   */
  gcc_pure
  inline int ifastcosine() const {
    return (::ifastcosine(Native()));
  }

  gcc_pure
  int Sign() const;

  gcc_pure
  int Sign(const fixed &tolerance) const;

  gcc_pure
  std::pair<fixed, fixed> SinCos() const {
    return ::sin_cos(Radians());
  }

  gcc_pure
  fixed AbsoluteDegrees() const;

  gcc_pure
  fixed AbsoluteRadians() const;

  void Flip();

  gcc_pure
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
  constexpr
  Angle Half() const {
    return Angle(::Half(value));
  }

  /**
   * Rotate angle by 180 degrees and limit to 0 - 360 degrees
   * @return Output angle (0 - 360 degrees)
   */
  gcc_pure
  Angle Reciprocal() const;

  gcc_pure
  Angle HalfAngle(const Angle end) const;

  gcc_pure
  Angle Fraction(const Angle end, const fixed fraction) const;

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
  operator*(const int x) const
  {
    return Angle(value * x);
  }

  gcc_pure
  Angle
  operator*(const unsigned x) const
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
  operator/(const int x) const
  {
    return Angle(value / x);
  }

  gcc_pure
  Angle
  operator/(const unsigned x) const
  {
    return Angle(value / x);
  }

  constexpr
  Angle
  operator+(const Angle x) const
  {
    return Angle(value + x.value);
  }

  constexpr
  Angle
  operator-(const Angle x) const
  {
    return Angle(value - x.value);
  }

  constexpr
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

  constexpr bool
  operator==(const Angle x) const
  {
    return value == x.value;
  }

  constexpr bool
  operator!=(const Angle x) const
  {
    return value != x.value;
  }

  constexpr bool
  operator<(const Angle x) const
  {
    return value < x.value;
  }

  constexpr bool
  operator>(const Angle x) const
  {
    return value > x.value;
  }

  constexpr bool
  operator<=(const Angle x) const
  {
    return value <= x.value;
  }

  constexpr bool
  operator>=(const Angle x) const
  {
    return value >= x.value;
  }

  /**
   * Is this angle between the other two values?  If "end" is smaller
   * than "start", then wraparound is calculated correctly.
   */
  gcc_pure
  bool Between(const Angle start, const Angle end) const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, const Angle& a);
#endif

  /**
   * Returns the angle based on the input of both a x- and y-coordinate.
   * This is the mathematical angle where zero means along x axis and
   * the positive direction is counter-clockwise!
   * @param x x-coordinate
   * @param y y-coordinate
   * @return Counter-clockwise angle between the x-axis and the given coordinate
   */
  static Angle FromXY(const fixed& x, const fixed& y) {
    return Angle::Radians(atan2(y,x));
  }
};

static_assert(is_trivial<Angle>::value, "type is not trivial");

#endif
