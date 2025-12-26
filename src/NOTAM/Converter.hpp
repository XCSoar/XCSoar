// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Engine/Airspace/Ptr.hpp"
#include "Geo/GeoPoint.hpp"

#include <optional>
#include <vector>

class Airspaces;

namespace NOTAMConverter {

inline constexpr double DEFAULT_POINT_RADIUS_METERS = 500.0;
inline constexpr double MAX_CIRCLE_RADIUS_METERS = 1000000.0;
inline constexpr double DEFAULT_CONSERVATIVE_FALLBACK_ALTITUDE_M = 20000.0;

/**
 * Convert a NOTAM to XCSoar airspace format
 * and add it to the airspace database
 * 
 * @param notam NOTAM to convert
 * @param airspaces Target airspace database
 * @return true if the NOTAM was successfully converted and added
 */
[[nodiscard]] bool
ConvertNOTAMToAirspace(const struct NOTAM &notam, Airspaces &airspaces);

/**
 * Convert a NOTAM to an airspace object.
 *
 * @param notam NOTAM to convert
 * @return AirspacePtr on success, empty on failure
 */
[[nodiscard]] AirspacePtr
BuildNOTAMAirspace(const struct NOTAM &notam);

/**
 * Convert multiple NOTAMs to airspace format
 * 
 * @param notams Vector of NOTAMs to convert
 * @param airspaces Target airspace database
 * @return Number of NOTAMs successfully converted
 */
[[nodiscard]] unsigned
ConvertNOTAMsToAirspaces(const std::vector<struct NOTAM> &notams,
                         Airspaces &airspaces);

} // namespace NOTAMConverter
