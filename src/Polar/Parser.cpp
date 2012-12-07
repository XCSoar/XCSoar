/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Parser.hpp"
#include "Polar.hpp"
#include "Units/System.hpp"

#include <cstdio>

bool
ParsePolarShape(PolarShape &shape, const TCHAR *s)
{
  TCHAR *p;
  fixed v1 = Units::ToSysUnit(fixed(_tcstod(s, &p)), Unit::KILOMETER_PER_HOUR);
  if (*p != _T(','))
    return false;

  fixed w1 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  fixed v2 = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), Unit::KILOMETER_PER_HOUR);
  if (*p != _T(','))
    return false;

  fixed w2 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  fixed v3 = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), Unit::KILOMETER_PER_HOUR);
  if (*p != _T(','))
    return false;

  fixed w3 = fixed(_tcstod(p + 1, &p));
  if (*p != '\0')
    return false;

  shape[0].v = v1;
  shape[0].w = w1;
  shape[1].v = v2;
  shape[1].w = w2;
  shape[2].v = v3;
  shape[2].w = w3;
  return true;
}

bool
ParsePolar(PolarInfo &polar_r, const TCHAR *s)
{
  PolarInfo polar;
  // Example:
  // *LS-3  WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
  // 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,  -3.4

  if (s[0] == _T('*'))
    /* a comment */
    return false;

  TCHAR *p;
  polar.reference_mass = fixed(_tcstod(s, &p));
  if (*p != _T(','))
    return false;

  polar.max_ballast = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.shape[0].v = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), Unit::KILOMETER_PER_HOUR);
  if (*p != _T(','))
    return false;

  polar.shape[0].w = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.shape[1].v = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), Unit::KILOMETER_PER_HOUR);
  if (*p != _T(','))
    return false;

  polar.shape[1].w = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  polar.shape[2].v = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), Unit::KILOMETER_PER_HOUR);
  if (*p != _T(','))
    return false;

  polar.shape[2].w = fixed(_tcstod(p + 1, &p));
  polar.wing_area = (*p != _T(',')) ? fixed(0) : fixed(_tcstod(p + 1, &p));
  polar.v_no = (*p != _T(',')) ? fixed(0) : fixed(_tcstod(p + 1, &p));

  polar_r = polar;
  return true;
}

void
FormatPolarShape(const PolarShape &shape, TCHAR *buffer, size_t max_size)
{
  fixed v1, v2, v3;
  v1 = Units::ToUserUnit(shape[0].v, Unit::KILOMETER_PER_HOUR);
  v2 = Units::ToUserUnit(shape[1].v, Unit::KILOMETER_PER_HOUR);
  v3 = Units::ToUserUnit(shape[2].v, Unit::KILOMETER_PER_HOUR);

  _sntprintf(buffer, max_size, _T("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"),
             (double)v1, (double)shape[0].w,
             (double)v2, (double)shape[1].w,
             (double)v3, (double)shape[2].w);
}

void
FormatPolar(const PolarInfo &polar, TCHAR *buffer, size_t max_size,
            bool include_v_no)
{
  fixed v1, v2, v3;
  v1 = Units::ToUserUnit(polar.shape[0].v, Unit::KILOMETER_PER_HOUR);
  v2 = Units::ToUserUnit(polar.shape[1].v, Unit::KILOMETER_PER_HOUR);
  v3 = Units::ToUserUnit(polar.shape[2].v, Unit::KILOMETER_PER_HOUR);

  if (include_v_no)
    _sntprintf(buffer, max_size,
               _T("%.0f,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"),
               (double)polar.reference_mass, (double)polar.max_ballast,
               (double)v1, (double)polar.shape[0].w,
               (double)v2, (double)polar.shape[1].w,
               (double)v3, (double)polar.shape[2].w,
               (double)polar.wing_area, (double)polar.v_no);
  else
    _sntprintf(buffer, max_size,
               _T("%.0f,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"),
               (double)polar.reference_mass, (double)polar.max_ballast,
               (double)v1, (double)polar.shape[0].w,
               (double)v2, (double)polar.shape[1].w,
               (double)v3, (double)polar.shape[2].w,
               (double)polar.wing_area);
}
