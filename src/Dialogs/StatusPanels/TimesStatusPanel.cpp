/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Form/Edit.hpp"
#include "Units/UnitsFormatter.hpp"
#include "LocalTime.hpp"

void
TimesStatusPanel::Refresh()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const FlyingState &flight = calculated.flight;

  WndProperty *wp;
  TCHAR Temp[1000];
  fixed sunsettime;
  int sunsethours;
  int sunsetmins;

  sunsettime = XCSoarInterface::Calculated().sunset_time;
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime - fixed(sunsethours)) * 60);

  wp = (WndProperty*)form.FindByName(_T("prpSunset"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%02d:%02d"), sunsethours, sunsetmins);
  wp->SetText(Temp);

  wp = (WndProperty*)form.FindByName(_T("prpLocalTime"));
  assert(wp != NULL);
  Units::TimeToTextHHMMSigned(Temp, DetectCurrentTime(basic));
  wp->SetText(Temp);

  wp = (WndProperty*)form.FindByName(_T("prpTakeoffTime"));
  assert(wp != NULL);
  if (positive(flight.flight_time)) {
    Units::TimeToTextHHMMSigned(Temp, TimeLocal((long)flight.takeoff_time));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)form.FindByName(_T("prpLandingTime"));
  assert(wp != NULL);
  if (!flight.flying && positive(flight.flight_time)) {
    Units::TimeToTextHHMMSigned(Temp,
                      TimeLocal((long)(flight.takeoff_time
                                       + flight.flight_time)));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)form.FindByName(_T("prpFlightTime"));
  assert(wp != NULL);
  if (positive(flight.flight_time)) {
    Units::TimeToTextHHMMSigned(Temp, (int)flight.flight_time);
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }
}

void
TimesStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_TIMES"));
}
