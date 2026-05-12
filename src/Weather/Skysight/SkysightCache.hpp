// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <ctime>
#include <string_view>

namespace SkysightCache {

struct ForecastImageCandidate {
  AllocatedPath path;
  time_t forecast_time = 0;
};

[[nodiscard]] ForecastImageCandidate
FindForecastImage(Path directory, std::string_view region,
                  std::string_view layer_id);

void
Cleanup(Path directory) noexcept;

} // namespace SkysightCache
