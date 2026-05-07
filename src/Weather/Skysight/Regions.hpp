// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

enum class SkysightRegion : unsigned {
  EUROPE,
  SOUTH_AFRICA,
  WESTERN_US,
  EASTERN_US,
  ARGENTINA_CHILE,
  BRAZIL,
  JAPAN,
  NEW_ZEALAND,
  WESTERN_AUSTRALIA,
  EASTERN_AUSTRALIA,
};

struct SkysightRegionInfo {
  SkysightRegion value;
  const char *name;
  const char *id;
};

inline constexpr SkysightRegionInfo skysight_regions[] = {
  { SkysightRegion::EUROPE, "Europe", "EUROPE" },
  { SkysightRegion::SOUTH_AFRICA, "South Africa", "SANEW" },
  { SkysightRegion::WESTERN_US, "Western US", "WEST_US" },
  { SkysightRegion::EASTERN_US, "Eastern US", "EAST_US" },
  { SkysightRegion::ARGENTINA_CHILE, "Argentina/Chile", "ARGENTINA_CHILE" },
  { SkysightRegion::BRAZIL, "Brazil", "BRAZIL" },
  { SkysightRegion::JAPAN, "Japan", "JAPAN" },
  { SkysightRegion::NEW_ZEALAND, "New Zealand", "NZ" },
  { SkysightRegion::WESTERN_AUSTRALIA, "Western Australia", "WA" },
  { SkysightRegion::EASTERN_AUSTRALIA, "Eastern Australia", "EAST_AUS" },
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

[[gnu::pure]]
static constexpr const SkysightRegionInfo &
FindSkysightRegionByValue(unsigned value) noexcept
{
  for (const auto &region : skysight_regions)
    if (value == unsigned(region.value))
      return region;

  return GetDefaultSkysightRegion();
}