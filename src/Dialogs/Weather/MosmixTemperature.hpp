// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/Temperature.hpp"

#include <optional>

/**
 * Fetch today's forecast maximum temperature, if that is due.
 *
 * Due means: today, and neither fetched already nor overridden by
 * hand -- see MOSMIX::ShouldFetchToday().  A fetch shows a modal
 * progress dialog the pilot can cancel.
 *
 * Failures are shown and swallowed: the flight setup dialog has
 * nothing to do about them but open with the value it already had.
 *
 * @return nothing when no fetch was due, none was possible, or the
 * forecast carries no value for today
 */
std::optional<Temperature>
MaybeFetchForecastTemperature() noexcept;
