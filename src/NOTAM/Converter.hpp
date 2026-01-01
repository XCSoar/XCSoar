// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"

#include <vector>

class Airspaces;
struct GeoPoint;

namespace NOTAMConverter {

/**
 * Convert a NOTAM to XCSoar airspace format
 * and add it to the airspace database
 * 
 * @param notam NOTAM to convert
 * @param airspaces Target airspace database
 * @return true if the NOTAM was successfully converted and added
 */
[[nodiscard]] bool
ConvertNOTAMToAirspace(const NOTAM &notam, Airspaces &airspaces) noexcept;

/**
 * Convert multiple NOTAMs to airspace format
 * 
 * @param notams Vector of NOTAMs to convert
 * @param airspaces Target airspace database
 * @return Number of NOTAMs successfully converted
 */
[[nodiscard]] unsigned
ConvertNOTAMsToAirspaces(const std::vector<NOTAM> &notams,
                         Airspaces &airspaces) noexcept;

/**
 * Convert NOTAM coordinates to GeoPoint
 *
 * @param point NOTAM coordinate to convert
 * @return Converted GeoPoint
 */
[[nodiscard]] GeoPoint
NOTAMPointToGeoPoint(const NOTAMPoint &point) noexcept;

} // namespace NOTAMConverter
