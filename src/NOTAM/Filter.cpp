// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Filter.hpp"

#include "LogFile.hpp"
#include "util/ConvertString.hpp"

#include <chrono>
#include <cctype>

namespace NOTAMFilter {

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
    if (prefix_len > 0 &&
        qcode.compare(0, prefix_len,
                      hidden_list.substr(start, prefix_len)) == 0) {
      return true;
    }

    if (end == std::string_view::npos)
      break;

    start = hidden_list.find_first_not_of(" \t,", end + 1);
  }

  return false;
}

bool
ShouldDisplay(const NOTAM &notam, const NOTAMSettings &settings,
              bool log) noexcept
{
  // Check IFR filter (I=IFR-only, V=VFR-only, IV=both)
  if (!settings.show_ifr && !notam.traffic.empty() && notam.traffic == "I") {
    if (log) {
      LogDebug("NOTAM Filter: {} is IFR-only traffic ({}), filtered out",
               notam.number.c_str(), notam.traffic.c_str());
    }
    return false;
  }

  // Check if currently effective (if filter enabled)
  if (settings.show_only_effective) {
    if (!notam.IsActive()) {
      if (log) {
        LogDebug("NOTAM Filter: {} not currently effective, filtered out",
                 notam.number.c_str());
      }
      return false;
    }
  }

  // Check radius filter
  if (settings.max_radius_m > 0 &&
      notam.geometry.radius_meters > settings.max_radius_m) {
    if (log) {
      LogDebug("NOTAM Filter: {} radius {:.0f} m exceeds limit {} m, "
               "filtered out",
               notam.number.c_str(), notam.geometry.radius_meters,
               settings.max_radius_m);
    }
    return false;
  }

  const auto &qcode = notam.feature_type;
  if (qcode.empty())
    return true;

  if (log) {
    LogDebug("NOTAM Filter: {} Q-code='{}'", notam.number.c_str(),
             qcode.c_str());
  }

  WideToUTF8Converter hidden_conv(settings.hidden_qcodes.c_str());
  const char *hidden_list = hidden_conv.IsValid() ? hidden_conv.c_str() : "";
  if (IsQCodeHidden(qcode, hidden_list)) {
    if (log) {
      LogDebug("NOTAM Filter: {} Q-code {} is hidden",
               notam.number.c_str(), qcode.c_str());
    }
    return false;
  }

  return true;
}

} // namespace NOTAMFilter
