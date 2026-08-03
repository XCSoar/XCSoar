// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <cstdint>
#include <ctime>
#include <string_view>
#include <vector>

namespace SkySightCache {

inline constexpr char THROTTLE_CACHE_FILENAME[] = "throttle-v1.cache";

struct Usage {
  uint64_t bytes = 0;
  unsigned files = 0;
};

[[nodiscard]] Usage
GetUsage(Path directory) noexcept;

/**
 * Delete downloaded forecasts, tiles and temporary files while preserving
 * provider metadata and the persisted API throttle.
 *
 * @return the successfully deleted size and file count
 */
[[nodiscard]] Usage
ClearDownloadedData(Path directory) noexcept;

[[nodiscard]] bool
IsTrustedTimeAvailableForCleanup() noexcept;

struct ForecastImageCandidate {
  AllocatedPath path;
  time_t forecast_time = 0;
};

[[nodiscard]] ForecastImageCandidate
FindForecastImage(Path directory, std::string_view region,
                  std::string_view layer_id,
                  time_t preferred_time = 0);

[[nodiscard]] std::vector<time_t>
CollectForecastTimes(Path directory, std::string_view region,
                     std::string_view layer_id);

/**
 * Remove expired cache files.
 *
 * @return true if trusted time was available for forecast-age cleanup
 */
[[nodiscard]] bool
Cleanup(Path directory) noexcept;

} // namespace SkySightCache
