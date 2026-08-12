// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct DialogSettings {
  enum class TextInputStyle : uint8_t {
    /**
     * Use the platform default - i.e. the keyboard of the operating
     * system where there is one (#SystemKeyboard), else our own
     * keyboard if the device has a pointing device.
     */
    Default,

    /**
     * Always use XCSoar's own on-screen keyboard.
     */
    Keyboard,

    HighScore,

    /**
     * Use the on-screen keyboard provided by the operating system
     * (e.g. on iOS), which gives access to all special characters.
     * Falls back to #Keyboard if the platform has none.
     */
    SystemKeyboard,
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
