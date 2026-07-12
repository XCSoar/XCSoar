// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/custom/GeoBitmap.hpp"

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace SkySight {

enum class ForecastTimeMode : uint8_t {
  AutoDefault,
  Fixed,
};

enum class ForecastProgressPhase : uint8_t {
  Metadata,
  Download,
  Decode,
  Throttled,
  Complete,
};

struct ForecastProgress {
  ForecastProgressPhase phase = ForecastProgressPhase::Metadata;
  unsigned total = 0;
  unsigned completed = 0;
  unsigned failed = 0;
  unsigned retry_seconds = 0;
};

struct LegendColor {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;

  constexpr LegendColor() noexcept = default;

  constexpr LegendColor(uint8_t _red, uint8_t _green,
                        uint8_t _blue) noexcept
    :red(_red), green(_green), blue(_blue) {}
};

struct ForecastDatafile {
  time_t time = 0;
  std::string link;

  ForecastDatafile() = default;

  ForecastDatafile(time_t _time, std::string _link) noexcept
    :time(_time), link(std::move(_link)) {}
};

struct Layer {
  std::string id;
  std::string name;
  std::string description;
  std::string projection;
  std::string data_type;
  std::map<float, LegendColor> legend;
  std::string time_name;
  std::vector<ForecastDatafile> forecast_datafiles;
  double from = 0;
  double to = 0;
  double mtime = 0;
  bool requires_auth = false;
  /** True while the UI should present this layer as busy. */
  bool updating = false;
  /** Metadata request for available forecast steps is still pending. */
  bool datafiles_pending = false;
  /** A downloaded forecast file is being decoded into an overlay image. */
  bool decoding = false;
  bool preload_requested = false;
  bool default_preload_requested = false;
  bool tile_layer = false;
  bool live_layer = false;
  /** Number of queued/download/decode jobs still outstanding. */
  unsigned pending_downloads = 0;
  unsigned zoom_min = 1;
  unsigned zoom_max = GeoBitmap::MAX_TILE_ZOOM;
  float alpha = 0.6f;
  time_t last_update = 0;
  time_t forecast_time = 0;
  ForecastTimeMode forecast_time_mode = ForecastTimeMode::AutoDefault;

  Layer() = default;

  Layer(std::string _id, std::string _name, std::string _description,
        bool _requires_auth, bool _live_layer, bool _tile_layer,
        unsigned _zoom_min = 1,
        unsigned _zoom_max = GeoBitmap::MAX_TILE_ZOOM,
        float _alpha = 0.6f) noexcept
    :id(std::move(_id)),
     name(std::move(_name)),
     description(std::move(_description)),
     requires_auth(_requires_auth),
     tile_layer(_tile_layer),
     live_layer(_live_layer),
     zoom_min(_zoom_min),
     zoom_max(_zoom_max),
     alpha(_alpha) {}

  [[nodiscard]] bool SupportsLiveTiles() const noexcept {
    return live_layer && tile_layer;
  }

  [[nodiscard]] bool UsesAutomaticForecastTime() const noexcept {
    return forecast_time_mode == ForecastTimeMode::AutoDefault;
  }

  /**
   * Forecast layers may keep metadata loading in the background while cached
   * steps or rendered data are already usable in the UI.
   */
  [[nodiscard]] bool HasUsableForecastData() const noexcept {
    return !SupportsLiveTiles() && (!forecast_datafiles.empty() || mtime != 0);
  }

  /**
   * A forecast layer is only shown as "updating" for metadata fetches if no
   * cached data is usable yet; active downloads/decodes are always busy.
   */
  [[nodiscard]] bool ShouldShowUpdating() const noexcept {
    return decoding || pending_downloads > 0 ||
      (datafiles_pending && !HasUsableForecastData());
  }

  bool operator==(std::string_view other) const noexcept {
    return !other.empty() && id == other;
  }
};

} // namespace SkySight
