// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

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
 * Forecast UTC hour selected by auto-time mode (:45 switches to next hour).
 */
[[gnu::const]]
unsigned PickAutoTargetUtcHour(unsigned utc_h, unsigned utc_min) noexcept;

/**
 * Pick the cached-hour vector index closest to #PickAutoTargetUtcHour().
 *
 * @return index into @p cached_hours, or -1 if empty.
 */
[[gnu::pure]]
int PickAutoTimeIndex(const std::vector<unsigned> &cached_hours,
                      unsigned utc_h, unsigned utc_min) noexcept;

/**
 * Forecast UTC hour for cursor-bar @p time_index.
 *
 * Prefers @p available_hours (index catalog), then @p cached_hours, then
 * wraps @p time_index across a 24 h UTC day.
 */
[[gnu::pure]]
unsigned ForecastHourAt(const std::vector<unsigned> &cached_hours,
                        unsigned time_index,
                        const std::vector<unsigned> &available_hours,
                        unsigned fallback = 12) noexcept;

/**
 * Cursor-bar time label, e.g. @c "12:00 UTC (+1:30)" or @c "AUTO: …".
 */
void FormatTimeLabel(StaticString<64> &dest, const char *api_parameter,
                     unsigned forecast_utc_hour,
                     bool time_auto_active) noexcept;

} // namespace XCTherm
