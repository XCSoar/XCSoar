// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/custom/GeoBitmap.hpp"

#include <cstdint>
#include <ctime>
#include <string>
#include <string_view>

namespace SkySight {

enum class LiveMetadataSupport : uint8_t {
  Unknown,
  Supported,
  Unsupported,
};

struct Layer {
  std::string id;
  std::string name;
  std::string description;
  bool requires_auth = false;
  bool tile_layer = true;
  unsigned zoom_min = 1;
  unsigned zoom_max = GeoBitmap::MAX_TILE_ZOOM;
  float alpha = 0.6f;
  time_t last_update = 0;
  /** The current live timestamp was discovered by a successful tile probe. */
  bool live_timestamp_from_probe = false;
  /** Latest /data/last_updated request start for this live layer. */
  time_t last_update_request = 0;
  /** Whether /data/last_updated actually returns this pseudo-layer. */
  LiveMetadataSupport live_metadata_support = LiveMetadataSupport::Unknown;

  [[nodiscard]] bool HasKnownLiveTimestamp() const noexcept {
    return last_update > 0;
  }

  [[nodiscard]] bool
  IsLiveMetadataPollDue(time_t now, time_t initial_interval,
                        time_t regular_interval) const noexcept
  {
    if (live_metadata_support == LiveMetadataSupport::Unsupported)
      return false;

    const auto interval = last_update != 0
      ? regular_interval
      : initial_interval;
    return last_update_request == 0 ||
      now >= last_update_request + interval;
  }

  bool operator==(std::string_view other) const noexcept {
    return id == other;
  }
};

} // namespace SkySight
