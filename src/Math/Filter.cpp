/* Copyright_License {

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

#include "Filter.hpp"
#include "Math/Angle.hpp"

#include <assert.h>
#include <stdio.h>

bool
Filter::Design(const fixed cutoff_wavelength, const bool bessel)
{
  static const unsigned sample_freq = 1;
  static const unsigned n = 1;
  fixed c;
  unsigned g, p;

  if (bessel) {
    // Bessel
    c = pow((sqrt(pow(fixed(2), fixed(1) / n) - fixed(0.75)) - fixed(0.5)),
            -fixed(0.5)) / sqrt(fixed(3));
    g = p = 3;
  } else {
    // Critically damped
    c = pow((pow(fixed(2), fixed(1) / (2 * n)) - fixed(1)), -fixed(0.5));
    g = 1;
    p = 2;
  }

  fixed f_star = c / (sample_freq * cutoff_wavelength);

  if (!positive(f_star) || f_star >= fixed(1) / 8) {
    ok = false;
    return false;
  }

  fixed omega0 = (Angle::HalfCircle() * f_star).tan();
  fixed K1 = p * omega0;
  fixed K2 = g * sqr(omega0);

  a[0] = K2 / (fixed(1) + K1 + K2);
  a[1] = Double(a[0]);
  a[2] = a[0];
  b[0] = a[1] * (fixed(1) / K2 - fixed(1));
  b[1] = fixed(1) - (a[0] + a[1] + a[2] + b[0]);

  Reset(fixed(0));
  ok = true;

  return true;
}

fixed
Filter::Reset(const fixed _x)
{
  x[0] = _x;
  y[0] = _x;
  x[1] = _x;
  y[1] = _x;
  x[2] = _x;

  return _x;
}

fixed
Filter::Update(const fixed _x)
{
  assert(ok);

  x[2] = x[1];
  x[1] = x[0];
  x[0] = _x;

  fixed _y = a[0] * x[0]
           + a[1] * x[1]
           + a[2] * x[2]
           + b[0] * y[0]
           + b[1] * y[1];

  y[1] = y[0];
  y[0] = _y;

  return _y;
}
