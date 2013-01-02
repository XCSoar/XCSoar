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

// Written by Curtis Olson, started September 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.

/**
 * Implements a simple linear least squares best fit routine.
 * @file leastsqs.h
 */

#ifndef _LEASTSQS_H
#define _LEASTSQS_H

#include "Util/StaticArray.hpp"
#include "Math/fixed.hpp"

/**
 * A solver for least squares problems
 *
 * Classical least squares fit:
 *
 * \f[
 *     y = m * x + b
 * \f]
 *
 * the least squares error:
 *
 * \f[
 *     \frac{(y_i - \hat{y}_i)^2}{n}
 * \f]
 *
 * the maximum least squares error:
 *
 * \f[
 *     (y_i - \hat{y}_i)^2
 * \f]
 */
class LeastSquares
{
public:
  fixed sum_xi, sum_yi, sum_xi_2, sum_xi_yi;

  unsigned sum_n;

  /**
  * \f[
  *     m = \frac{n * \sum_0^{i-1} (x_i*y_i) - \sum_0^{i-1} x_i* \sum_0^{i-1} y_i}
  *              {n*\sum_0^{i-1} x_i^2 - (\sum_0^{i-1} x_i)^2}
  * \f]
  */
  fixed m;
  /**
  * \f[
  *     b = \frac{\sum_0^{i-1} y_i}{n} - b_1 * \frac{\sum_0^{i-1} x_i}{n}
  * \f]
  */
  fixed b;
  fixed sum_error;

  fixed rms_error;
  fixed max_error;
  fixed sum_weights;
  fixed y_max;
  fixed y_min;
  fixed x_min;
  fixed x_max;

  fixed y_ave;

  struct Slot {
    fixed x, y;

#ifdef LEASTSQS_WEIGHT_STORE
    fixed weight;
#endif

    Slot() = default;

    constexpr
    Slot(fixed _x, fixed _y, fixed _weight)
      :x(_x), y(_y)
#ifdef LEASTSQS_WEIGHT_STORE
      , weight(_weight)
#endif
    {}
  };

  StaticArray<Slot, 1000> slots;

  LeastSquares();

  bool IsEmpty() const {
    return sum_n == 0;
  }

  void Reset();

  void LeastSquaresUpdate();
  void LeastSquaresUpdate(fixed y);
  void LeastSquaresUpdate(fixed x, fixed y, fixed weight = fixed(1));

  void LeastSquaresErrorUpdate();

  void LeastSquaresAdd(fixed x, fixed y, fixed weight = fixed(1));
};

#endif // _LEASTSQS_H
