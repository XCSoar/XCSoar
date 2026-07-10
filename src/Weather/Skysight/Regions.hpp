// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Language/Language.hpp"

#include <string>
#include <string_view>
#include <vector>

struct SkysightRegionInfo {
  const char *name;
  const char *id;
};

inline constexpr SkysightRegionInfo SKYSIGHT_REGIONS[] = {
  { N_("Europe"), "EUROPE" },
  { N_("South Africa"), "SANEW" },
  { N_("Western US"), "WEST_US" },
  { N_("Eastern US"), "EAST_US" },
  { N_("Argentina/Chile"), "ARGENTINA_CHILE" },
  { N_("Brazil"), "BRAZIL" },
  { N_("Japan"), "JAPAN" },
  { N_("New Zealand"), "NZ" },
  { N_("Western Australia"), "WA" },
  { N_("Eastern Australia"), "EAST_AUS" },
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
  return SKYSIGHT_REGIONS[0];
}

[[gnu::pure]]
static constexpr const SkysightRegionInfo &
FindSkysightRegionById(std::string_view region_id) noexcept
{
  for (const auto &region : SKYSIGHT_REGIONS)
    if (region_id == region.id)
      return region;

  return GetDefaultSkysightRegion();
}

inline void
ResetDefaultSkysightRegions(std::vector<SkysightRegionEntry> &result)
{
  result.clear();
  if (result.capacity() < std::size(SKYSIGHT_REGIONS))
    result.reserve(std::size(SKYSIGHT_REGIONS));

  for (const auto &region : SKYSIGHT_REGIONS)
    result.push_back({region.id, region.name, {}});
}

[[nodiscard]] inline std::vector<SkysightRegionEntry>
GetDefaultSkysightRegions()
{
  std::vector<SkysightRegionEntry> result;
  ResetDefaultSkysightRegions(result);
  return result;
}
