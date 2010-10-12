/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "InfoBoxes/Content/Other.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Hardware/Battery.hpp"
#include "Asset.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentGLoad::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Basic().acceleration.Available) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.2f"),
            (double)XCSoarInterface::Basic().acceleration.Gload);
  infobox.SetValue(tmp);
}

void
InfoBoxContentBattery::Update(InfoBoxWindow &infobox)
{
  TCHAR tmp[32];

#ifdef HAVE_BATTERY

  bool DisplaySupplyVoltageAsValue=false;
  switch (Power::External::Status) {
    case Power::External::OFF:
      infobox.SetComment(_("AC OFF"));
      break;
    case Power::External::ON:
      if (XCSoarInterface::Basic().SupplyBatteryVoltage <= (fixed)0)
        infobox.SetComment(_("AC ON"));
      else{
        DisplaySupplyVoltageAsValue = true;
        _stprintf(tmp, _T("%2.1fV"),
                  (double)XCSoarInterface::Basic().SupplyBatteryVoltage);
        infobox.SetValue(tmp);
      }
      break;
    case Power::External::UNKNOWN:
    default:
      infobox.SetCommentInvalid();
  }
  switch (Power::Battery::Status){
    case Power::Battery::HIGH:
    case Power::Battery::LOW:
    case Power::Battery::CRITICAL:
    case Power::Battery::CHARGING:
      if (Power::Battery::RemainingPercentValid){
        _stprintf(tmp, _T("%d%%"), Power::Battery::RemainingPercent);
        if (!DisplaySupplyVoltageAsValue)
          infobox.SetValue(tmp);
        else
          infobox.SetComment(tmp);
      }
      else
        if (!DisplaySupplyVoltageAsValue)
          infobox.SetValueInvalid();
        else
          infobox.SetCommentInvalid();
      break;
    case Power::Battery::NOBATTERY:
    case Power::Battery::UNKNOWN:
      if (!DisplaySupplyVoltageAsValue)
        infobox.SetValueInvalid();
      else
        infobox.SetCommentInvalid();
  }
  return;

#endif

  if (!negative(XCSoarInterface::Basic().SupplyBatteryVoltage)) {
    _stprintf(tmp, _T("%2.1fV"),
              (double)XCSoarInterface::Basic().SupplyBatteryVoltage);
    infobox.SetValue(tmp);
    return;
  }

  infobox.SetInvalid();
}

void
InfoBoxContentExperimental1::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%-2.1f"),
            (double)XCSoarInterface::Calculated().Experimental);
  infobox.SetValue(tmp);
}

void
InfoBoxContentExperimental2::Update(InfoBoxWindow &infobox)
{
  // Set Value
  infobox.SetInvalid();
}
