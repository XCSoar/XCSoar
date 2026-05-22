// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/Settings.hpp"

#include <cstddef>
#include <string>

namespace XCThermLayers {

struct Layer {
  const char *label;
  const char *short_label;
  const char *file_suffix;
  unsigned altitude_m;
  bool is_agl;
};

extern const Layer CH_LAYERS[];
extern const Layer UK_LAYERS[];

constexpr std::size_t CH_COUNT = 10;
constexpr std::size_t UK_COUNT = 10;

[[gnu::pure]]
bool
IsUK(XCThermRegion region) noexcept;

[[gnu::pure]]
const Layer *
Get(XCThermRegion region, std::size_t &count) noexcept;

[[gnu::pure]]
const char *
ModelString(XCThermRegion region) noexcept;

std::string
BuildApiParameter(const Layer &layer) noexcept;

[[gnu::pure]]
bool
IsActive(const Layer &layer,
         unsigned active_parameter,
         unsigned active_wave_height,
         unsigned active_vertical_agl) noexcept;

} // namespace XCThermLayers
