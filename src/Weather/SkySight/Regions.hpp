// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

enum class SkySightRegion : unsigned {
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

struct SkySightRegionInfo {
  SkySightRegion value;
  const char *name;
  const char *id;
};

inline constexpr SkySightRegionInfo skysight_regions[] = {
  { SkySightRegion::EUROPE, "Europe", "EUROPE" },
  { SkySightRegion::SOUTH_AFRICA, "South Africa", "SANEW" },
  { SkySightRegion::WESTERN_US, "Western US", "WEST_US" },
  { SkySightRegion::EASTERN_US, "Eastern US", "EAST_US" },
  { SkySightRegion::ARGENTINA_CHILE, "Argentina/Chile", "ARGENTINA_CHILE" },
  { SkySightRegion::BRAZIL, "Brazil", "BRAZIL" },
  { SkySightRegion::JAPAN, "Japan", "JAPAN" },
  { SkySightRegion::NEW_ZEALAND, "New Zealand", "NZ" },
  { SkySightRegion::WESTERN_AUSTRALIA, "Western Australia", "WA" },
  { SkySightRegion::EASTERN_AUSTRALIA, "Eastern Australia", "EAST_AUS" },
};

[[gnu::const]]
static constexpr const SkySightRegionInfo &
GetDefaultSkySightRegion() noexcept
{
  return skysight_regions[0];
}

[[gnu::pure]]
static constexpr const SkySightRegionInfo &
FindSkySightRegionById(std::string_view region_id) noexcept
{
  for (const auto &region : skysight_regions)
    if (region_id == region.id)
      return region;

  return GetDefaultSkySightRegion();
}

[[gnu::pure]]
static constexpr const SkySightRegionInfo &
FindSkySightRegionByValue(unsigned value) noexcept
{
  for (const auto &region : skysight_regions)
    if (value == unsigned(region.value))
      return region;

  return GetDefaultSkySightRegion();
}