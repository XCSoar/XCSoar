// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherProfile.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "Weather/Settings.hpp"

#ifdef HAVE_PCMET

namespace Profile {
  static void Load(const ProfileMap &map, PCMetSettings &settings) {
    map.Get(ProfileKeys::PCMetUsername, settings.www_credentials.username);
    map.Get(ProfileKeys::PCMetPassword, settings.www_credentials.password);
    map.Get(ProfileKeys::PCMetFtpUsername, settings.ftp_credentials.username);
    map.Get(ProfileKeys::PCMetFtpPassword, settings.ftp_credentials.password);
  }

  static void Load(const ProfileMap &map, SkysightSettings &settings) {
    map.Get(ProfileKeys::SkysightEmail, settings.email);
    map.Get(ProfileKeys::SkysightPassword, settings.password);
    map.Get(ProfileKeys::SkysightRegion, settings.region);
  }  
}

#endif

void
Profile::Load(const ProfileMap &map, WeatherSettings &settings)
{
#ifdef HAVE_PCMET
  Load(map, settings.pcmet);
#endif

#ifdef HAVE_HTTP
  map.Get(ProfileKeys::EnableThermalInformationMap, settings.enable_tim);
#endif

#ifdef HAVE_SKYSIGHT
  Load(map, settings.skysight);
#endif
}
