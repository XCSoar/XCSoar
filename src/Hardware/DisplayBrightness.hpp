// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <memory>

class DisplayBrightness final {
#if defined(KOBO) || (defined(__linux__) && !defined(ANDROID))
  enum class Type {
    Kobo,
    Linux,
  };

  Type type;
  AllocatedPath brightness_path;
  unsigned max_brightness;
  bool writable;

  DisplayBrightness(Type _type,
                    AllocatedPath &&_brightness_path,
                    unsigned _max_brightness,
                    bool _writable) noexcept;
#endif

public:
  [[nodiscard]] static std::unique_ptr<DisplayBrightness>
  Detect() noexcept;

  [[nodiscard]] bool IsWritable() const noexcept {
#if defined(KOBO) || (defined(__linux__) && !defined(ANDROID))
    return writable;
#else
    return false;
#endif
  }

  [[nodiscard]] unsigned GetBrightnessPercent() const noexcept;

  void SetBrightnessPercent(unsigned percent) const noexcept;
};
