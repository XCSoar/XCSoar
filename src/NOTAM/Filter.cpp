// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Filter.hpp"
#include "LogFile.hpp"

#include <cctype>

namespace NOTAMFilter {

[[gnu::pure]]
bool
IsQCodeHidden(std::string_view qcode,
              std::string_view hidden_list) noexcept
{
  if (qcode.empty() || hidden_list.empty())
    return false;

  size_t start = hidden_list.find_first_not_of(" \t,");
  while (start != std::string_view::npos) {
    const size_t end = hidden_list.find_first_of(" \t,", start);
    const size_t prefix_len = end == std::string_view::npos
      ? hidden_list.size() - start
      : end - start;
    if (prefix_len > 0 && qcode.size() >= prefix_len) {
      bool matches = true;
      for (size_t i = 0; i < prefix_len; ++i) {
        const unsigned char q = static_cast<unsigned char>(qcode[i]);
        const unsigned char h =
          static_cast<unsigned char>(hidden_list[start + i]);
        if (std::tolower(q) != std::tolower(h)) {
          matches = false;
          break;
        }
      }

      if (matches)
        return true;
    }

    if (end == std::string_view::npos)
      break;

    start = hidden_list.find_first_not_of(" \t,", end + 1);
  }

  return false;
}

FilterReasons
Evaluate(const struct NOTAM &notam, const NOTAMSettings &settings,
         std::chrono::system_clock::time_point now) noexcept
{
  FilterReasons reasons = 0;

  if (!settings.show_ifr && !notam.traffic.empty() && notam.traffic == "I")
    reasons |= static_cast<FilterReasons>(FilterReason::IFR);

  if (settings.show_only_effective && !notam.IsActive(now))
    reasons |= static_cast<FilterReasons>(FilterReason::TIME);

  if (settings.max_radius_m > 0 &&
      notam.geometry.radius_meters > settings.max_radius_m)
    reasons |= static_cast<FilterReasons>(FilterReason::RADIUS);

  if (!notam.feature_type.empty() &&
      IsQCodeHidden(notam.feature_type, settings.hidden_qcodes))
    reasons |= static_cast<FilterReasons>(FilterReason::QCODE);

  return reasons;
}

bool
ShouldDisplay(const struct NOTAM &notam, const NOTAMSettings &settings,
              std::chrono::system_clock::time_point now,
              bool log) noexcept
{
  const auto reasons = Evaluate(notam, settings, now);

  if (HasFilterReason(reasons, FilterReason::IFR)) {
    if (log) {
      LogDebug("NOTAM Filter: {} is IFR-only traffic ({}), filtered out",
               notam.number.c_str(), notam.traffic.c_str());
    }
    return false;
  }

  if (HasFilterReason(reasons, FilterReason::TIME)) {
    if (log) {
      LogDebug("NOTAM Filter: {} not currently effective, filtered out",
               notam.number.c_str());
    }
    return false;
  }

  if (HasFilterReason(reasons, FilterReason::RADIUS)) {
    if (log) {
      LogDebug("NOTAM Filter: {} radius {:.0f} m exceeds limit {} m, "
               "filtered out",
               notam.number.c_str(), notam.geometry.radius_meters,
               settings.max_radius_m);
    }
    return false;
  }

  const auto &qcode = notam.feature_type;
  if (!qcode.empty() && log) {
    LogDebug("NOTAM Filter: {} Q-code='{}'", notam.number.c_str(),
             qcode.c_str());
  }

  if (HasFilterReason(reasons, FilterReason::QCODE)) {
    if (log) {
      LogDebug("NOTAM Filter: {} Q-code {} is hidden",
               notam.number.c_str(), qcode.c_str());
    }
    return false;
  }

  return reasons == 0;
}

unsigned
CountDisplayed(const std::vector<struct NOTAM> &notams,
               const NOTAMSettings &settings,
               const std::chrono::system_clock::time_point now) noexcept
{
  unsigned count = 0;
  for (const auto &notam : notams) {
    if (ShouldDisplay(notam, settings, now, false))
      ++count;
  }
  return count;
}

FilterStats
ComputeStats(const std::vector<struct NOTAM> &notams,
             const NOTAMSettings &settings,
             const std::chrono::system_clock::time_point now) noexcept
{
  FilterStats stats;
  stats.total = static_cast<unsigned>(notams.size());

  for (const auto &notam : notams) {
    const auto reasons = Evaluate(notam, settings, now);
    if (HasFilterReason(reasons, FilterReason::IFR))
      ++stats.filtered_by_ifr;

    if (HasFilterReason(reasons, FilterReason::TIME))
      ++stats.filtered_by_time;

    if (HasFilterReason(reasons, FilterReason::QCODE))
      ++stats.filtered_by_qcode;

    if (HasFilterReason(reasons, FilterReason::RADIUS))
      ++stats.filtered_by_radius;

    if (reasons == 0)
      ++stats.final_count;
  }

  return stats;
}

} // namespace NOTAMFilter
