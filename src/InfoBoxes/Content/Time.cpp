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

#include "InfoBoxes/Content/Time.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "LocalTime.hpp"
#include "Units/UnitsFormatter.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentTimeLocal::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  // Set Value
  int dd = DetectCurrentTime(XCSoarInterface::Basic());
  const BrokenTime t = BrokenTime::FromSecondOfDayChecked(abs(dd));

  // Set Value
  _stprintf(sTmp, _T("%02u:%02u"), t.hour, t.minute);
  infobox.SetValue(sTmp);

  // Set Comment
  _stprintf(sTmp, _T("%02u"), t.second);
  infobox.SetComment(sTmp);
}

void
InfoBoxContentTimeUTC::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  // Set Value
  const BrokenDateTime t = XCSoarInterface::Basic().DateTime;
  _stprintf(sTmp, _T("%02d:%02d"), t.hour, t.minute);
  infobox.SetValue(sTmp);

  // Set Comment
  _stprintf(sTmp, _T("%02d"), t.second);
  infobox.SetComment(sTmp);
}

void
InfoBoxContentTimeFlight::Update(InfoBoxWindow &infobox)
{
  if (!positive(CommonInterface::Calculated().flight.FlightTime)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR value[32], comment[32];
  Units::TimeToTextSmart(value, comment,
                         (int)XCSoarInterface::Calculated().flight.FlightTime);

  infobox.SetValue(value);
  infobox.SetComment(comment);
}
