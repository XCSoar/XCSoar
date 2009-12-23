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

#ifndef __cplusplus
# error This library requires C++
#endif

#define MAX_STATISTICS 1000

/**
 * A solver for least squares problems
 *
 * Classical least squares fit:
 *
 * \f[
 *     y = b_0 + b_1 * x
 * \f]
 *
 * \f[
 *     b_1 = \frac{n * \sum_0^{i-1} (x_i*y_i) - \sum_0^{i-1} x_i* \sum_0^{i-1} y_i}
 *           {n*\sum_0^{i-1} x_i^2 - (\sum_0^{i-1} x_i)^2}
 * \f]
 *
 * \f[
 *     b_0 = \frac{\sum_0^{i-1} y_i}{n} - b_1 * \frac{\sum_0^{i-1} x_i}{n}
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
  double sum_xi, sum_yi, sum_xi_2, sum_xi_yi;
  int sum_n;
  double m;
  double b;
  double sum_error;

  double rms_error;
  double max_error;
  double sum_weights;
  double y_max;
  double y_min;
  double x_min;
  double x_max;

  double y_ave;

  float xstore[MAX_STATISTICS];
  float ystore[MAX_STATISTICS];
  float weightstore[MAX_STATISTICS];

  LeastSquares();

  void Reset();

  void LeastSquaresUpdate();
  void LeastSquaresUpdate(double y);
  void LeastSquaresUpdate(double x, double y, double weight = 1.0);

  void LeastSquaresErrorUpdate();

  void LeastSquaresAdd(double x, double y, double weight = 1.0);
};

#endif // _LEASTSQS_H
