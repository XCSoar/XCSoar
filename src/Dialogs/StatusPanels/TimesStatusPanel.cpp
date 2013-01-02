/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "LocalTime.hpp"
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

  StaticString<64> temp;

  if (basic.location_available && basic.date_available) {
    SunEphemeris::Result sun = SunEphemeris::CalcSunTimes(
        basic.location, basic.date_time_utc, fixed(GetUTCOffset()) / 3600);

    const unsigned sunrisehours = (int)sun.time_of_sunrise;
    const unsigned sunrisemins = (int)((sun.time_of_sunrise - fixed(sunrisehours)) * 60);
    const unsigned sunsethours = (int)sun.time_of_sunset;
    const unsigned sunsetmins = (int)((sun.time_of_sunset - fixed(sunsethours)) * 60);

    temp.Format(_T("%02u:%02u - %02u:%02u"), sunrisehours, sunrisemins, sunsethours, sunsetmins);
    SetText(Daylight, temp);
  } else {
    SetText(Daylight, _T(""));
  }

  if (basic.time_available) {
    FormatSignedTimeHHMM(temp.buffer(), DetectCurrentTime(basic));
    SetText(LocalTime, temp);
    FormatSignedTimeHHMM(temp.buffer(), (int) basic.time);
    SetText(UTCTime, temp);
  } else {
    SetText(LocalTime, _T(""));
    SetText(UTCTime, _T(""));
  }

  if (basic.date_available) {
    temp.Format(_T("%04d-%02d-%02d"), basic.date_time_utc.year,
                basic.date_time_utc.month, basic.date_time_utc.day);
    SetText(UTCDate, temp);
  } else {
    SetText(UTCDate, _T(""));
  }

  if (positive(flight.flight_time)) {
    FormatSignedTimeHHMM(temp.buffer(), TimeLocal((int)flight.takeoff_time));
    SetText(TakeoffTime, temp);
  } else {
    SetText(TakeoffTime, _T(""));
  }

  if (!flight.flying && positive(flight.flight_time)) {
    FormatSignedTimeHHMM(temp.buffer(),
                      TimeLocal((long)(flight.takeoff_time
                                       + flight.flight_time)));
    SetText(LandingTime, temp);
  } else {
    SetText(LandingTime, _T(""));
  }

  if (positive(flight.flight_time)) {
    FormatSignedTimeHHMM(temp.buffer(), (int)flight.flight_time);
    SetText(FlightTime, temp);
  } else {
    SetText(FlightTime, _T(""));
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
