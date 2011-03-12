/*
Copyright_License {

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

#include "Polar/Polar.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Units.hpp"

#include <stdlib.h>
#include <cstdio>

PolarCoefficients
SimplePolar::CalculateCoefficients() const
{
  PolarCoefficients pc;

  fixed d;
  fixed V1, V2, V3;
  fixed W1, W2, W3;

  V1 = Units::ToSysUnit(fixed(v1), unKiloMeterPerHour);
  V2 = Units::ToSysUnit(fixed(v2), unKiloMeterPerHour);
  V3 = Units::ToSysUnit(fixed(v3), unKiloMeterPerHour);
  W1 = fixed(w1);
  W2 = fixed(w2);
  W3 = fixed(w3);

  d = V1 * V1 * (V2 - V3) + V2 * V2 * (V3 - V1) + V3 * V3 * (V1 - V2);
  pc.a = (d == fixed_zero) ? fixed_zero :
         -((V2 - V3) * (W1 - W3) + (V3 - V1) * (W2 - W3)) / d;

  d = V2 - V3;
  pc.b = (d == fixed_zero) ? fixed_zero:
         -(W2 - W3 + pc.a * (V2 * V2 - V3 * V3)) / d;

  pc.c = -(W3 + pc.a * V3 * V3 + pc.b * V3);

  return pc;
}

bool
SimplePolar::IsValid(const PolarCoefficients &coeff)
{
  return positive(coeff.a) && negative(coeff.b) && positive(coeff.c);
}

bool
SimplePolar::IsValid() const
{
  PolarCoefficients pc = CalculateCoefficients();
  return IsValid(pc);
}

void
SimplePolar::Init()
{
  v1 = v2 = v3 = w1 = w2 = w3 = fixed_zero;
  dry_mass = max_ballast = wing_area = v_no = fixed_zero;
  name = NULL;
}

bool
SimplePolar::CopyIntoGlidePolar(GlidePolar &polar) const
{
  PolarCoefficients pc = CalculateCoefficients();
  if (!IsValid(pc))
    return false;

  polar.ideal_polar_a = pc.a;
  polar.ideal_polar_b = pc.b;
  polar.ideal_polar_c = pc.c;

  // Glider empty weight
  polar.dry_mass = fixed(dry_mass);
  // Ballast weight
  polar.ballast_ratio = fixed(max_ballast) / polar.dry_mass;

  polar.wing_area = fixed(wing_area);

  polar.update();
  return true;
}

void
SimplePolar::GetString(TCHAR* line, size_t size, bool include_v_no) const
{
  if (include_v_no)
    _sntprintf(line, size, _T("%.0f,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"),
               (double)dry_mass, (double)max_ballast, (double)v1, (double)w1,
               (double)v2, (double)w2, (double)v3, (double)w3,
               (double)wing_area, (double)v_no);
  else
    _sntprintf(line, size, _T("%.0f,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"),
               (double)dry_mass, (double)max_ballast, (double)v1, (double)w1,
               (double)v2, (double)w2, (double)v3, (double)w3,
               (double)wing_area);
}

bool
SimplePolar::ReadString(const TCHAR *line)
{
  SimplePolar polar;
  // Example:
  // *LS-3  WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
  // 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,  -3.4

  if (line[0] == _T('*'))
    /* a comment */
    return false;

  TCHAR *p;

  polar.dry_mass = fixed(_tcstod(line, &p));
  if (*p != _T(','))
    return false;

  polar.max_ballast = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.v1 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.w1 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.v2 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.w2 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.v3 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.w3 = fixed(_tcstod(p + 1, &p));
  polar.wing_area = (*p != _T(',')) ? fixed_zero : fixed(_tcstod(p + 1, &p));
  polar.v_no = (*p != _T(',')) ? fixed_zero : fixed(_tcstod(p + 1, &p));

  *this = polar;

  return true;
}
