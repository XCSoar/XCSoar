// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
