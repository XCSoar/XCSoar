// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "XCThermGeoJSON.hpp"

#include <string_view>

namespace XCTherm {

/**
 * Parse GeoJSON and apply it as the main map overlay.
 * Shows an error dialog on failure.
 */
void
ApplyForecastToMap(std::string_view geojson, const char *label) noexcept;

/**
 * Apply an already-parsed forecast layer to the main map overlay.
 */
void
ApplyForecastToMap(XCThermGeoJSON::ForecastLayer &&forecast,
                   const char *label) noexcept;

} // namespace XCTherm
