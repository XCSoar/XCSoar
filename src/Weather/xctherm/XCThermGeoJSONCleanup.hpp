// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "XCThermGeoJSON.hpp"

#include <vector>

namespace XCThermGeoJSON {

/**
 * Post-import cleanup for one exterior ring:
 * - drop consecutive duplicate vertices
 * - drop junk (fewer than 3 vertices, near-zero area)
 * - split self-crossing rings into simple exteriors
 * - decompose concave rings into convex pieces
 *
 * Returns zero or more exterior-only polygons (each a single ring).
 */
std::vector<std::vector<Ring>>
CleanExterior(const Ring &exterior) noexcept;

/**
 * Apply CleanExterior() to every polygon in @p band, keep exterior
 * only (holes discarded), and drop empty results.
 */
void
CleanBandPolygons(WindBand &band) noexcept;

/**
 * Sort bands by ascending |midpoint| so weaker fills draw first and
 * stronger bands overwrite (matches opaque overlay compositing).
 */
void
SortBandsByAbsMid(ForecastLayer &layer) noexcept;

} // namespace XCThermGeoJSON
