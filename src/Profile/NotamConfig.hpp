// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProfileMap;
struct NOTAMSettings;

namespace Profile
{
/**
 * Load NOTAM-related settings from the profile.
 */
void LoadNotamSettings(const ProfileMap &map, NOTAMSettings &settings);
}
