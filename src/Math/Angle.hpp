/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include <type_traits>

#ifdef DO_PRINT
#include <iosfwd>
#endif

#include <cmath>
#include <compare>

class Angle
{
  double value;

  explicit constexpr Angle(const double _value) noexcept:value(_value) {};

public:
  /**
   * The default constructor does not initialize the value.  It must
   * not be used until it is assigned.
   */
  Angle() noexcept = default;

  static constexpr Angle Zero() noexcept {
    return Native(double(0));
  }

  static constexpr Angle Native(const double _value) noexcept {
    return Angle(_value);
  }

  /**
   * Construct an instance that describes a "full circle" (360
   * degrees).
   */
  static constexpr Angle FullCircle() noexcept {
    return Native(M_2PI);
  }

  /**
   * Construct an instance that describes a "half circle" (180
   * degrees).
   */
  static constexpr Angle HalfCircle() noexcept {
    return Native(M_PI);
  }

  /**
   * Construct an instance that describes a "quarter circle" 90
   * degrees).
   */
  static constexpr Angle QuarterCircle() noexcept {
    return Native(M_PI_2);
  }

  constexpr double Native() const noexcept {
    return value;
  }

  static constexpr Angle Degrees(double value) noexcept {
    return Angle(value * DEG_TO_RAD);
  }

  static constexpr Angle Radians(const double _value) noexcept {
    return Angle(_value);
  }

  constexpr double Degrees() const noexcept {
    return value * RAD_TO_DEG;
  }

  constexpr double Radians() const noexcept {
    return value;
  }

  constexpr double Hours() const noexcept {
    return value * 24 / M_2PI;
  }

  struct DMS {
    unsigned degrees, minutes, seconds;
    bool negative;

    DMS() noexcept = default;

    constexpr DMS(unsigned d, unsigned m=0, unsigned s=0,
                  bool n=false) noexcept
      :degrees(d), minutes(m), seconds(s), negative(n) {}

    constexpr double ToAbsoluteFloat() const noexcept {
      return degrees + minutes / 60. + seconds / 3600.;
    }

    constexpr double ToFloat() const noexcept {
      return negative ? -ToAbsoluteFloat() : ToAbsoluteFloat();
    }
  };

  constexpr Angle(DMS dms) noexcept:Angle(Degrees(dms.ToFloat())) {}

