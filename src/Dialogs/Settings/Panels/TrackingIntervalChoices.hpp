// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

/** The intervals a tracking page can offer, shortest first. */
static constexpr unsigned TRACKING_INTERVAL_SECONDS[] = {
  1, 2, 3, 5, 10, 15, 20, 30, 45,
  60, 120, 180, 300, 600, 900, 1200, 1800, 2400, 3000, 3600,
};

static constexpr unsigned TRACKING_INTERVAL_COUNT =
  sizeof(TRACKING_INTERVAL_SECONDS) /
  sizeof(TRACKING_INTERVAL_SECONDS[0]);

void
FormatTrackingInterval(StaticString<16> &buffer,
                       unsigned seconds) noexcept;

[[gnu::pure]]
unsigned
ClosestTrackingInterval(unsigned value) noexcept;

/**
 * Let the user pick one of the tracking intervals.
 *
 * @return true if @p interval has changed
 */
bool
PickTrackingInterval(unsigned &interval) noexcept;
