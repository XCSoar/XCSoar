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

#ifndef XCSOAR_GEOPOINT_FORMATTER_HPP
#define XCSOAR_GEOPOINT_FORMATTER_HPP

#include "Util/StringBuffer.hxx"
#include "Geo/CoordinateFormat.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <stddef.h>

class Angle;
struct GeoPoint;

/**
 * Converts a double-based longitude into a formatted string
 * @param longitude The double-based longitude
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
bool FormatLongitude(Angle longitude, TCHAR *buffer, size_t size,
                     CoordinateFormat format);

/**
 * Converts a double-based Latitude into a formatted string
 * @param Latitude The double-based Latitude
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
bool FormatLatitude(Angle latitude, TCHAR *buffer, size_t size,
                    CoordinateFormat format);

/**
 * Convert a GeoPoint into a formatted string.
 */
TCHAR *FormatGeoPoint(const GeoPoint &location, TCHAR *buffer, size_t size,
                      CoordinateFormat format, TCHAR separator = _T(' '));

gcc_pure
static inline StringBuffer<TCHAR, 32>
FormatGeoPoint(const GeoPoint &location, CoordinateFormat format,
               TCHAR separator = _T(' '))
{
  StringBuffer<TCHAR, 32> buffer;
  auto result = FormatGeoPoint(location, buffer.data(), buffer.capacity(),
                               format, separator);
  if (result == nullptr)
    buffer.clear();
  return buffer;
}

#endif
