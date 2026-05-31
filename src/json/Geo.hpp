// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <boost/json/value_from.hpp>
#include <boost/json/value_to.hpp>

#include <stdexcept>

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
           const Angle &request) noexcept
{
  jv = request.Degrees();
}

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
           const GeoPoint &request) noexcept
{
  jv = {
    {"longitude", boost::json::value_from(request.longitude)},
    {"latitude", boost::json::value_from(request.latitude)},
  };
}

/**
 * GeoJSON position: [longitude, latitude] in degrees.
 */
inline GeoPoint
tag_invoke(boost::json::value_to_tag<GeoPoint>,
           const boost::json::value &jv)
{
  if (!jv.is_array())
    throw std::runtime_error("Invalid GeoJSON coordinate");

  const auto &arr = jv.as_array();
  if (arr.size() < 2 || !arr[0].is_number() || !arr[1].is_number())
    throw std::runtime_error("Invalid GeoJSON coordinate");

  const double longitude = arr[0].to_number<double>();
  const double latitude = arr[1].to_number<double>();
  GeoPoint point{Angle::Degrees(longitude), Angle::Degrees(latitude)};
  if (!point.Check())
    throw std::runtime_error("Invalid GeoJSON coordinate");
  return point;
}