  static constexpr Angle FromDMS(unsigned d, unsigned m=0, unsigned s=0,
                                 bool n=false) noexcept {
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
  [[gnu::pure]]
  DMS ToDMS() const noexcept;

  struct DMM {
    unsigned degrees, minutes, decimal_minutes;

    /**
     * True if East, false if West.
     */
    bool positive;
  };

  /**
   * Converts this Angle to degrees, minute, decimal minutes and a
   * bool-based east/north variable
   */
  [[gnu::pure]]
  DMM ToDMM() const noexcept;

  [[gnu::pure]]
  Angle Absolute() const noexcept {
    return Angle(fabs(Native()));
  }

  /**
   * Calculates the tangent of the Angle.
   */
  [[gnu::pure]]
  inline double tan() const noexcept {
    return ::tan(Radians());
  }

  /**
   * Calculates the sine of the Angle.
   */
  [[gnu::pure]]
  inline double sin() const noexcept {
    return ::sin(Radians());
  }

  [[gnu::pure]]
  inline double accurate_half_sin() const noexcept {
    return Half().sin();
  }

  /**
   * Calculates the cosine of the Angle.
   */
  [[gnu::pure]]
  inline double cos() const noexcept {
    return ::cos(Radians());
  }

  /**
   * Faster but more inaccurate version of sin()
   */
  [[gnu::pure]]
  inline double fastsine() const noexcept {
    return (::fastsine(Native()));
  }

  /**
   * Faster but more inaccurate version of cos()
   */
  [[gnu::pure]]
  inline double fastcosine() const noexcept {
    return (::fastcosine(Native()));
  }

  [[gnu::pure]]
  inline double invfastcosine() const noexcept {
    return (::invfastcosine(Native()));
  }

  /**
   * Returns the sine of the Angle as an integer
   * in the range between -1024 and 1024.
   */
  [[gnu::pure]]
  inline int ifastsine() const noexcept {
    return (::ifastsine(Native()));
  }

  /**
   * Returns the cosine of the Angle as an integer
   * in the range between -1024 and 1024.
   */
  [[gnu::pure]]
  inline int ifastcosine() const noexcept {
    return (::ifastcosine(Native()));
  }

  [[gnu::pure]]
  bool IsPositive() const noexcept {
    return value > 0;
  }

  [[gnu::pure]]
  bool IsNegative() const noexcept {
    return std::signbit(value);
  }

  [[gnu::pure]]
  std::pair<double, double> SinCos() const noexcept {
    return ::sin_cos(Radians());
  }

  [[gnu::pure]]
  double AbsoluteDegrees() const noexcept;

  [[gnu::pure]]
  double AbsoluteRadians() const noexcept;

  void Flip() noexcept {
    value = -value;
  }

  constexpr
  Angle Flipped() const noexcept {
    return Angle(-value);
  }

  /**
   * Limits the angle (theta) to -180 - +180 degrees
   * @return Output angle (-180 - +180 degrees)
   */
  [[gnu::pure]]
  Angle AsDelta() const noexcept;

  /**
   * Limits the angle (theta) to 0 - 360 degrees
   * @return Output angle (0-360 degrees)
   */
  [[gnu::pure]]
  Angle AsBearing() const noexcept;

  /**
   * Returns half of this angle.  This is only useful (and valid) when
   * the angle has been normalized with AsDelta().
   */
  constexpr
  Angle Half() const noexcept {
    return Angle(value * 0.5);
  }

  /**
   * Rotate angle by 180 degrees and limit to 0 - 360 degrees
   * @return Output angle (0 - 360 degrees)
   */
  [[gnu::pure]]
  Angle Reciprocal() const noexcept;

  [[gnu::pure]]
  Angle HalfAngle(const Angle end) const noexcept;

  /**
   * Computes a certain fraction between the two angles.
   *
   * @param fraction a fraction between 0 and 1
   * @return the resulting Angle, not normalized
   */
  [[gnu::pure]]
  Angle Fraction(const Angle end, const double fraction) const noexcept;

  [[gnu::pure]] Angle
  constexpr operator*(const double x) const noexcept {
    return Angle(value * x);
  }

  [[gnu::pure]]
  constexpr Angle operator/(const double x) const noexcept {
    return Angle(value / x);
  }

  [[gnu::pure]]
  constexpr double operator/(const Angle x) const noexcept {
    return value / x.value;
  }

  constexpr Angle operator+(const Angle x) const noexcept {
    return Angle(value + x.value);
  }

  constexpr Angle operator-(const Angle x) const noexcept {
    return Angle(value - x.value);
  }

  constexpr Angle operator-() const noexcept {
    return Angle(-value);
  }

  const Angle &operator*=(const double x) noexcept {
    value *= x;
    return *this;
  }

  const Angle &operator+=(Angle x) noexcept {
    value += x.value;
    return *this;
  }

  const Angle &operator-=(Angle x) noexcept {
    value -= x.value;
    return *this;
  }

  friend constexpr auto operator<=>(const Angle &,
                                    const Angle &) noexcept = default;

  /**
   * Return the positive difference between two angles.
   */
  [[gnu::pure]]
  Angle fdim(const Angle x) const noexcept {
    return Native(std::fdim(value, x.value));
  }

  /**
   * Is this angle between the other two values?  If "end" is smaller
   * than "start", then wraparound is calculated correctly.
   */
  [[gnu::pure]]
  bool Between(const Angle start, const Angle end) const noexcept;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, Angle a);
#endif

  [[gnu::const]]
  static Angle asin(double x) noexcept {
    return Radians(::asin(x));
  }

  [[gnu::const]]
  static Angle acos(double x) noexcept {
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
  [[gnu::const]]
  static Angle FromXY(const double x, const double y) noexcept {
    return Angle::Radians(atan2(y,x));
  }

  /**
   * Check whether the two angles are roughly equal.
   */
  [[gnu::const]]
  bool CompareRoughly(Angle other, Angle threshold = Angle::Degrees(10)) const noexcept;
};

static_assert(std::is_trivial<Angle>::value, "type is not trivial");

#endif
