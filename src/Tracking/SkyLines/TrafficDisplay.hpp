// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Details.hpp"
#include "FLARM/Id.hpp"
#include "util/StaticString.hxx"

#include <cstdint>

namespace SkyLinesTracking {

/**
 * Build the primary label for an online traffic target.
 *
 * Priority: FLARM database (callsign, registration, pilot), live
 * traffic name, server-supplied name.  Leaves the title empty when
 * nothing else is known; the traffic source line carries OGN / SkyLines.
 */
template<std::size_t max>
void FormatTrafficTitle(StaticString<max> &dest, uint32_t pilot_id,
                        FlarmId flarm_id,
                        const char *server_name,
                        const char *live_name=nullptr) noexcept;

} // namespace SkyLinesTracking

#include "TrafficDisplay.inl"
