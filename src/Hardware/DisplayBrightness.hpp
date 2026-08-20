// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <memory>

class DisplayBrightness final {
  AllocatedPath brightness_path;
  unsigned max_brightness;
  bool writable;

  DisplayBrightness(AllocatedPath &&_brightness_path,
                    unsigned _max_brightness,
                    bool _writable) noexcept;

public:
  static constexpr unsigned MAX_BRIGHTNESS_PERCENT = 100;

  /**
   * Detect the brightness control on Kobo and non-Android Linux platforms.
   *
   * @return A brightness control, or `nullptr` when none is supported or
   * available.
   */
  [[nodiscard]]
  static std::unique_ptr<DisplayBrightness>
  Detect() noexcept;

  [[nodiscard]]
  bool IsWritable() const noexcept {
    return writable;
  }

  /**
   * Get the current brightness as a percentage on Kobo and non-Android Linux.
   *
   * @return A value from zero through MAX_BRIGHTNESS_PERCENT, or zero when
   * reading fails.
   */
  [[nodiscard]]
  unsigned GetBrightnessPercent() const noexcept;

  /**
   * Set the brightness on a writable Kobo or non-Android Linux platform.
   *
   * The percentage is clamped from zero through MAX_BRIGHTNESS_PERCENT.
   */
  void SetBrightnessPercent(unsigned percent) const noexcept;
};
