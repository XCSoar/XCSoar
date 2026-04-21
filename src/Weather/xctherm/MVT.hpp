// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace XCThermMVT {

struct IntPoint {
  int x = 0;
  int y = 0;
};

struct PointFeature {
  int x = 0;
  int y = 0;
  float direction_deg = 0;
  float vertical_speed = 0;
};

struct PolygonFeature {
  float min_value = 0;
  float max_value = 0;
  std::vector<std::vector<IntPoint>> rings;
};

struct Tile {
  uint32_t extent = 4096;
  std::vector<PointFeature> points;
  std::vector<PolygonFeature> polygons;
};

bool Parse(const std::vector<std::uint8_t> &buffer, Tile &out);

} // namespace XCThermMVT
