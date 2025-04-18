// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/InfoBoxSettings.hpp"

class ProfileMap;

namespace Profile
{
  void Load(const ProfileMap &map, InfoBoxSettings &settings);
  void Save(ProfileMap &map,
            const InfoBoxSettings::Panel &panel, unsigned index);
};
