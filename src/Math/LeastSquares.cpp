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
  StoreReset();
  sum_xxw = 0.;
  sum_xyw = 0.;
  max_error = 0.;
  sum_error = 0.;
  rms_error = 0.;

  x_mean = y_mean = x_S = y_S = xy_C = x_var = y_var = xy_var = 0.;
}

void
LeastSquares::Compute()
{
  auto denom = (sum_weights * sum_xxw - sum_xw * sum_xw);

  if (fabs(denom) > 0) {
    m = (sum_weights * sum_xyw - sum_xw * sum_yw) / denom;
  } else {
    m = 0.;
  }
  b = (sum_yw - m * sum_xw) / sum_weights;

  y_ave = sum_yw / sum_weights;

  if (sum_n>1) {
    x_var = x_S / (sum_n - 1);
    y_var = y_S / (sum_n - 1);
    xy_var = xy_C / (sum_n - 1);
  }
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
  StoreAdd(x, y, weight);

  sum_xxw += Square(x)*weight;
  sum_xyw += x * y * weight;

  // See Knuth TAOCP vol 2, 3rd edition, page 232
  if (sum_n == 1) {
    x_mean = x;
    y_mean = y;
  } else {
    auto dx = x-x_mean;
    auto dy = y-y_mean;

    x_mean += dx / sum_n;
    x_S += dx * (x - x_mean);

    y_mean += dy / sum_n;
    y_S += dy * (y - y_mean);

    xy_C += dx * (y - y_mean);
  }
}

void
LeastSquares::Remove(const unsigned i)
{
  assert(i< sum_n);

  const auto &pt = slots[i];
  // Remove weighted point
  double weight = 1;
#ifdef LEASTSQS_WEIGHT_STORE
  weight = pt.weight;
#endif

  sum_xxw -= Square(pt.x) * weight;
  sum_xyw -= (pt.x * pt.y * weight);

  StoreRemove(i);

  // Update calculation
  Compute();
}

ErrorEllipse
LeastSquares::GetErrorEllipse() const
{
  /*
    A = a b = cov(x,x)   cov(x,y)
    c d   cov(x,y)   cov(y,y)

    T = trace = a + d     = cov(x,x) + cov(y,y)
    D = det = a*d - b*c   = cov(x,x) * cov(y,y) - cov(x,y)**2

    eigenvalues are L1,2 = T/2 +/-  sqrt(T^2/4 -D)
    if c is not zero, the eigenvectors are:
    L1-d    L2-d
    c       c
    else if b is not zero then the eigenvectors are:
    b       b
    L1-a    L2-a

    else the eigenvectors are
    1       0
    0       1

    http://www.visiondummy.com/wp-content/uploads/2014/04/error_ellipse.cpp

  */
  double a = GetVarX();
  double b = GetCovXY();
  double c = b;
  double d = GetVarY();

  double T = a + d;
  double D = a*d - b*c;
  double La = T/2;
  double Lb = sqrt(T*T/4-D);

  double L1 = La+Lb;
  double L2 = La-Lb;

  double v1x, v2x;
  if (b == 0) {
    v1x = 1;
    v2x = 0;
  } else {
    v1x = L1- GetVarY();
    v2x = L2- GetVarY();
  }

  ErrorEllipse ellipse;
  ellipse.angle = Angle::FromXY(v2x, v1x);
  ellipse.halfmajor = sqrt(L1);
  ellipse.halfminor = sqrt(L2);
  ellipse.x = x_mean;
  ellipse.y = y_mean;

  //double chisquare_val = 2.4477;
  return ellipse;
}
