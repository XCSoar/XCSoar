// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/Features.hpp"

struct TrackingSettings;
class ProfileMap;

namespace Profile {
  void Load(const ProfileMap &map, TrackingSettings &settings);
};
