// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <chrono>

namespace Co {
template<typename T> class Task;
}

class CurlGlobal;

namespace PureTrack {

struct Settings;

struct Sample {
  std::chrono::system_clock::time_point timestamp;
  GeoPoint location;
  double altitude = 0;
  double speed = 0;
  double course = 0;
  double vertical_speed = 0;
};

class Client final {
  CurlGlobal &curl;

public:
  explicit Client(CurlGlobal &_curl) noexcept
    :curl(_curl) {}

  Co::Task<void> Insert(const Settings &settings, const Sample &sample);
};

} // namespace PureTrack
