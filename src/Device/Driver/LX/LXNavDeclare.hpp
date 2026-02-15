// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Declaration.hpp"
#include "IGC/Generator.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"

#include <fmt/format.h>
#include <cmath>
#include <string>

namespace LXNavDeclare {

/**
 * Compute the A12 bearing for a turnpoint in the LXNAV OZ format.
 *
 * - Start: bearing from this toward the next turnpoint
 * - Intermediate: bisector of (bearing to prev, bearing to next)
 * - Finish: bearing from this toward the previous turnpoint
 */
inline Angle
ComputeA12(const GeoPoint &prev, const GeoPoint &current,
           const GeoPoint &next,
           bool is_start, bool is_finish) noexcept
{
  if (is_start)
    return current.Bearing(next);
  else if (is_finish)
    return current.Bearing(prev);
  else
    /* HalfAngle returns the outward-facing bisector (used for OZ
       sector rendering); LXNAV A12 is the inward bisector, so we
       apply Reciprocal() to flip by 180 degrees */
    return current.Bearing(prev)
      .HalfAngle(current.Bearing(next))
      .Reciprocal();
}

/**
 * Map the turnpoint position to the LXNAV OZ Style value.
 *
 * ozFixed=0, ozSymmetric=1, ozNext=2, ozPrev=3, ozStart=4
 */
constexpr unsigned
GetOZStyle(bool is_start, bool is_finish) noexcept
{
  if (is_start)
    return 2;
  else if (is_finish)
    return 3;
  else
    return 1;
}

/**
 * Format latitude in LXNAV OZ format: DDMM.mmmN/S
 */
inline std::string
FormatLat(const GeoPoint &location)
{
  const double abs_lat = std::abs(location.latitude.Degrees());
  const int deg = static_cast<int>(abs_lat);
  const double min = (abs_lat - deg) * 60.0;
  const char suffix = location.latitude.IsNegative() ? 'S' : 'N';
  return fmt::format("{:02d}{:06.3f}{}", deg, min, suffix);
}

/**
 * Format longitude in LXNAV OZ format: DDDMM.mmmE/W
 */
inline std::string
FormatLon(const GeoPoint &location)
{
  const double abs_lon = std::abs(location.longitude.Degrees());
  const int deg = static_cast<int>(abs_lon);
  const double min = (abs_lon - deg) * 60.0;
  const char suffix = location.longitude.IsNegative() ? 'W' : 'E';
  return fmt::format("{:03d}{:06.3f}{}", deg, min, suffix);
}

/**
 * Format an LLXVOZ line for a single turnpoint in the declaration.
 *
 * @param declaration The full declaration
 * @param tp_index Index of the turnpoint within the declaration
 * @return The formatted LLXVOZ line content
 */
inline std::string
FormatOZLine(const Declaration &declaration, unsigned tp_index)
{
  const auto &tp = declaration.turnpoints[tp_index];
  const unsigned n_tp = declaration.Size();
  const bool is_start = (tp_index == 0);
  const bool is_finish = (tp_index == n_tp - 1);

  const int oz_index = static_cast<int>(tp_index) - 1;

  const unsigned style = GetOZStyle(is_start, is_finish);
  const unsigned r1 = tp.radius;
  const double a1 = tp.sector_angle.Degrees();
  const unsigned r2 = tp.inner_radius;
  const double a2 = (r2 > 0) ? 180.0 : 0.0;

  const GeoPoint &prev_loc = is_start
    ? tp.waypoint.location
    : declaration.turnpoints[tp_index - 1].waypoint.location;
  const GeoPoint &next_loc = is_finish
    ? tp.waypoint.location
    : declaration.turnpoints[tp_index + 1].waypoint.location;
  const double a12 = ComputeA12(prev_loc, tp.waypoint.location,
                                next_loc, is_start, is_finish)
    .AsBearing().Degrees();

  const auto lat_str = FormatLat(tp.waypoint.location);
  const auto lon_str = FormatLon(tp.waypoint.location);

  auto line = fmt::format(
    "LLXVOZ={},Style={},R1={}m,A1={:.1f},R2={}m,A2={:.1f},"
    "A12={:.1f},Maxa=0m",
    oz_index, style, r1, a1, r2, a2, a12);

  if (tp.shape == Declaration::TurnPoint::LINE)
    line += ",Line=1";

  line += ",Autonext=1";

  if (tp.is_aat)
    line += ",AAT=1";

  line += fmt::format(",Lat={},Lon={}", lat_str, lon_str);

  line += fmt::format(",Near={}", is_finish ? 1 : 0);

  return line;
}

/**
 * Format a C-record turnpoint string with LXNAV elevation extension.
 *
 * Format: C<lat><lon><name>::<elevation>
 */
inline std::string
FormatTurnPointCRecord(const Declaration::TurnPoint &tp)
{
  char loc_buf[32];
  char *p = FormatIGCLocation(loc_buf, tp.waypoint.location);
  *p = '\0';

  const double elevation = tp.waypoint.GetElevationOrZero();
  return fmt::format("C{}{}{}{:.5f}", loc_buf,
                     tp.waypoint.name.c_str(),
                     "::", elevation);
}

} // namespace LXNavDeclare
