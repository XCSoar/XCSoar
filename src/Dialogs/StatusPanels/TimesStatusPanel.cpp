/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
TimesStatusPanel::Refresh()
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

    temp.Format(_T("%02u:%02u - %02u:%02u"), sunrisehours, sunrisemins, sunsethours, sunsetmins);
    SetText(Daylight, temp);
  } else {
    ClearText(Daylight);
  }

  if (basic.time_available) {
    SetText(LocalTime,
            FormatLocalTimeHHMM((int)basic.time, settings.utc_offset));
    SetText(UTCTime, FormatSignedTimeHHMM((int)basic.time));
  } else {
    ClearText(LocalTime);
    ClearText(UTCTime);
  }

  if (basic.date_time_utc.IsDatePlausible()) {
    temp.Format(_T("%04d-%02d-%02d"), basic.date_time_utc.year,
                basic.date_time_utc.month, basic.date_time_utc.day);
    SetText(UTCDate, temp);
  } else {
    ClearText(UTCDate);
  }

  if (flight.takeoff_time >= 0) {
    SetText(TakeoffTime,
            FormatLocalTimeHHMM((int)flight.takeoff_time,
                                settings.utc_offset));
  } else {
    ClearText(TakeoffTime);
  }

  if (flight.landing_time >= 0) {
    SetText(LandingTime,
            FormatLocalTimeHHMM(int(flight.landing_time),
                                settings.utc_offset));
  } else {
    ClearText(LandingTime);
  }

  if (flight.flight_time > 0) {
    SetText(FlightTime, FormatSignedTimeHHMM((int)flight.flight_time));
  } else {
    ClearText(FlightTime);
  }
}

void
TimesStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddReadOnly(_("Local time"));
  AddReadOnly(_("UTC time"));
  AddReadOnly(_("UTC date"));
  AddReadOnly(_("Flight time"));
  AddReadOnly(_("Takeoff time"));
  AddReadOnly(_("Landing time"));
  AddReadOnly(_("Daylight time"));
}
