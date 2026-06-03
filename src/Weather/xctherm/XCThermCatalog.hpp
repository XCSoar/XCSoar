// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

namespace XCTherm {

/**
 * Forecast region — stored in profile as @c settings.xctherm.model
 * (unsigned). Must match @c StaticEnumChoice order in
 * WeatherConfigPanel.
 */
enum class Region : unsigned {
  CH = 0,
  UK = 1,
  COUNT,
};

struct Layer {
  /** API path segment, e.g. @c "5000amsl". */
  const char *file_suffix;

  /** Full index.json parameter name, e.g. @c "vertical_wind_5000amsl". */
  const char *api_parameter;

  /** Dialog list row (English; list widget caption). */
  const char *dialog_label;

  /** Cursor bar center label, e.g. @c "5000m AMSL". */
  const char *short_label;

  unsigned altitude_m;
  bool is_agl;
};

struct RegionDef {
  /** Tiles URL model slug, e.g. @c "icon-ch". */
  const char *api_slug;
  const Layer *layers;
  unsigned layer_count;
};

namespace detail {

constexpr Layer CH_LAYERS[] = {
  { "1500amsl", "vertical_wind_1500amsl", "Vertical wind 1500 m AMSL",
    "1500m AMSL", 1500, false },
  { "2000amsl", "vertical_wind_2000amsl", "Vertical wind 2000 m AMSL",
    "2000m AMSL", 2000, false },
  { "3000amsl", "vertical_wind_3000amsl", "Vertical wind 3000 m AMSL",
    "3000m AMSL", 3000, false },
  { "4000amsl", "vertical_wind_4000amsl", "Vertical wind 4000 m AMSL",
    "4000m AMSL", 4000, false },
  { "5000amsl", "vertical_wind_5000amsl", "Vertical wind 5000 m AMSL",
    "5000m AMSL", 5000, false },
  { "6000amsl", "vertical_wind_6000amsl", "Vertical wind 6000 m AMSL",
    "6000m AMSL", 6000, false },
  { "7000amsl", "vertical_wind_7000amsl", "Vertical wind 7000 m AMSL",
    "7000m AMSL", 7000, false },
  { "8000amsl", "vertical_wind_8000amsl", "Vertical wind 8000 m AMSL",
    "8000m AMSL", 8000, false },
  { "100agl", "vertical_wind_100agl", "Vertical wind 100 m AGL",
    "100m AGL", 100, true },
  { "400agl", "vertical_wind_400agl", "Vertical wind 400 m AGL",
    "400m AGL", 400, true },
};

constexpr Layer UK_LAYERS[] = {
  { "1000amsl", "vertical_wind_1000amsl", "Vertical wind 1000 m AMSL",
    "1000m AMSL", 1000, false },
  { "1500amsl", "vertical_wind_1500amsl", "Vertical wind 1500 m AMSL",
    "1500m AMSL", 1500, false },
  { "2000amsl", "vertical_wind_2000amsl", "Vertical wind 2000 m AMSL",
    "2000m AMSL", 2000, false },
  { "2500amsl", "vertical_wind_2500amsl", "Vertical wind 2500 m AMSL",
    "2500m AMSL", 2500, false },
  { "3000amsl", "vertical_wind_3000amsl", "Vertical wind 3000 m AMSL",
    "3000m AMSL", 3000, false },
  { "4200amsl", "vertical_wind_4200amsl", "Vertical wind 4200 m AMSL",
    "4200m AMSL", 4200, false },
  { "100agl", "vertical_wind_100agl", "Vertical wind 100 m AGL",
    "100m AGL", 100, true },
  { "200agl", "vertical_wind_200agl", "Vertical wind 200 m AGL",
    "200m AGL", 200, true },
  { "400agl", "vertical_wind_400agl", "Vertical wind 400 m AGL",
    "400m AGL", 400, true },
  { "800agl", "vertical_wind_800agl", "Vertical wind 800 m AGL",
    "800m AGL", 800, true },
};

constexpr RegionDef REGIONS[] = {
  { "icon-ch", CH_LAYERS, unsigned(std::size(CH_LAYERS)) },
  { "icon-uk", UK_LAYERS, unsigned(std::size(UK_LAYERS)) },
};

static_assert(std::size(REGIONS) == unsigned(Region::COUNT),
              "REGIONS must match Region::COUNT");

} // namespace detail

constexpr unsigned MaxLayerCount() noexcept {
  unsigned n = 0;
  for (const auto &region : detail::REGIONS)
    if (region.layer_count > n)
      n = region.layer_count;
  return n;
}

[[gnu::pure]]
constexpr Region ToRegion(unsigned model_id) noexcept {
  if (model_id >= unsigned(Region::COUNT))
    return Region::CH;
  return Region(model_id);
}

[[gnu::pure]]
constexpr const RegionDef &GetRegion(Region region) noexcept {
  return detail::REGIONS[unsigned(region)];
}

[[gnu::pure]]
constexpr const RegionDef &GetRegion(unsigned model_id) noexcept {
  return GetRegion(ToRegion(model_id));
}

/**
 * Index of @p altitude_m in the region's layer table, or -1 if absent.
 */
[[gnu::pure]]
constexpr int FindLayerIndex(Region region, unsigned altitude_m,
                             bool is_agl) noexcept {
  const auto &def = GetRegion(region);
  for (unsigned i = 0; i < def.layer_count; ++i) {
    const auto &layer = def.layers[i];
    if (layer.is_agl == is_agl && layer.altitude_m == altitude_m)
      return int(i);
  }
  return -1;
}

[[gnu::pure]]
constexpr bool IsActiveLayer(const Layer &layer,
                             unsigned active_parameter,
                             unsigned active_wave_height,
                             unsigned active_vertical_agl) noexcept {
  if (layer.is_agl)
    return active_parameter == 1 &&
           layer.altitude_m == active_vertical_agl;
  return active_parameter == 0 && layer.altitude_m == active_wave_height;
}

} // namespace XCTherm
