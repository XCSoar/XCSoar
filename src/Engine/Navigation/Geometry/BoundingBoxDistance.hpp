/* Copyright_License {

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

#ifndef BOUNDINGBOXDISTANCE_HPP
#define BOUNDINGBOXDISTANCE_HPP

#include "Math/FastMath.h"
#include "Compiler.h"

/**
 * Class used for 2-d integer bounding box distance calculations by kd-tree
 * \todo better documentation for BBDist hack
 */
class BBDist {
public:
  /**
   * Constructor
   *
   * @param _dim Dimension index
   * @param _val Value of distance
   */
  BBDist(const size_t _dim, const int _val)
  {
    set_vals(-1);
    val[_dim % 2] = _val;
    calc_d();
  }

  /**
   * Constructor for set distance
   *
   * @param _val Set distance (typically 0)
   */
  explicit BBDist(const double _val) {
    set_vals(-1);
    d = (int)_val;
  }

  /**
   * Constructor for set distance
   *
   * @param _val Set distance (typically 0)
   */
  explicit BBDist(const int _val) {
    set_vals(-1);
    d = _val;
  }

  /**
   * Add distance measure
   *
   * @param rhs BBDist to add
   *
   * @return Updated distance
   */
  BBDist& operator+=(const BBDist &rhs) {
    set_max(0, rhs);
    set_max(1, rhs);
    calc_d();
    return *this;
  }

  /**
   * Return accumulated distance.
   * Typically this expects all dimensions to be added
   * before calculating the distance.
   *
   * @return Absolute value (accumulated) distance
   */
  gcc_pure
  BBDist sqrt() const {
    return BBDist((int)isqrt4(d));
  }

  bool operator<=(const BBDist &other) const {
    return d <= other.d;
  }

  bool is_zero() const {
    return d == 0;
  }

private:
  void set_max(const size_t _dim, const BBDist &rhs) {
    if (rhs.val[_dim] > val[_dim])
      val[_dim] = rhs.val[_dim];
  }

  void calc_d() {
    d = 0;
    for (unsigned i = 0; i < 2; i++) {
      if (val[i] > 0) {
        d += val[i] * val[i];
      }
    }
  }

  void set_vals(const int _val) {
    val[0] = _val;
    val[1] = _val;
  }

  int val[2];
  int d;  
};

/**
 * Wrapper for BBDist::sqrt() which overloads ::sqrt().
 */
gcc_pure
static inline BBDist
sqrt(const BBDist &d)
{
  return d.sqrt();
}

#endif
