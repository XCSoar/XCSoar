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

#include "PolarCoefficients.hpp"

PolarCoefficients
PolarCoefficients::From3VW(double v1, double v2, double v3,
                           double w1, double w2, double w3)
{
  PolarCoefficients pc;

  auto d = v1 * v1 * (v2 - v3) + v2 * v2 * (v3 - v1) + v3 * v3 * (v1 - v2);
  pc.a = d == 0
    ? 0.
    : -((v2 - v3) * (w1 - w3) + (v3 - v1) * (w2 - w3)) / d;

  d = v2 - v3;
  pc.b = d == 0
    ? 0.
    : -(w2 - w3 + pc.a * (v2 * v2 - v3 * v3)) / d;

  pc.c = -(w3 + pc.a * v3 * v3 + pc.b * v3);

  return pc;
}

PolarCoefficients
PolarCoefficients::From2VW(double v1, double v2, double w1, double w2)
{
  PolarCoefficients pc;

  auto d = (v2 - v1) * (v2 - v1);
  pc.a = d == 0
    ? 0.
    : (w2 - w1) / d;
  pc.b = -2 * pc.a * v1;
  pc.c = pc.a * v1 * v1 + w1;

  return pc;
}

bool
PolarCoefficients::IsValid() const
{
  return a > 0 && b < 0 && c > 0;
}
