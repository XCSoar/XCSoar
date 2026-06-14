// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

namespace EDL {

/**
 * Decode an EDL ascendancy tile pixel to vertical velocity in m/s.
 *
 * Matches the colour scale on https://www.edl-soaring.com/maps/ :
 * transparent pixels have no data.  Full yellow/cyan palette pixels use
 * alpha for magnitude; pale (r,r,0) and (0,g,g) ramps use RGB intensity.
 *
 * @return false only for fully transparent pixels
 */
[[gnu::const]]
bool
DecodeAscendancyPixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                      double &value_mps) noexcept;

} // namespace EDL
