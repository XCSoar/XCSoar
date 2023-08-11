// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Point2D.hpp"

class WindEKF {
  float X[3];
  float k;

public:
  void Init() noexcept;
  void Update(float airspeed, FloatPoint2D gps_vel) noexcept;

  constexpr FloatPoint2D GetResult() const noexcept {
    return {X[0], X[1]};
  }
};
