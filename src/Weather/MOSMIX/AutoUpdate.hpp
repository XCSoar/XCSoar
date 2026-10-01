// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/Temperature.hpp"

#include <optional>

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

/**
 * The forecast fetched today, if one was and it carried a value.
 *
 * Kept so that opening the dialog a second time on the same day shows
 * the same number without asking the network again -- and so that
 * cancelling the dialog does not throw the answer away.
 */
[[gnu::pure]]
std::optional<Temperature>
GetStoredForecast(const BrokenDate &today) noexcept;

/**
 * A forecast was fetched today; do not fetch another.
 *
 * @param value what it said, or nothing when it carried no value for
 * today -- which still counts as the day's fetch, because asking
 * again would only repeat the answer
 */
void RememberFetch(const BrokenDate &today,
                   std::optional<Temperature> value) noexcept;

/**
 * The pilot set the temperature by hand.  Stops the automatic update
 * for the rest of the day.
 */
void RememberManualEntry(const BrokenDate &today) noexcept;

} // namespace MOSMIX
