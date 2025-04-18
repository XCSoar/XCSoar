// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringBuffer.hxx"
#include "Geo/CoordinateFormat.hpp"

#include <tchar.h>
#include <cstddef>

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

[[gnu::pure]]
static inline BasicStringBuffer<TCHAR, 32>
FormatGeoPoint(const GeoPoint &location, CoordinateFormat format,
               TCHAR separator = _T(' '))
{
  BasicStringBuffer<TCHAR, 32> buffer;
  auto result = FormatGeoPoint(location, buffer.data(), buffer.capacity(),
                               format, separator);
  if (result == nullptr)
    buffer.clear();
  return buffer;
}
