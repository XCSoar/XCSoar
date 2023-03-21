// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <vector>

class AirspaceIntersectionVector:
  public std::vector< std::pair<GeoPoint, GeoPoint> > {};
