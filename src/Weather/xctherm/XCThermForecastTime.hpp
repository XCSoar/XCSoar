// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <vector>

namespace XCTherm {

struct UtcTimeParts {
  unsigned hour = 12;
  unsigned minute = 0;
};

/**
 * Current UTC hour/minute from the live blackboard, or 12:00 if unknown.
 */
[[gnu::pure]]
UtcTimeParts GetUtcTimeParts() noexcept;

/**
 * Pick the cached-hour vector index for auto-time mode (:45 rule).
 *
 * @return index into @p cached_hours, or -1 if empty.
 */
[[gnu::pure]]
int PickAutoTimeIndex(const std::vector<unsigned> &cached_hours,
                      unsigned utc_h, unsigned utc_min) noexcept;

/**
 * Forecast UTC hour from @p cached_hours at @p time_index, or a fallback.
 */
[[gnu::pure]]
unsigned ForecastHourAt(const std::vector<unsigned> &cached_hours,
                        unsigned time_index,
                        const std::vector<unsigned> &available_hours,
                        unsigned fallback = 12) noexcept;

} // namespace XCTherm
