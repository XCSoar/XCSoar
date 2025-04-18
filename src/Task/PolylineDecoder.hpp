// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

struct GeoPoint;

int_fast32_t
ReadPolylineInt(std::string_view &src);

GeoPoint
ReadPolylineGeoPoint(std::string_view &src);

/**
 * Like ReadPolylineGeoPoint(), but read the longitude first and then
 * the latitude.
 */
GeoPoint
ReadPolylineLonLat(std::string_view &src);

/**
 * Parse an encoded polyline.
 *
 * Throws on error.
 *
 * @see https://developers.google.com/maps/documentation/utilities/polylinealgorithm
 */
std::vector<GeoPoint>
DecodePolyline(std::string_view src);
