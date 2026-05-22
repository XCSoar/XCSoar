// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Layers.hpp"

namespace XCThermLayers {

const Layer CH_LAYERS[] = {
  { "Vertical wind 1500 m AMSL", "1500m AMSL", "1500amsl", 1500, false },
  { "Vertical wind 2000 m AMSL", "2000m AMSL", "2000amsl", 2000, false },
  { "Vertical wind 3000 m AMSL", "3000m AMSL", "3000amsl", 3000, false },
  { "Vertical wind 4000 m AMSL", "4000m AMSL", "4000amsl", 4000, false },
  { "Vertical wind 5000 m AMSL", "5000m AMSL", "5000amsl", 5000, false },
  { "Vertical wind 6000 m AMSL", "6000m AMSL", "6000amsl", 6000, false },
  { "Vertical wind 7000 m AMSL", "7000m AMSL", "7000amsl", 7000, false },
  { "Vertical wind 8000 m AMSL", "8000m AMSL", "8000amsl", 8000, false },
  { "Vertical wind 100 m AGL",   "100m AGL",   "100agl",   100,  true },
  { "Vertical wind 400 m AGL",   "400m AGL",   "400agl",   400,  true },
};

const Layer UK_LAYERS[] = {
  { "Vertical wind 1000 m AMSL", "1000m AMSL", "1000amsl", 1000, false },
  { "Vertical wind 1500 m AMSL", "1500m AMSL", "1500amsl", 1500, false },
  { "Vertical wind 2000 m AMSL", "2000m AMSL", "2000amsl", 2000, false },
  { "Vertical wind 2500 m AMSL", "2500m AMSL", "2500amsl", 2500, false },
  { "Vertical wind 3000 m AMSL", "3000m AMSL", "3000amsl", 3000, false },
  { "Vertical wind 4200 m AMSL", "4200m AMSL", "4200amsl", 4200, false },
  { "Vertical wind 100 m AGL",   "100m AGL",   "100agl",   100,  true },
  { "Vertical wind 200 m AGL",   "200m AGL",   "200agl",   200,  true },
  { "Vertical wind 400 m AGL",   "400m AGL",   "400agl",   400,  true },
  { "Vertical wind 800 m AGL",   "800m AGL",   "800agl",   800,  true },
};

static_assert(std::size(CH_LAYERS) == CH_COUNT, "");
static_assert(std::size(UK_LAYERS) == UK_COUNT, "");

bool
IsUK(XCThermRegion region) noexcept
{
  return region == XCThermRegion::UK;
}

const Layer *
Get(XCThermRegion region, std::size_t &count) noexcept
{
  if (IsUK(region)) {
    count = UK_COUNT;
    return UK_LAYERS;
  }

  count = CH_COUNT;
  return CH_LAYERS;
}

const char *
ModelString(XCThermRegion region) noexcept
{
  return IsUK(region) ? "icon-uk" : "icon-ch";
}

std::string
BuildApiParameter(const Layer &layer) noexcept
{
  return std::string("vertical_wind_") + layer.file_suffix;
}

bool
IsActive(const Layer &layer,
         unsigned active_parameter,
         unsigned active_wave_height,
         unsigned active_vertical_agl) noexcept
{
  if (layer.is_agl)
    return active_parameter == 1 && layer.altitude_m == active_vertical_agl;
  else
    return active_parameter == 0 && layer.altitude_m == active_wave_height;
}

} // namespace XCThermLayers
