/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_TRACKING_SKYLINES_IMPORT_HPP
#define XCSOAR_TRACKING_SKYLINES_IMPORT_HPP

#include "Protocol.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/ByteOrder.hxx"

#include <chrono>

namespace SkyLinesTracking {

constexpr Angle
ImportAngle(int32_t src)
{
  return Angle::Degrees(int32_t(FromBE32(src)) / 1000000.);
}

/**
 * Convert a SkyLines #SkyLinesTracking::GeoPoint to a XCSoar
 * #::GeoPoint.
 */
constexpr ::GeoPoint
ImportGeoPoint(SkyLinesTracking::GeoPoint src)
{
  return ::GeoPoint(ImportAngle(src.longitude), ImportAngle(src.latitude));
}

/**
 * Import a big-endian time stamp to std::chrono::milliseconds.
 */
constexpr std::chrono::milliseconds
ImportTimeMs(uint32_t src_be)
{
  return std::chrono::milliseconds(FromBE32(src_be));
}

} /* namespace SkyLinesTracking */

#endif
