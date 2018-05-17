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
#ifndef ANGLE_HPP
#define ANGLE_HPP

#include "Trig.hpp"
#include "FastTrig.hpp"
#include "Constants.hpp"
#include "Compiler.h"

#include <type_traits>

#ifdef DO_PRINT
#include <iostream>
#endif

#include <cmath>

class Angle
{
  double value;

  explicit constexpr Angle(const double _value):value(_value) {};

public:
  /**
   * The default constructor does not initialize the value.  It must
   * not be used until it is assigned.
   */
  Angle() = default;

  constexpr
  static Angle Zero() {
    return Native(double(0));
  }

  constexpr
  static Angle Native(const double _value) {
    return Angle(_value);
  }

  /**
   * Construct an instance that describes a "full circle" (360
   * degrees).
   */
  constexpr
  static Angle FullCircle() {
    return Native(M_2PI);
  }

  /**
   * Construct an instance that describes a "half circle" (180
   * degrees).
   */
  constexpr
  static Angle HalfCircle() {
    return Native(M_PI);
  }

  /**
   * Construct an instance that describes a "quarter circle" 90
   * degrees).
   */
  constexpr
  static Angle QuarterCircle() {
    return Native(M_PI_2);
  }

  constexpr double Native() const {
    return value;
  }

  constexpr
  static Angle Degrees(int value) {
    return Angle(value * DEG_TO_RAD);
  }

  constexpr
  static Angle Degrees(unsigned value) {
    return Degrees(int(value));
  }

  constexpr
  static Angle Degrees(double value) {
    return Angle(value * DEG_TO_RAD);
  }

  static constexpr Angle Radians(const double _value) {
    return Angle(_value);
  }

  constexpr double Degrees() const {
    return value * RAD_TO_DEG;
  }

  constexpr double Radians() const {
    return value;
  }

  constexpr double Hours() const {
    return value * 24 / M_2PI;
  }

  struct DMS {
    unsigned degrees, minutes, seconds;
    bool negative;

    DMS() = default;

    constexpr DMS(unsigned d, unsigned m=0, unsigned s=0, bool n=false)
      :degrees(d), minutes(m), seconds(s), negative(n) {}

    constexpr double ToAbsoluteFloat() const {
      return degrees + minutes / 60. + seconds / 3600.;
    }

    constexpr double ToFloat() const {
      return negative ? -ToAbsoluteFloat() : ToAbsoluteFloat();
    }
  };

  constexpr Angle(DMS dms):Angle(Degrees(dms.ToFloat())) {}

  static constexpr Angle FromDMS(unsigned d, unsigned m=0, unsigned s=0,
                                 bool n=false) {
    return DMS(d, m, s, n);
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
  gcc_pure
  DMS ToDMS() const;

  /**
   * Converts this Angle to degrees, minute, decimal minutes and a
   * bool-based east/north variable
   *
   * @param dd Degrees (pointer)
   * @param mm Minutes (pointer)
   * @param mmm Decimal minutes (pointer)
   * @param east True if East, False if West (pointer)
   */
  void ToDMM(unsigned &dd, unsigned &mm, unsigned &mmm,
             bool &is_positive) const;

  gcc_pure
  Angle Absolute() const {
    return Angle(fabs(Native()));
  }

  /**
   * Calculates the tangent of the Angle.
   */
  gcc_pure
  inline double tan() const {
    return ::tan(Radians());
  }

  /**
   * Calculates the sine of the Angle.
   */
  gcc_pure
  inline double sin() const {
    return ::sin(Radians());
  }

  gcc_pure
  inline double accurate_half_sin() const {
    return Half().sin();
  }

  /**
   * Calculates the cosine of the Angle.
   */
  gcc_pure
  inline double cos() const {
    return ::cos(Radians());
  }

  /**
   * Faster but more inaccurate version of sin()
   */
  gcc_pure
  inline double fastsine() const {
    return (::fastsine(Native()));
  }

  /**
   * Faster but more inaccurate version of cos()
   */
  gcc_pure
  inline double fastcosine() const {
    return (::fastcosine(Native()));
  }

  gcc_pure
  inline double invfastcosine() const {
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
  bool IsPositive() const {
    return value > 0;
  }

  gcc_pure
  bool IsNegative() const {
    return std::signbit(value);
  }

  gcc_pure
  std::pair<double, double> SinCos() const {
    return ::sin_cos(Radians());
  }

  gcc_pure
  double AbsoluteDegrees() const;

  gcc_pure
  double AbsoluteRadians() const;

  void Flip() {
    value = -value;
  }

  constexpr
  Angle Flipped() const {
    return Angle(-value);
  }

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
    return Angle(value * 0.5);
  }

  /**
   * Rotate angle by 180 degrees and limit to 0 - 360 degrees
   * @return Output angle (0 - 360 degrees)
   */
  gcc_pure
  Angle Reciprocal() const;

  gcc_pure
  Angle HalfAngle(const Angle end) const;

  /**
   * Computes a certain fraction between the two angles.
   *
   * @param fraction a fraction between 0 and 1
   * @return the resulting Angle, not normalized
   */
  gcc_pure
  Angle Fraction(const Angle end, const double fraction) const;

  gcc_pure Angle
  operator*(const double x) const
  {
    return Angle(value * x);
  }

  constexpr
  Angle
  operator*(const int x) const
  {
    return Angle(value * x);
  }

  constexpr
  Angle
  operator*(const unsigned x) const
  {
    return Angle(value * x);
  }

  gcc_pure
  Angle
  operator/(const double x) const
  {
    return Angle(value / x);
  }

  gcc_pure
  double
  operator/(const Angle x) const
  {
    return value / x.value;
  }

  constexpr
  Angle
  operator/(const int x) const
  {
    return Angle(value / x);
  }

  constexpr
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
  operator*=(const double x)
  {
    value *= x;
    return *this;
  }

  const Angle&
  operator+=(Angle x)
  {
    value += x.value;
    return *this;
  }

  const Angle&
  operator-=(Angle x)
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
   * Return the positive difference between two angles.
   */
  gcc_pure
  Angle fdim(const Angle x) const {
    return Native(std::fdim(value, x.value));
  }

  /**
   * Is this angle between the other two values?  If "end" is smaller
   * than "start", then wraparound is calculated correctly.
   */
  gcc_pure
  bool Between(const Angle start, const Angle end) const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, Angle a);
#endif

  gcc_const
  static Angle asin(double x) {
    return Radians(::asin(x));
  }

  gcc_const
  static Angle acos(double x) {
    return Radians(::acos(x));
  }

  /**
   * Returns the angle based on the input of both a x- and y-coordinate.
   * This is the mathematical angle where zero means along x axis and
   * the positive direction is counter-clockwise!
   * @param x x-coordinate
   * @param y y-coordinate
   * @return Counter-clockwise angle between the x-axis and the given coordinate
   */
  gcc_const
  static Angle FromXY(const double x, const double y) {
    return Angle::Radians(atan2(y,x));
  }

  /**
   * Check whether the two angles are roughly equal.
   */
  gcc_const
  bool CompareRoughly(Angle other, Angle threshold = Angle::Degrees(10)) const;
};

static_assert(std::is_trivial<Angle>::value, "type is not trivial");

#endif
