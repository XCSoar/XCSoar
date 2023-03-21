// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class WindEKF {
  float X[3];
  float k;

public:
  void Init();
  void Update(double airspeed, const float gps_vel[2]);
  const float* get_state() const { return X; };
};
