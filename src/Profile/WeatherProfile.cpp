// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherProfile.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "Weather/Settings.hpp"
#include "util/StaticString.hxx"

#ifdef HAVE_PCMET

namespace Profile {
  static void Load(const ProfileMap &map, PCMetSettings &settings) {
    map.Get(ProfileKeys::PCMetUsername, settings.www_credentials.username);
    map.Get(ProfileKeys::PCMetPassword, settings.www_credentials.password);
    map.Get(ProfileKeys::PCMetFtpUsername, settings.ftp_credentials.username);
    map.Get(ProfileKeys::PCMetFtpPassword, settings.ftp_credentials.password);
  }
}

#endif

#ifdef HAVE_HTTP

namespace Profile {
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
#if !defined(HAVE_PCMET) && !defined(HAVE_HTTP)
  (void)map;
  (void)settings;
#endif
#ifdef HAVE_PCMET
  Load(map, settings.pcmet);
#endif

#ifdef HAVE_HTTP
  map.Get(ProfileKeys::EnableThermalInformationMap, settings.enable_tim);
  map.Get(ProfileKeys::XCThermAutoSwitch, settings.xctherm.auto_switch);
#endif

  {
    StaticString<64> xctherm_email;
    map.Get(ProfileKeys::XCThermEmail, xctherm_email);
    if (!xctherm_email.empty())
      settings.xctherm.credentials.email = xctherm_email;
  }

  {
    StaticString<64> xctherm_password;
    map.Get(ProfileKeys::XCThermPassword, xctherm_password);
    if (!xctherm_password.empty())
      settings.xctherm.credentials.password = xctherm_password;
  }

  map.Get(ProfileKeys::XCThermModel, settings.xctherm.model);
  map.Get(ProfileKeys::XCThermParameter, settings.xctherm.parameter);
  map.Get(ProfileKeys::XCThermWaveHeight, settings.xctherm.wave_height);
  map.Get(ProfileKeys::XCThermVerticalWindAGL,
          settings.xctherm.vertical_wind_agl);
  /* download_span_hours is intentionally NOT persisted - it resets
     to the default (1 h) at every startup. */

#ifdef HAVE_HTTP
  Load(map, settings.skysight);
#endif
}
