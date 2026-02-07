// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimesStatusPanel.hpp"
#include "Interface.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Math/SunEphemeris.hpp"
#include "Language/Language.hpp"

enum Controls {
  LocalTime,
  UTCTime,
  UTCDate,
  FlightTime,
  TakeoffTime,
  LandingTime,
  Daylight,
};

void
TimesStatusPanel::Refresh() noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const FlyingState &flight = CommonInterface::Calculated().flight;
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  StaticString<64> temp;

  if (basic.location_available && basic.date_time_utc.IsDatePlausible()) {
    SunEphemeris::Result sun =
      SunEphemeris::CalcSunTimes(basic.location, basic.date_time_utc,
                                 settings.utc_offset);

    const unsigned sunrisehours = (int)sun.time_of_sunrise;
    const unsigned sunrisemins = (int)((sun.time_of_sunrise - double(sunrisehours)) * 60);
    const unsigned sunsethours = (int)sun.time_of_sunset;
    const unsigned sunsetmins = (int)((sun.time_of_sunset - double(sunsethours)) * 60);

    temp.Format("%02u:%02u - %02u:%02u", sunrisehours, sunrisemins, sunsethours, sunsetmins);
    SetText(Daylight, temp);
  } else {
    ClearText(Daylight);
  }

  if (basic.time_available) {
    SetText(LocalTime,
            FormatLocalTimeHHMM(basic.time, settings.utc_offset));
    SetText(UTCTime, FormatTimeHHMM(basic.time));
  } else {
    ClearText(LocalTime);
    ClearText(UTCTime);
  }

  if (basic.date_time_utc.IsDatePlausible()) {
    temp.Format("%04d-%02d-%02d", basic.date_time_utc.year,
                basic.date_time_utc.month, basic.date_time_utc.day);
    SetText(UTCDate, temp);
  } else {
    ClearText(UTCDate);
  }

  if (flight.takeoff_time.IsDefined()) {
    SetText(TakeoffTime,
            FormatLocalTimeHHMM(flight.takeoff_time,
                                settings.utc_offset));
  } else {
    ClearText(TakeoffTime);
  }

  if (flight.landing_time.IsDefined()) {
    SetText(LandingTime,
            FormatLocalTimeHHMM(flight.landing_time,
                                settings.utc_offset));
  } else {
    ClearText(LandingTime);
  }

  if (flight.flight_time.count() > 0) {
    SetText(FlightTime, FormatSignedTimeHHMM(flight.flight_time));
  } else {
    ClearText(FlightTime);
  }
}

void
TimesStatusPanel::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddReadOnly(_("Local time"));
  AddReadOnly(_("UTC time"));
  AddReadOnly(_("UTC date"));
  AddReadOnly(_("Flight time"));
  AddReadOnly(_("Takeoff time"));
  AddReadOnly(_("Landing time"));
  AddReadOnly(_("Daylight time"));
}
