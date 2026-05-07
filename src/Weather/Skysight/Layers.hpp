// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/custom/GeoBitmap.hpp"

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <string_view>

namespace SkySight {

struct LegendColor {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;

  constexpr LegendColor() noexcept = default;

  constexpr LegendColor(uint8_t _red, uint8_t _green,
                        uint8_t _blue) noexcept
    :red(_red), green(_green), blue(_blue) {}
};

struct Layer {
  std::string id;
  std::string name;
  std::string description;
  std::string projection;
  std::string data_type;
  std::map<float, LegendColor> legend;
  std::string time_name;
  double from = 0;
  double to = 0;
  double mtime = 0;
  bool requires_auth = false;
  bool updating = false;
  bool tile_layer = false;
  bool live_layer = false;
  unsigned zoom_min = 1;
  unsigned zoom_max = GeoBitmap::MAX_TILE_ZOOM;
  float alpha = 0.6f;
  time_t last_update = 0;
  time_t forecast_time = 0;

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

  bool operator==(std::string_view other) const noexcept {
    return !other.empty() && id == other;
  }
};

} // namespace SkySight
