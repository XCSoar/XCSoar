// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class UnitGroup: uint8_t
{
  NONE,
  DISTANCE,
  ALTITUDE,
  TEMPERATURE,
  HORIZONTAL_SPEED,
  VERTICAL_SPEED,
  WIND_SPEED,
  TASK_SPEED,
  PRESSURE,
  WING_LOADING,
  MASS,
  ROTATION,
};
