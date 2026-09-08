// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>

class ProfileMap;
struct ComputerSettings;
class RoughTimeDelta;

namespace Profile {
  constexpr std::chrono::seconds MIN_UTC_OFFSET{-13 * 3600};
  constexpr std::chrono::seconds MAX_UTC_OFFSET{14 * 3600};

  bool LoadUTCOffset(const ProfileMap &map, RoughTimeDelta &value_r) noexcept;
  void Load(const ProfileMap &map, ComputerSettings &settings);
};
