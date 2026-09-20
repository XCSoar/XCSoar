// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct BrokenDate;

namespace MOSMIX {

/**
 * Should a forecast be fetched today?
 *
 * False once one has been fetched today, and false for the rest of
 * the day once the pilot has set the temperature by hand: a value
 * entered deliberately outranks a forecast, but only until the next
 * day, when the forecast is about a different sky.
 *
 * Both answers are remembered in the profile rather than in memory,
 * so restarting between two flights on the same day does not start
 * the day over.
 */
[[gnu::pure]]
bool ShouldFetchToday(const BrokenDate &today) noexcept;

/** A forecast was fetched today; do not fetch another. */
void RememberFetch(const BrokenDate &today) noexcept;

/**
 * The pilot set the temperature by hand.  Stops the automatic update
 * for the rest of the day.
 */
void RememberManualEntry(const BrokenDate &today) noexcept;

} // namespace MOSMIX
