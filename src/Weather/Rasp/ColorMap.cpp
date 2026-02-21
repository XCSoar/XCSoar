// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ColorMap.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

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
ComputeHash(const StaticArray<ColorRampEntry,
                              MAX_COLOR_MAP_POINTS> &entries,
            const StaticArray<ColorRampEntryAlpha,
                              MAX_COLOR_MAP_POINTS> &entries_alpha,
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

/**
 * Apply the linear value transform and round the result to the ramp
 * "h" domain.  ColorRampLookup() requires strictly increasing h
 * values (it divides by the distance between two adjacent entries),
 * but rounding may collapse control points which are close together;
 * widen those by one unit.
 *
 * @param previous the preceding entry's h, or nullptr for the first
 */
static short
MaterializeRampH(float value, float scale, float offset,
                 const short *previous) noexcept
{
  constexpr long min_h = std::numeric_limits<short>::min();
  constexpr long max_h = std::numeric_limits<short>::max();

  long h = std::clamp(std::lround(value * scale + offset),
                      min_h, max_h);

  if (previous != nullptr && h <= long(*previous))
    h = std::min(long(*previous) + 1, max_h);

  assert(previous == nullptr || h > long(*previous));

  return static_cast<short>(h);
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
  assert(color_map.num_points <= MAX_COLOR_MAP_POINTS);
  assert(color_map.points != nullptr);

  MaterializedColorRamp result;
  result.has_alpha = color_map_alpha.num_points > 0;

  // Materialize RGB entries (drop alpha channel)
  for (unsigned i = 0; i < color_map.num_points; ++i) {
    const auto &p = color_map.points[i];
    const short h = MaterializeRampH(p.value, scale, offset,
                                     i > 0
                                     ? &result.entries[i - 1].h
                                     : nullptr);
    result.entries.push_back({h, static_cast<RGB8Color>(p.color)});
  }

  // Materialize RGBA entries if alpha map provided
  if (result.has_alpha) {
    assert(color_map_alpha.points != nullptr);

    for (unsigned i = 0; i < color_map_alpha.num_points; ++i) {
      const auto &p = color_map_alpha.points[i];
      const short h = MaterializeRampH(p.value, scale, offset,
                                       i > 0
                                       ? &result.entries_alpha[i - 1].h
                                       : nullptr);
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
