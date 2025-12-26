// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NotamConfig.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "NOTAM/Settings.hpp"

void
Profile::LoadNotamSettings(const ProfileMap &map, NOTAMSettings &settings)
{
  // Basic NOTAM settings
  map.Get(ProfileKeys::NOTAMEnabled, settings.enabled);
  map.Get(ProfileKeys::NOTAMRadius, settings.radius_km);
  map.Get(ProfileKeys::NOTAMRefreshInterval, settings.refresh_interval_min);

  // Filter settings
  map.Get(ProfileKeys::NOTAMShowIFR, settings.show_ifr);
  map.Get(ProfileKeys::NOTAMShowOnlyEffective, settings.show_only_effective);
  map.Get(ProfileKeys::NOTAMMaxRadius, settings.max_radius_m);
  map.Get(ProfileKeys::NOTAMHiddenQCodes, settings.hidden_qcodes);
}
