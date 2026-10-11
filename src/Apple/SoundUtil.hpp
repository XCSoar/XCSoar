// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

class SoundUtil {
public:
  static bool Play(const char *resource_name);

  /**
   * Play the sounds one after another, replacing a sequence which is
   * still playing.
   */
  static bool PlaySequence(std::span<const char *const> resource_names);
};
