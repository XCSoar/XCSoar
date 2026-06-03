// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/xctherm/XCThermGeoJSON.hpp"

#include <string>

namespace XCTherm {

/**
 * Parse @p geojson and install it as the main map XCTherm overlay.
 *
 * @param label user-visible layer name (e.g. @c "5000m AMSL")
 * @param parameter API parameter for map-item metadata (optional)
 * @param forecast_utc valid UTC hour for cache lookup (optional)
 */
void ApplyForecastToMap(const std::string &geojson, const char *label,
                        const char *parameter = nullptr,
                        unsigned forecast_utc = 0) noexcept;

/**
 * Install an already-parsed forecast as the main map overlay.
 */
void ApplyForecastLayerToMap(XCThermGeoJSON::ForecastLayer &&forecast,
                             const char *label,
                             const char *parameter = nullptr,
                             unsigned forecast_utc = 0) noexcept;

void ClearMapOverlay() noexcept;

} // namespace XCTherm
