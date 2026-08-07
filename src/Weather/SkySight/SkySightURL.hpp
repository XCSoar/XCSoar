// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <fmt/format.h>

#include <string>
#include <string_view>

namespace SkySightUrl {

static constexpr std::string_view API_BASE = "https://skysight.io/api";

[[nodiscard]] inline std::string
Api(std::string_view endpoint)
{
  std::string url{API_BASE};
  if (!endpoint.empty()) {
    url.push_back('/');
    url.append(endpoint);
  }

  return url;
}

[[nodiscard]] inline std::string
Tile(std::string_view layer_id,
     unsigned zoom, unsigned x, unsigned y,
     std::string_view timestamp)
{
  return fmt::format("{}/{}/{}/{}/{}/{}", API_BASE, layer_id,
                     zoom, x, y, timestamp);
}

} // namespace SkySightUrl
