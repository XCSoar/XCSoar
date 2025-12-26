// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Settings.hpp"

#include <chrono>
#include <string_view>

namespace NOTAMFilter {

/**
 * Returns true if @p qcode begins with any case-insensitive token from
 * @p hidden_list.  Tokens are separated by spaces, tabs or commas; wildcards
 * are not interpreted.
 */
[[nodiscard]]
[[gnu::pure]]
bool IsQCodeHidden(std::string_view qcode,
                   std::string_view hidden_list) noexcept;

/**
 * Returns false when @p notam is suppressed by IFR, effective-time, radius or
 * Q-code settings.  @p now is used for the effective-time check; @p log enables
 * debug logging of suppression reasons.
 */
[[nodiscard]]
bool ShouldDisplay(const NOTAM &notam,
                   const NOTAMSettings &settings,
                   std::chrono::system_clock::time_point now,
                   bool log) noexcept;

} // namespace NOTAMFilter
