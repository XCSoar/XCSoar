// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class FlarmColorDatabase;
class ProfileMap;

namespace Profile
{
  void Load(const ProfileMap &map, FlarmColorDatabase &db);
  void Save(ProfileMap &map, const FlarmColorDatabase &db);
};
