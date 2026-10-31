// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/PortableColor.hpp"
#include "ui/canvas/Ramp.hpp"

extern const ColorRamp terrain_ramps[];

/**
 * One sample of a map terrain ramp.  Water uses the same blue as the
 * map terrain image.
 */
[[nodiscard]]
RGB8Color
LookupTerrainColor(unsigned ramp, int height, bool water) noexcept;
