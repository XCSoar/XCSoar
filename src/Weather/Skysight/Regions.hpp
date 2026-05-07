// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>
#include <string_view>
#include <vector>

struct SkysightRegionInfo {
  const char *name;
  const char *id;
};

inline constexpr SkysightRegionInfo skysight_regions[] = {
  { "Europe", "EUROPE" },
  { "South Africa", "SANEW" },
  { "Western US", "WEST_US" },
  { "Eastern US", "EAST_US" },
  { "Argentina/Chile", "ARGENTINA_CHILE" },
  { "Brazil", "BRAZIL" },
  { "Japan", "JAPAN" },
  { "New Zealand", "NZ" },
  { "Western Australia", "WA" },
  { "Eastern Australia", "EAST_AUS" },
};

struct SkysightRegionEntry {
  std::string id;
  std::string name;
  std::string projection;

  bool operator==(std::string_view other) const noexcept {
    return id == other;
  }
};

[[gnu::const]]
static constexpr const SkysightRegionInfo &
GetDefaultSkysightRegion() noexcept
{
  return skysight_regions[0];
}

[[gnu::pure]]
static constexpr const SkysightRegionInfo &
FindSkysightRegionById(std::string_view region_id) noexcept
{
  for (const auto &region : skysight_regions)
    if (region_id == region.id)
      return region;

  return GetDefaultSkysightRegion();
}

inline void
ResetDefaultSkysightRegions(std::vector<SkysightRegionEntry> &result)
{
  result.clear();
  if (result.capacity() < std::size(skysight_regions))
    result.reserve(std::size(skysight_regions));

  for (const auto &region : skysight_regions)
    result.push_back({region.id, region.name, {}});
}

[[nodiscard]] inline std::vector<SkysightRegionEntry>
GetDefaultSkysightRegions()
{
  std::vector<SkysightRegionEntry> result;
  ResetDefaultSkysightRegions(result);
  return result;
}
