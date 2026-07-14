// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/custom/GeoBitmap.hpp"

#include <string>
#include <string_view>

namespace SkySight {

struct Layer {
  std::string id;
  std::string name;
  std::string description;
  bool requires_auth = false;
  bool tile_layer = true;
  unsigned zoom_min = 1;
  unsigned zoom_max = GeoBitmap::MAX_TILE_ZOOM;
  float alpha = 0.6f;

  bool operator==(std::string_view other) const noexcept {
    return id == other;
  }
};

} // namespace SkySight
