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

// leastsqs.c -- Implements a simple linear least squares best fit routine
//
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

#include "LeastSquares.hpp"
#include "Util.hpp"

#include <math.h>

/*
Least squares fit:

  y = b0 + b1x

        n * sum(xi * yi) - (sum(xi) * sum(yi))
  b1 = ----------------------------------------
             n * sum(xi^2) - sum(xi)^2


  b0 = sum(yi) / n - b1 * (sum(xi) / n)

*/

/*
return the least squares error:

   (y[i] - y_hat[i])^2
  ---------------------
          n
*/

/*
return the maximum least squares error:

  (y[i] - y_hat[i])^2

*/

void
LeastSquares::Reset()
{
  sum_n = 0;
  sum_xi = 0.;
  sum_yi = 0.;
  sum_xi_2 = 0.;
  sum_xi_yi = 0.;
  max_error = 0.;
  sum_error = 0.;
  rms_error = 0.;
  sum_weights = 0.;
  y_max = 0.;
  y_min = 0.;
  x_min = 0.;
  x_max = 0.;
  y_ave = 0.;
  slots.clear();
}

void
LeastSquares::Compute()
{
  auto denom = (sum_weights * sum_xi_2 - sum_xi * sum_xi);

  if (fabs(denom) > 0) {
    m = (sum_weights * sum_xi_yi - sum_xi * sum_yi) / denom;
  } else {
    m = 0.;
  }
  b = (sum_yi - m * sum_xi) / sum_weights;

  y_ave = GetYAt(GetMiddleX());
}

void
LeastSquares::Update(double y)
{
  Update(double(sum_n + 1), y);
}

void
LeastSquares::Update(double x, double y, double weight)
{
  // Add new point
  Add(x, y, weight);
  // Update calculation
  Compute();

  // Calculate error
  auto error = fabs(y - GetYAt(x));
  sum_error += Square(error) * weight;
  if (error > max_error)
    max_error = error;
}

void
LeastSquares::UpdateError()
{
  rms_error = sqrt(sum_error / sum_weights);
}

void
LeastSquares::Add(double x, double y, double weight)
{
  // Update maximum/minimum values
  if (IsEmpty() || y > y_max)
    y_max = y;

  if (IsEmpty() || y < y_min)
    y_min = y;

  if (IsEmpty() || x > x_max)
    x_max = x;

  if (IsEmpty() || x < x_min)
    x_min = x;

  // Add point
  // TODO code: really should have a circular buffer here
  if (!slots.full())
    slots.append() = Slot(x, y, weight);

  ++sum_n;

  // Add weighted point
  sum_weights += weight;

  auto xw = x * weight;
  auto yw = y * weight;

  sum_xi += xw;
  sum_yi += yw;
  sum_xi_2 += Square(xw);
  sum_xi_yi += xw * yw;
}

void
LeastSquares::Remove(const unsigned i)
{
  assert(i< sum_n);
  const auto &pt = slots[i];

  --sum_n;

  // Remove weighted point
  auto weight = 1;
#ifdef LEASTSQS_WEIGHT_STORE
  weight = pt.weight;
#endif

  sum_weights -= weight;

  auto xw = pt.x * weight;
  auto yw = pt.y * weight;

  sum_xi -= xw;
  sum_yi -= yw;
  sum_xi_2 -= Square(xw);
  sum_xi_yi -= xw * yw;

  slots.remove(i);

  // Update calculation
  Compute();
}
