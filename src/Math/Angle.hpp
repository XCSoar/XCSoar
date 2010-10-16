/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

class Angle {
private:
  Angle(const fixed& init_value): m_value(init_value) {};

public:
  /**
   * The default constructor does not initialize the value.  It must
   * not be used until it is assigned.
   */
  Angle() {}

  gcc_const
  static Angle native(const fixed value) {
    return Angle(value);
  }

  fixed value_native() const {
    return m_value;
  }

#ifdef RADIANS
  gcc_const
  static Angle degrees(const fixed value) {
    return Angle(value*fixed_deg_to_rad);
  }

  gcc_const
  static Angle radians(const fixed value) {
    return Angle(value);
  }

  fixed value_degrees() const {
    return m_value*fixed_rad_to_deg;
  }
  fixed value_radians() const {
    return m_value;
  }
  fixed value_hours() const {
    return m_value * fixed(24) / fixed_two_pi;
  }
#else
  gcc_const
  static Angle degrees(const fixed value) {
    return Angle(value);
  }

  gcc_const
  static Angle radians(const fixed value) {
    return Angle(value*fixed_rad_to_deg);
  }

  fixed value_degrees() const {
    return m_value;
  }
  fixed value_radians() const {
    return m_value*fixed_deg_to_rad;
  }
  fixed value_hours() const {
    return m_value * fixed(24. / 360.);
  }
#endif

  gcc_const
  static Angle dms(const fixed d, const fixed m, const fixed s) {
    return Angle::degrees(d + m / 60 + s / 3600);
  }

  gcc_pure
  inline fixed tan() const {
    return ::tan(value_radians());
  }

  gcc_pure
  inline fixed sin() const {
    return ::sin(value_radians());
  }

  gcc_pure
  inline fixed cos() const {
    return ::cos(value_radians());
  }

  gcc_pure
  inline fixed fastsine() const {
    return (::fastsine(value_native()));
  }

  gcc_pure
  inline fixed fastcosine() const {
    return (::fastcosine(value_native()));
  }

  gcc_pure
  inline fixed invfastcosine() const {
    return (::invfastcosine(value_native()));
  }

  gcc_pure
  inline int ifastsine() const {
    return (::ifastsine(value_native()));
  }

  gcc_pure
  inline int ifastcosine() const {
    return (::ifastcosine(value_native()));
  }

  gcc_pure
  int sign() const;

  inline void sin_cos(fixed& s, fixed& c) const {
    return ::sin_cos(value_radians(), &s, &c);
  }
  

  gcc_pure
  fixed magnitude_degrees() const;

  gcc_pure
  fixed magnitude_radians() const;

  void flip();

  Angle flipped() const;

  /**
   * Limits the angle (theta) to -180 - +180 degrees
   * @return Output angle (-180 - +180 degrees)
   */
  gcc_pure
  Angle as_delta() const;

  /**
   * Limits the angle (theta) to 0 - 360 degrees
   * @return Output angle (0-360 degrees)
   */
  gcc_pure
  Angle as_bearing() const;

  /**
   * Rotate angle by 180 degrees and limit to 0 - 360 degrees
   * @return Output angle (0 - 360 degrees)
   */
  gcc_pure
  Angle Reciprocal() const;

  gcc_pure
  Angle BiSector(const Angle &OutBound) const;

  gcc_pure
  Angle HalfAngle(const Angle &End) const;

  gcc_pure
  Angle Fraction(const Angle &End, const fixed fraction) const;

  gcc_pure
  Angle
  operator*(const Angle x) const
  {
    return Angle(m_value * x.m_value);
  }

  gcc_pure
  Angle
  operator*(const fixed x) const
  {
    return Angle(m_value * x);
  }

  gcc_pure
  Angle
  operator/(const fixed x) const
  {
    return Angle(m_value / x);
  }

  gcc_pure
  Angle
  operator+(const Angle &x) const
  {
    return Angle(m_value + x.m_value);
  }

  gcc_pure
  Angle
  operator-(const Angle &x) const
  {
    return Angle(m_value - x.m_value);
  }

  gcc_pure
  Angle
  operator-() const
  {
    return Angle(-m_value);
  }

  const Angle&
  operator*=(const fixed x)
  {
    m_value *= x;
    return *this;
  }

  const Angle&
  operator+=(const Angle& x)
  {
    m_value += x.m_value;
    return *this;
  }

  const Angle&
  operator-=(const Angle& x)
  {
    m_value -= x.m_value;
    return *this;
  }

  bool
  operator==(const Angle&x) const
  {
    return m_value == x.m_value;
  }

  bool
  operator!=(const Angle&x) const
  {
    return m_value != x.m_value;
  }

  bool
  operator<(const Angle&x) const
  {
    return m_value < x.m_value;
  }

  bool
  operator>(const Angle&x) const
  {
    return m_value > x.m_value;
  }

  bool
  operator<=(const Angle&x) const
  {
    return m_value <= x.m_value;
  }

  bool
  operator>=(const Angle&x) const
  {
    return m_value >= x.m_value;
  }

  /**
   * Is this angle between the other two values?  If "end" is smaller
   * than "start", then wraparound is calculated correctly.
   */
  gcc_pure
  bool between(const Angle &start, const Angle &end) const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, const Angle& a);
#endif

private:
  fixed m_value;
};

#endif
