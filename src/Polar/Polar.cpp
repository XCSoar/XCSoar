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
#include "Units/Units.hpp"

#include <stdlib.h>
#include <cstdio>

PolarCoefficients
PolarCoefficients::FromVW(fixed v1, fixed v2, fixed v3,
                          fixed w1, fixed w2, fixed w3)
{
  PolarCoefficients pc;

  fixed d = v1 * v1 * (v2 - v3) + v2 * v2 * (v3 - v1) + v3 * v3 * (v1 - v2);
  pc.a = (d == fixed_zero) ? fixed_zero :
         -((v2 - v3) * (w1 - w3) + (v3 - v1) * (w2 - w3)) / d;

  d = v2 - v3;
  pc.b = (d == fixed_zero) ? fixed_zero:
         -(w2 - w3 + pc.a * (v2 * v2 - v3 * v3)) / d;

  pc.c = -(w3 + pc.a * v3 * v3 + pc.b * v3);

  return pc;
}

PolarCoefficients
PolarInfo::CalculateCoefficients() const
{
  return PolarCoefficients::FromVW(v1, v2, v3, w1, w2, w3);
}

bool
PolarCoefficients::IsValid() const
{
  return positive(a) && negative(b) && positive(c);
}

bool
PolarInfo::IsValid() const
{
  return CalculateCoefficients().IsValid();
}

void
PolarInfo::Init()
{
  v1 = v2 = v3 = w1 = w2 = w3 = fixed_zero;
  reference_mass = dry_mass = max_ballast = wing_area = v_no = fixed_zero;
  name = NULL;
}

bool
PolarInfo::CopyIntoGlidePolar(GlidePolar &polar) const
{
  PolarCoefficients pc = CalculateCoefficients();
  if (!pc.IsValid())
    return false;

  polar.ideal_polar_a = pc.a;
  polar.ideal_polar_b = pc.b;
  polar.ideal_polar_c = pc.c;

  // Glider empty weight
  polar.reference_mass = reference_mass;
  polar.dry_mass = (positive(dry_mass) ? dry_mass : reference_mass);
  // Ballast weight
  polar.ballast_ratio = max_ballast / polar.reference_mass;

  polar.wing_area = wing_area;

  polar.Update();
  return true;
}

void
PolarInfo::GetString(TCHAR* line, size_t size, bool include_v_no) const
{
  fixed V1, V2, V3;
  V1 = Units::ToUserUnit(v1, unKiloMeterPerHour);
  V2 = Units::ToUserUnit(v2, unKiloMeterPerHour);
  V3 = Units::ToUserUnit(v3, unKiloMeterPerHour);

  if (include_v_no)
    _sntprintf(line, size, _T("%.0f,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"),
               (double)reference_mass, (double)max_ballast, (double)V1, (double)w1,
               (double)V2, (double)w2, (double)V3, (double)w3,
               (double)wing_area, (double)v_no);
  else
    _sntprintf(line, size, _T("%.0f,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"),
               (double)reference_mass, (double)max_ballast, (double)V1, (double)w1,
               (double)V2, (double)w2, (double)V3, (double)w3,
               (double)wing_area);
}

bool
PolarInfo::ReadString(const TCHAR *line)
{
  PolarInfo polar;
  // Example:
  // *LS-3  WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
  // 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,  -3.4

  if (line[0] == _T('*'))
    /* a comment */
    return false;

  TCHAR *p;

  polar.reference_mass = fixed(_tcstod(line, &p));
  if (*p != _T(','))
    return false;

  polar.max_ballast = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.v1 = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), unKiloMeterPerHour);
  if (*p != _T(','))
    return false;

  polar.w1 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.v2 = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), unKiloMeterPerHour);
  if (*p != _T(','))
    return false;

  polar.w2 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.v3 = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), unKiloMeterPerHour);
  if (*p != _T(','))
    return false;

  polar.w3 = fixed(_tcstod(p + 1, &p));
  polar.wing_area = (*p != _T(',')) ? fixed_zero : fixed(_tcstod(p + 1, &p));
  polar.v_no = (*p != _T(',')) ? fixed_zero : fixed(_tcstod(p + 1, &p));
  polar.dry_mass = fixed_zero;

  *this = polar;

  return true;
}
