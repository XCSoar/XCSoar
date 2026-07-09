// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermCatalog.hpp"
#include "XCThermAPI.hpp"
#include "Weather/Settings.hpp"

#include <iterator>

namespace XCTherm {

int
FindActiveLayerIndex(const XCThermSettings &settings) noexcept
{
  const auto &region = GetRegion(settings.model);
  for (unsigned i = 0; i < region.layer_count; ++i) {
    if (IsActiveLayer(region.layers[i], settings.parameter,
                      settings.wave_height, settings.vertical_wind_agl))
      return int(i);
  }
  return -1;
}

int
FindFirstCachedLayerIndex(const XCThermSettings &settings) noexcept
{
  auto &api = XCThermAPI::Instance();
  const auto &region = GetRegion(settings.model);
  for (unsigned i = 0; i < region.layer_count; ++i) {
    if (!api.GetCachedHours(region.layers[i].api_parameter).empty())
      return int(i);
  }
  return -1;
}

int
ResolveDisplayLayerIndex(const XCThermSettings &settings) noexcept
{
  const int active = FindActiveLayerIndex(settings);
  if (active >= 0) {
    const auto &region = GetRegion(settings.model);
    if (!XCThermAPI::Instance()
          .GetCachedHours(region.layers[unsigned(active)].api_parameter)
          .empty())
      return active;
  }

  return FindFirstCachedLayerIndex(settings);
}

} // namespace XCTherm
