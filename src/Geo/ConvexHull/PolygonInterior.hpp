// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Geo/SearchPointVector.hpp"

struct GeoPoint;
struct FlatGeoPoint;
class SearchPoint;

/**
 * Note that this expects the vector to be closed, that is, starting point
 * and ending point are the same
 */
[[gnu::pure]]
bool
PolygonInterior(const GeoPoint &p,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end);

[[gnu::pure]]
bool
PolygonInterior(const FlatGeoPoint &p,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end);
