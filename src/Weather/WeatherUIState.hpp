// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenTime.hpp"

/**
 * The state of weather display on the user interface.
 */
struct WeatherUIState {
  /**
   * The map index being displayed.  -1 means no weather map (normal
   * terrain display).
   */
  int map;

  /**
   * The weather forecast time that shall be displayed.
   */
  BrokenTime time;

  void Clear() {
    map = -1;
    time = BrokenTime::Invalid();
  }
};
