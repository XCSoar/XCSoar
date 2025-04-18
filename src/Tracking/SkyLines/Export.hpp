// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/ByteOrder.hxx"

namespace SkyLinesTracking {

constexpr int32_t
ExportAngle(Angle src)
{
  return ToBE32(int(src.Degrees() * 1000000));
}

inline GeoPoint
ExportGeoPoint(::GeoPoint src)
{
    src.Normalize();
    return { ExportAngle(src.latitude), ExportAngle(src.longitude) };
}

} /* namespace SkyLinesTracking */
