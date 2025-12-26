// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NotamConfig.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "NOTAM/Settings.hpp"
#include "NOTAM/Config.hpp"

#include <algorithm>

void
Profile::LoadNOTAMSettings(const ProfileMap &map, NOTAMSettings &settings)
{
  // Basic NOTAM settings
  map.Get(ProfileKeys::NOTAMEnabled, settings.enabled);
  map.Get(ProfileKeys::NOTAMRadius, settings.radius_km);
  settings.radius_km = std::clamp(settings.radius_km, 1u,
                                  MAX_NOTAM_REQUEST_RADIUS_KM);
  map.Get(ProfileKeys::NOTAMRefreshInterval, settings.refresh_interval_min);
  settings.refresh_interval_min =
    std::clamp(settings.refresh_interval_min, 0u,
               MAX_NOTAM_REFRESH_INTERVAL_MIN);

  if (map.Exists(ProfileKeys::NOTAMApiUrl))
    map.Get(ProfileKeys::NOTAMApiUrl, settings.api_base_url);

  // Filter settings
  map.Get(ProfileKeys::NOTAMShowIFR, settings.show_ifr);
  map.Get(ProfileKeys::NOTAMShowOnlyEffective, settings.show_only_effective);
  map.Get(ProfileKeys::NOTAMMaxRadius, settings.max_radius_m);
  map.Get(ProfileKeys::NOTAMHiddenQCodes, settings.hidden_qcodes);
}
