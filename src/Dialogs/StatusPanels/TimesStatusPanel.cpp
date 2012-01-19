/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Form/Util.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "LocalTime.hpp"
#include "Math/SunEphemeris.hpp"

void
TimesStatusPanel::Refresh()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const FlyingState &flight = CommonInterface::Calculated().flight;

  TCHAR Temp[64];

  if (basic.location_available && basic.date_available) {
    SunEphemeris::Result sun = SunEphemeris::CalcSunTimes(
        basic.location, basic.date_time_utc, fixed(GetUTCOffset()) / 3600);

    int sunsethours = (int)sun.time_of_sunset;
    int sunsetmins = (int)((sun.time_of_sunset - fixed(sunsethours)) * 60);

    _stprintf(Temp, _T("%02d:%02d"), sunsethours, sunsetmins);
    SetFormValue(form, _T("prpSunset"), Temp);
  } else {
    SetFormValue(form, _T("prpSunset"), _T(""));
  }

  TimeToTextHHMMSigned(Temp, DetectCurrentTime(basic));
  SetFormValue(form, _T("prpLocalTime"), Temp);

  if (positive(flight.flight_time)) {
    TimeToTextHHMMSigned(Temp, TimeLocal((long)flight.takeoff_time));
    SetFormValue(form, _T("prpTakeoffTime"), Temp);
  } else {
    SetFormValue(form, _T("prpTakeoffTime"), _T(""));
  }

  if (!flight.flying && positive(flight.flight_time)) {
    TimeToTextHHMMSigned(Temp,
                      TimeLocal((long)(flight.takeoff_time
                                       + flight.flight_time)));
    SetFormValue(form, _T("prpLandingTime"), Temp);
  } else {
    SetFormValue(form, _T("prpLandingTime"), _T(""));
  }

  if (positive(flight.flight_time)) {
    TimeToTextHHMMSigned(Temp, (int)flight.flight_time);
    SetFormValue(form, _T("prpFlightTime"), Temp);
  } else {
    SetFormValue(form, _T("prpFlightTime"), _T(""));
  }
}

void
TimesStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_TIMES"));
}
