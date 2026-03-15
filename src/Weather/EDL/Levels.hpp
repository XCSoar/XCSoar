// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace EDL {

static constexpr unsigned NUM_ISOBARS = 5;
static constexpr unsigned ISOBARS[NUM_ISOBARS] = {
  50000,
  60000,
  70000,
  80000,
  90000,
};

/**
 * Check whether the specified pressure level is supported by the EDL service.
 */
[[gnu::pure]]
bool
IsSupportedIsobar(unsigned isobar) noexcept;

/**
 * Resolve the nearest supported EDL isobar from the current aircraft altitude.
 */
[[gnu::pure]]
unsigned
ResolveCurrentLevel() noexcept;

} // namespace EDL
