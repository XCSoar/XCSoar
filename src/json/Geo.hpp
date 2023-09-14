// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <boost/json/value_from.hpp>

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
