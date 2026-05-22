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
  map.GetEnum(ProfileKeys::XCThermOverlayLocation,
              settings.xctherm.overlay_location);
  map.Get(ProfileKeys::XCThermAutoSwitch, settings.xctherm.auto_switch);
#endif

  map.Get(ProfileKeys::XCThermEmail, settings.xctherm.credentials.email);
  map.Get(ProfileKeys::XCThermPassword, settings.xctherm.credentials.password);

#ifdef HAVE_HTTP
  map.GetEnum(ProfileKeys::XCThermModel, settings.xctherm.model);
#else
  map.Get(ProfileKeys::XCThermModel, settings.xctherm.model);
#endif
  map.Get(ProfileKeys::XCThermParameter, settings.xctherm.parameter);
  map.Get(ProfileKeys::XCThermWaveHeight, settings.xctherm.wave_height);
  map.Get(ProfileKeys::XCThermVerticalWindAGL,
          settings.xctherm.vertical_wind_agl);
  /* download_span_hours is intentionally NOT persisted — it resets
     to the default (1 h) at every startup. */
}
