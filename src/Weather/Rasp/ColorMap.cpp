// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ColorMap.hpp"

#include <cassert>
#include <cmath>

static uint32_t
HashUpdate(uint32_t hash, const void *data, size_t size) noexcept
{
  const auto *bytes = static_cast<const uint8_t *>(data);
  for (size_t i = 0; i < size; ++i) {
    hash ^= bytes[i];
    hash *= 0x01000193u; // FNV-1a prime
  }
  return hash;
}

static uint32_t
ComputeHash(const std::vector<ColorRampEntry> &entries,
            const std::vector<ColorRampEntryAlpha> &entries_alpha,
            bool has_alpha,
            unsigned height_scale, bool do_water) noexcept
{
  uint32_t hash = 0x811c9dc5u; // FNV-1a offset basis

  for (const auto &e : entries) {
    hash = HashUpdate(hash, &e.h, sizeof(e.h));
    hash = HashUpdate(hash, &e.color, sizeof(e.color));
  }

  for (const auto &e : entries_alpha) {
    hash = HashUpdate(hash, &e.h, sizeof(e.h));
    hash = HashUpdate(hash, &e.color, sizeof(e.color));
  }

  hash = HashUpdate(hash, &height_scale, sizeof(height_scale));

  uint8_t flags = (has_alpha ? 1u : 0u)
    | (do_water ? 2u : 0u);
  hash = HashUpdate(hash, &flags, sizeof(flags));

  return hash;
}

ColorRamp
MaterializedColorRamp::GetColorRamp() const noexcept
{
  return {
    has_alpha,
    static_cast<short>(entries.size()),
    entries.data(),
    has_alpha ? entries_alpha.data() : nullptr,
  };
}

MaterializedColorRamp
MaterializeColorRamp(const ColorMap &color_map,
                     const ColorMap &color_map_alpha,
                     float scale, float offset,
                     unsigned height_scale,
                     bool do_water) noexcept
{
  assert(color_map.num_points >= 2);
  assert(color_map.points != nullptr);

  MaterializedColorRamp result;
  result.has_alpha = color_map_alpha.num_points > 0;

  // Materialize RGB entries (drop alpha channel)
  result.entries.reserve(color_map.num_points);
  for (unsigned i = 0; i < color_map.num_points; ++i) {
    const auto &p = color_map.points[i];
    short h = static_cast<short>(
      std::lround(p.value * scale + offset));
    result.entries.push_back({h, static_cast<RGB8Color>(p.color)});
  }

  // Materialize RGBA entries if alpha map provided
  if (result.has_alpha) {
    assert(color_map_alpha.points != nullptr);

    result.entries_alpha.reserve(color_map_alpha.num_points);
    for (unsigned i = 0; i < color_map_alpha.num_points; ++i) {
      const auto &p = color_map_alpha.points[i];
      short h = static_cast<short>(
        std::lround(p.value * scale + offset));
      result.entries_alpha.push_back({h, p.color});
    }

    assert(result.entries.size()
           == result.entries_alpha.size());
  }

  result.hash = ComputeHash(result.entries, result.entries_alpha,
                            result.has_alpha,
                            height_scale, do_water);

  return result;
}
