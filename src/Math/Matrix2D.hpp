/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#ifndef MATRIX_2D_HPP
#define MATRIX_2D_HPP

#include "Math/Angle.hpp"
#include "Screen/Point.hpp"
#include "Topography/XShapePoint.hpp"

#ifdef HAVE_BOOST
#include <boost/cstdint.hpp>
#else
#include <stdint.h>
#endif

/**
 * This is an equivalent to OpenGL ES 32-bit fixed point matrix math, for the
 * 2D case only. Functions ending with x expect fixed point parameters.
 */
class Matrix2D {
#ifdef HAVE_BOOST
  typedef boost::int64_t int64_t;
#endif

  int32_t m[3][2];
public:
  Matrix2D() {
    // Identity
    m[0][0] = m[1][1] = 1<<16;
    m[1][0] = m[2][0] = 0;
    m[0][1] = m[2][1] = 0;
  }
  Matrix2D(const Matrix2D &other) {
    m[0][0] = other.m[0][0];  m[0][1] = other.m[0][1];
    m[1][0] = other.m[1][0];  m[1][1] = other.m[1][1];
    m[2][0] = other.m[2][0];  m[2][1] = other.m[2][1];
  }

  void Scale(const fixed &factor) {
#ifdef FIXED_MATH
    Scalex(factor.as_glfixed_scale());
#else
    Scalex(factor * (1LL<<32));
#endif
  }

  void Scalex(long factor) {
    m[0][0] = mul(factor, m[0][0]);
    m[0][1] = mul(factor, m[0][1]);
    m[1][0] = mul(factor, m[1][0]);
    m[1][1] = mul(factor, m[1][1]);
  }

  void Translate(RasterPoint pt) {
    Translatex((int)pt.x << 16, (int)pt.y << 16);
  }

  void Translatex(ShapePoint pt) {
    Translatex(pt.x, pt.y);
  }

  void Translatex(long x, long y) {
    m[2][0] += mul(x, m[0][0]) + mul(y, m[1][0]);
    m[2][1] += mul(x, m[0][1]) + mul(y, m[1][1]);
  }

  void Rotate(const Angle &alpha) {
    fixed sin, cos;
    alpha.SinCos(sin, cos);
#ifdef FIXED_MATH
    long s = sin.as_glfixed();
    long c = cos.as_glfixed();
#else
    long s = sin * (1<<16);
    long c = cos * (1<<16);
#endif
    Rotatex(s, c);
  }

  void Rotatex(long s, long c) {
    long tmp;
    tmp = mul(s, m[0][0]) + mul(c, m[1][0]);
    m[0][0] = mul(c, m[0][0]) - mul(s, m[1][0]);
    m[1][0] = tmp;
    tmp = mul(s, m[0][1]) + mul(c, m[1][1]);
    m[0][1] = mul(c, m[0][1]) - mul(s, m[1][1]);
    m[1][1] = tmp;
  }

  RasterPoint Apply(const ShapePoint &sp) const {
    RasterPoint rp;
    rp.x = (mul(sp.x, m[0][0]) + mul(sp.y, m[1][0]) + m[2][0]) >> 16;
    rp.y = (mul(sp.x, m[1][0]) + mul(sp.y, m[1][1]) + m[2][1]) >> 16;
    return rp;
  }

protected:
  long mul(long a, long b) const {
    return ((int64_t)a * b) >> 16;
  }
};

#endif
