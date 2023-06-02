// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct DialogSettings {
  enum class TextInputStyle : uint8_t {
    /**
     * Use the platform default - i.e. keyboard if the device has a
     * pointing device.
     */
    Default,
    Keyboard,
    HighScore,
  };

  enum class TabStyle : uint8_t {
    Text,
    Icon,
  };

  TextInputStyle text_input_style;
  TabStyle tab_style;

  /**
   * Show the "expert" settings?
   */
  bool expert;

  void SetDefaults() noexcept;
};
