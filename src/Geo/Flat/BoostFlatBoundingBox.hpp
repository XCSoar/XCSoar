// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BoostFlatGeoPoint.hpp"
#include "FlatBoundingBox.hpp"

#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/concepts/box_concept.hpp>

BOOST_GEOMETRY_REGISTER_BOX(FlatBoundingBox, FlatGeoPoint,
                            lower_left, upper_right)
