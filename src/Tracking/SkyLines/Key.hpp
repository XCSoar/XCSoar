// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

namespace SkyLinesTracking {

/**
 * Generate a new SkyLines tracking key that aims to be unique.
 */
uint64_t
GenerateKey();

} /* namespace SkyLinesTracking */
