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

#ifndef XCSOAR_UNITS_FORMATTER_HPP
#define XCSOAR_UNITS_FORMATTER_HPP

#include "Units/Units.hpp"
#include "Math/fixed.hpp"

#include <tchar.h>

class Angle;
struct GeoPoint;

/**
 * Namespace to manage unit display.
 */
namespace Units
{
  /**
   * Converts a double-based Longitude into a formatted string
   * @param Longitude The double-based Longitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   */
  bool LongitudeToString(Angle Longitude, TCHAR *Buffer, size_t size);

  /**
   * Converts a double-based Latitude into a formatted string
   * @param Latitude The double-based Latitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   */
  bool LatitudeToString(Angle Latitude, TCHAR *Buffer, size_t size);

  /**
   * Convert a GeoPoint into a formatted string.
   */
  TCHAR *FormatGeoPoint(const GeoPoint &location, TCHAR *buffer, size_t size);

  /**
   * Converts a double-based Altitude into a formatted string
   * @param Altitude The double-based Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserAltitude(fixed Altitude, TCHAR *Buffer, size_t size,
                          bool IncludeUnit = true);

  /**
   * Converts a double-based Altitude into a formatted string of the alternate
   * altitude format
   * @param Altitude The double-based Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatAlternateUserAltitude(fixed Altitude, TCHAR *Buffer,
                                   size_t size, bool IncludeUnit = true);

  /**
   * Converts a double-based Arrival Altitude into a formatted string
   * @param Altitude The double-based Arrival Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserArrival(fixed Altitude, TCHAR *Buffer, size_t size,
                         bool IncludeUnit = true);

  /**
   * Converts a double-based horizontal Distance into a formatted string
   * @param Distance The double-based Distance
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserDistance(fixed Distance, TCHAR *Buffer, size_t size,
                          bool IncludeUnit = true);

  bool FormatUserMapScale(fixed Distance, TCHAR *Buffer,
                          size_t size, bool IncludeUnit = true);

  /**
   * Converts a double-based Speed into a formatted string
   * @param Speed The double-based Speed
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserSpeed(fixed Altitude, TCHAR *Buffer, size_t size,
                       bool IncludeUnit = true, bool Precision = true);

  /**
   * Converts a double-based Speed into a formatted string
   * @param Speed The double-based Speed
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserWindSpeed(fixed speed, TCHAR *buffer, size_t size,
                           bool IncludeUnit = true, bool Precision = true);

  /**
   * Converts a double-based vertical Speed into a formatted string
   * @param Speed The double-based vertical Speed
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserVSpeed(fixed Altitude, TCHAR *Buffer, size_t size,
                        bool IncludeUnit = true);

  /**
   * precedes with "-" if time is negative
   * @param text returns HHMM
   * @param d input seconds
   */
  void TimeToTextHHMMSigned(TCHAR* text, int d);

  /**
   * sets HHMMSSSmart and SSSmart
   * if hours > 0, returns HHMM in HHMMSSSmart and SS in SSSmart
   * if hours == 0, returns MMSS in HHMMSSSmart and "" in SSSmart
   * @param HHMMSSSmart buffer
   * @param SSSmart buffer
   * @param d input seconds
   */
  void TimeToTextSmart(TCHAR* HHMMSSSmart, TCHAR* SSSmart,int d);
};

#endif
