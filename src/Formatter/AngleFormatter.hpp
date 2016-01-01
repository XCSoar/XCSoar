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

#ifndef XCSOAR_ANGLE_FORMATTER_HPP
#define XCSOAR_ANGLE_FORMATTER_HPP

#include "Util/StringBuffer.hxx"
#include "Math/Angle.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <stddef.h>

class Angle;

void
FormatBearing(TCHAR *buffer, size_t size, unsigned degrees_value,
              const TCHAR *suffix = NULL);

void
FormatBearing(TCHAR *buffer, size_t size, Angle value,
              const TCHAR *suffix = NULL);

gcc_const
static inline StringBuffer<TCHAR, 16>
FormatBearing(unsigned degrees_value)
{
  StringBuffer<TCHAR, 16> buffer;
  FormatBearing(buffer.data(), buffer.capacity(), degrees_value);
  return buffer;
}

gcc_const
static inline StringBuffer<TCHAR, 16>
FormatBearing(Angle value)
{
  StringBuffer<TCHAR, 16> buffer;
  FormatBearing(buffer.data(), buffer.capacity(), value);
  return buffer;
}

void
FormatAngleDelta(TCHAR *buffer, size_t size, Angle value);

gcc_const
static inline StringBuffer<TCHAR, 16>
FormatAngleDelta(Angle value)
{
  StringBuffer<TCHAR, 16> buffer;
  FormatAngleDelta(buffer.data(), buffer.capacity(), value);
  return buffer;
}

void
FormatVerticalAngleDelta(TCHAR *buffer, size_t size, Angle value);

#endif
