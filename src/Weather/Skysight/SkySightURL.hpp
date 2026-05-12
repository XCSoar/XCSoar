// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  auto url = Api(layer_id);
  url.push_back('/');
  url += std::to_string(zoom);
  url.push_back('/');
  url += std::to_string(x);
  url.push_back('/');
  url += std::to_string(y);
  url.push_back('/');
  url.append(timestamp);
  return url;
}

} // namespace SkySightUrl