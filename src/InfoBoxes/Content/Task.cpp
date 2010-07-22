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

#include "InfoBoxes/Content/Task.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "TaskClientUI.hpp"

#include <tchar.h>

void
InfoBoxContentBearing::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("Bearing"));

  if (XCSoarInterface::Calculated().task_stats.current_leg.
      solution_remaining.Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f")_T(DEG)_T("T"),
            (double)XCSoarInterface::Calculated().task_stats.current_leg.
            solution_remaining.Vector.Bearing.value_degrees());
  infobox.SetValue(tmp);
}

void
InfoBoxContentBearingDiff::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("Brng D"));

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.
      Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  double Value =
      (XCSoarInterface::Calculated().task_stats.current_leg.
       solution_remaining.Vector.Bearing - XCSoarInterface::Basic().
       TrackBearing).as_delta().value_degrees();

#ifndef __MINGW32__
  if (Value > 1)
    _stprintf(tmp, _T("%2.0f°»"), Value);
  else if (Value < -1)
    _stprintf(tmp, _T("«%2.0f°"), -Value);
  else
    _tcscpy(tmp, _T("«»"));
#else
  if (Value > 1)
    _stprintf(tmp, _T("%2.0fÂ°Â»"), Value);
  else if (Value < -1)
    _stprintf(tmp, _T("Â«%2.0fÂ°"), -Value);
  else
    _tcscpy(tmp, _T("Â«Â»"));
#endif

  infobox.SetValue(tmp);
}

void
InfoBoxContentNextWaypoint::Update(InfoBoxWindow &infobox)
{
  const Waypoint* way_point = task_ui.getActiveWaypoint();

  if (!way_point) {
    // Set Title
    infobox.SetTitle(_T("Next"));

    infobox.SetInvalid();
    return;
  }

  // Set Title
  TCHAR tmp[32];
  if (XCSoarInterface::SettingsMap().DisplayTextType == DISPLAYFIRSTTHREE) {
    _tcsncpy(tmp, way_point->Name.c_str(), 3);
    tmp[3] = '\0';
  } else if (XCSoarInterface::SettingsMap().DisplayTextType == DISPLAYNUMBER) {
    _stprintf(tmp, _T("%d"), way_point->id);
  } else {
    _tcsncpy(tmp, way_point->Name.c_str(), (sizeof(tmp) / sizeof(TCHAR)) - 1);
    tmp[(sizeof(tmp) / sizeof(TCHAR)) - 1] = '\0';
  }
  infobox.SetTitle(tmp);

  if (!XCSoarInterface::Calculated().task_stats.task_valid ||
      XCSoarInterface::Calculated().task_stats.current_leg.solution_remaining.
      Vector.Distance <= fixed(10)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  double Value =
      (XCSoarInterface::Calculated().task_stats.current_leg.
       solution_remaining.Vector.Bearing - XCSoarInterface::Basic().
       TrackBearing).as_delta().value_degrees();

#ifndef __MINGW32__
  if (Value > 1)
    _stprintf(tmp, _T("%2.0f°»"), Value);
  else if (Value < -1)
    _stprintf(tmp, _T("«%2.0f°"), -Value);
  else
    _tcscpy(tmp, _T("«»"));
#else
  if (Value > 1)
    _stprintf(tmp, _T("%2.0fÂ°Â»"), Value);
  else if (Value < -1)
    _stprintf(tmp, _T("Â«%2.0fÂ°"), -Value);
  else
    _tcscpy(tmp, _T("Â«Â»"));
#endif

  infobox.SetValue(tmp);

  // Set Comment
  infobox.SetComment(way_point->Comment.c_str());

  // Set Color
  if (XCSoarInterface::Calculated().task_stats.current_leg.
      solution_remaining.is_final_glide())
    // blue
    infobox.SetColor(2);
  else
    // black
    infobox.SetColor(0);
}
