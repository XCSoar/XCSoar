/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_KDTREE_MATH_HPP
#define XCSOAR_KDTREE_MATH_HPP

#include "Math/FastMath.h"

#include <math.h>

namespace KDTree
{
  /**
   * This overloads the math.h function, and maps it to isqrt4().
   */
  gcc_const
  static inline unsigned int sqrt(unsigned long val)
  {
    return ::isqrt4(val);
  }

  /**
   * This overloads the math.h function, and maps it to isqrt4().
   */
  gcc_const
  static inline int sqrt(int val)
  {
    return ::isqrt4(val);
  }

  /**
   * Import the math.h function into this namespace.
   */
  gcc_const
  static inline double sqrt(double val)
  {
    return ::sqrt(val);
  }
} // namespace KDTree

#endif // include guard
