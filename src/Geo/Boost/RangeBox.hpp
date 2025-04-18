// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GeoPoint.hpp"

#include <boost/geometry/geometries/box.hpp>

/**
 * Create a boost::geometry box which covers the given range.
 */
[[gnu::const]]
boost::geometry::model::box<GeoPoint>
BoostRangeBox(const GeoPoint location, double range);
