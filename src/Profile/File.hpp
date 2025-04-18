// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProfileMap;
class Path;

namespace Profile {
  /**
   * Throws std::runtime_errror on error.
   */
  void LoadFile(ProfileMap &map, Path path);

/**
 * Throws std::runtime_errror on error.
 */
void SaveFile(const ProfileMap &map, Path path);
}
