/* Copyright_License {

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

#include "Filter.hpp"
#include "Angle.hpp"
#include "Util.hpp"

#include <assert.h>

bool
Filter::Design(const double cutoff_wavelength, const bool bessel)
{
  static constexpr unsigned sample_freq = 1;
  static constexpr unsigned n = 1;
  double c;
  unsigned g, p;

  if (bessel) {
    // Bessel
    c = pow((sqrt(pow(2., 1. / n) - 0.75) - 0.5), -0.5) / sqrt(3.);
    g = p = 3;
  } else {
    // Critically damped
    c = pow((pow(2., 0.5 / n) - 1.), -0.5);
    g = 1;
    p = 2;
  }

  auto f_star = c / (sample_freq * cutoff_wavelength);

  if (f_star <= 0 || f_star >= 1. / 8.) {
    ok = false;
    return false;
  }

  auto omega0 = (Angle::HalfCircle() * f_star).tan();
  auto K1 = p * omega0;
  auto K2 = g * Square(omega0);

  a[0] = K2 / (1. + K1 + K2);
  a[1] = 2 * a[0];
  a[2] = a[0];
  b[0] = a[1] * (1. / K2 - 1.);
  b[1] = 1. - (a[0] + a[1] + a[2] + b[0]);

  Reset(0);
  ok = true;

  return true;
}

double
Filter::Reset(const double _x)
{
  x[0] = _x;
  y[0] = _x;
  x[1] = _x;
  y[1] = _x;
  x[2] = _x;

  return _x;
}

double
Filter::Update(const double _x)
{
  assert(ok);

  x[2] = x[1];
  x[1] = x[0];
  x[0] = _x;

  auto _y = a[0] * x[0]
          + a[1] * x[1]
          + a[2] * x[2]
          + b[0] * y[0]
          + b[1] * y[1];

  y[1] = y[0];
  y[0] = _y;

  return _y;
}
