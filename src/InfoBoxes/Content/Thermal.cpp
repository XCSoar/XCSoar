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

#include "InfoBoxes/Content/Thermal.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Units.hpp"
#include "Interface.hpp"

#include <tchar.h>

void
InfoBoxContentThermal30s::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  // Set Title
  infobox.SetTitle(_T("TC 30s"));

  // Set Value
  _stprintf(sTmp, _T("%-2.1f"),
            (double)XCSoarInterface::Calculated().Average30s);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);

  if (XCSoarInterface::Calculated().Average30s <
      fixed_half * XCSoarInterface::Calculated().common_stats.current_risk_mc)
    // red
    infobox.SetColor(1);
  else
    infobox.SetColor(0);
}

void
InfoBoxContentThermalLastAvg::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("TL Avg"));

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%-2.1f"),
            (double)XCSoarInterface::Calculated().LastThermalAverage);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);
}

void
InfoBoxContentThermalLastGain::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("TL Gain"));

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%2.0f"),
            (double)XCSoarInterface::Calculated().LastThermalGain);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentThermalLastTime::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("TL Time"));

  // Set Value
  TCHAR sTmp[32];
  int dd = (int)abs(XCSoarInterface::Calculated().LastThermalTime) % (3600 * 24);
  int hours = dd / 3600;
  int mins = dd / 60 - hours * 60;
  int seconds = dd - mins * 60 - hours * 3600;

  if (hours > 0) { // hh:mm, ss
    _stprintf(sTmp, _T("%02d:%02d"), hours, mins);
    infobox.SetValue(sTmp);
    _stprintf(sTmp, _T("%02d"), seconds);
    infobox.SetComment(sTmp);
  } else { // mm:ss
    _stprintf(sTmp, _T("%02d:%02d"), mins, seconds);
    infobox.SetValue(sTmp);
    infobox.SetComment(_T(""));
  }
}
