// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

class ProfileMap;
class RoughTime;

namespace Profile {
  /**
   * Load a RoughTime from the profile.
   */
  bool Get(const ProfileMap &map, std::string_view key, RoughTime &value) noexcept;

  /**
   * Save a RoughTime to the profile.
   */
  void Set(std::string_view key, const RoughTime &value) noexcept;
}