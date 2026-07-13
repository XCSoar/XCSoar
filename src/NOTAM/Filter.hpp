// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Settings.hpp"

#include <chrono>
#include <string_view>
#include <vector>

namespace NOTAMFilter {

enum class FilterReason : unsigned {
  IFR = 1u << 0,
  TIME = 1u << 1,
  QCODE = 1u << 2,
  RADIUS = 1u << 3,
};

using FilterReasons = unsigned;

[[nodiscard]]
constexpr bool
HasFilterReason(FilterReasons reasons, FilterReason reason) noexcept
{
  return (reasons & static_cast<FilterReasons>(reason)) != 0;
}

struct FilterStats {
  unsigned total = 0;
  unsigned filtered_by_ifr = 0;
  unsigned filtered_by_time = 0;
  unsigned filtered_by_qcode = 0;
  unsigned filtered_by_radius = 0;
  unsigned final_count = 0;
};

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
 * Evaluates all active filters and returns the matching suppression reasons.
 */
[[nodiscard]]
FilterReasons Evaluate(const struct NOTAM &notam,
                       const NOTAMSettings &settings,
                       std::chrono::system_clock::time_point now) noexcept;

/**
 * Returns false when @p notam is suppressed by IFR, effective-time, radius or
 * Q-code settings.  @p now is used for the effective-time check; @p log enables
 * debug logging of suppression reasons.
 */
[[nodiscard]]
bool ShouldDisplay(const struct NOTAM &notam,
                   const NOTAMSettings &settings,
                   std::chrono::system_clock::time_point now,
                   bool log) noexcept;

[[nodiscard]]
unsigned CountDisplayed(const std::vector<struct NOTAM> &notams,
                        const NOTAMSettings &settings,
                        std::chrono::system_clock::time_point now) noexcept;

[[nodiscard]]
FilterStats ComputeStats(const std::vector<struct NOTAM> &notams,
                         const NOTAMSettings &settings,
                         std::chrono::system_clock::time_point now) noexcept;

} // namespace NOTAMFilter
