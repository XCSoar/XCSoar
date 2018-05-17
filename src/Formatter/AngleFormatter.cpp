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

#include "AngleFormatter.hpp"
#include "Math/Angle.hpp"
#include "Util/StringFormat.hpp"

#include <assert.h>
#include <string.h>

void
FormatBearing(TCHAR *buffer, size_t size, unsigned value_degrees,
              const TCHAR *suffix)
{
  assert(buffer != NULL);
  assert(size >= 8);

  if (suffix != NULL)
    StringFormat(buffer, size, _T("%u° %s"), value_degrees, suffix);
  else
    StringFormat(buffer, size, _T("%u°"), value_degrees);
}

void
FormatBearing(TCHAR *buffer, size_t size, Angle value, const TCHAR *suffix)
{
  FormatBearing(buffer, size, lround(value.AsBearing().Degrees()), suffix);
}

void
FormatAngleDelta(TCHAR *buffer, size_t size, Angle value)
{
  assert(buffer != NULL);
  assert(size >= 8);

  auto degrees = lround(value.AsDelta().Degrees());
  if (degrees > 1)
    StringFormat(buffer, size, _T("%u°»"), unsigned(degrees));
  else if (degrees < -1)
    StringFormat(buffer, size, _T("«%u°"), unsigned(-degrees));
  else
    _tcscpy(buffer, _T("«»"));
}

void
FormatVerticalAngleDelta(TCHAR *buffer, size_t size, Angle value)
{
  assert(buffer != NULL);
  assert(size >= 8);

  auto degrees = lround(value.AsDelta().Degrees());
  if (degrees < -1 || degrees > 1)
    StringFormat(buffer, size, _T("%+d°"), int(degrees));
  else
    _tcscpy(buffer, _T("--"));
}
