// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProfileMap;
struct PageLayout;
struct PageSettings;

namespace Profile {
  void Load(const ProfileMap &map, PageSettings &settings);

  void Save(ProfileMap &map, const PageLayout &page, unsigned i);
  void Save(ProfileMap &map, const PageSettings &settings);
};
