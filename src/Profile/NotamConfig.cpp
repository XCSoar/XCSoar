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
  map.Get(ProfileKeys::NOTAMMaxCount, settings.max_notams);
  map.Get(ProfileKeys::NOTAMRefreshInterval, settings.refresh_interval_min);

  // Time-based filtering
  map.Get(ProfileKeys::NOTAMFilterDaylightOnly, settings.filter_daylight_only);
  map.Get(ProfileKeys::NOTAMFilterNightOnly, settings.filter_night_only);
  map.Get(ProfileKeys::NOTAMHoursBeforeSunrise, settings.hours_before_sunrise);
  map.Get(ProfileKeys::NOTAMHoursAfterSunset, settings.hours_after_sunset);

  // Feature type filters (simplified)
  map.Get(ProfileKeys::NOTAMShowAirspace, settings.show_airspace);
  map.Get(ProfileKeys::NOTAMShowObst, settings.show_obst);
  map.Get(ProfileKeys::NOTAMShowMilitary, settings.show_military);
  map.Get(ProfileKeys::NOTAMShowOther, settings.show_other);
}
