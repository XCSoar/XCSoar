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

#include "InfoBoxes/Content/Team.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Units.hpp"
#include "Util/StringUtil.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentTeamCode::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::SettingsComputer().TeamCodeRefWaypoint) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  infobox.SetValue(XCSoarInterface::Calculated().OwnTeamCode);

  // Set Comment
  if (XCSoarInterface::SettingsComputer().TeammateCodeValid == true){
    infobox.SetComment(XCSoarInterface::SettingsComputer().TeammateCode);
    if (!XCSoarInterface::SettingsComputer().TeamFlarmTracking)
      infobox.SetColorBottom(0);
    else if (XCSoarInterface::Basic().flarm.FindTraffic(
        XCSoarInterface::SettingsComputer().TeamFlarmIdTarget) != NULL)
      infobox.SetColorBottom(2);
    else
      infobox.SetColorBottom(1);
  }
  else
    infobox.SetCommentInvalid();
}

bool
InfoBoxContentTeamCode::HandleKey(const InfoBoxKeyCodes keycode)
{
  const FLARM_STATE &flarm = XCSoarInterface::Basic().flarm;
  const FLARM_TRAFFIC *traffic =
      XCSoarInterface::SettingsComputer().TeamFlarmIdTarget.defined() ?
      flarm.FindTraffic(XCSoarInterface::SettingsComputer().TeamFlarmIdTarget) :
      NULL;

  if (keycode == ibkUp)
    traffic = (traffic == NULL ?
               flarm.FirstTraffic() : flarm.NextTraffic(traffic));
  else if (keycode == ibkDown)
    traffic = (traffic == NULL ?
               flarm.LastTraffic() : flarm.PreviousTraffic(traffic));
  else
    return false;

  if (traffic != NULL) {
    XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget = traffic->ID;

    if (traffic->HasName()) {
      // copy the 3 first chars from the name to TeamFlarmCNTarget
      for (int z = 0; z < 3; z++)
        XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[z] =
            (traffic->Name[z] != 0 ? traffic->Name[z] : 32);

      XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[3] = 0;
    } else {
      XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
    }
  } else {
    // no flarm traffic to select!
    XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget.clear();
    XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
  }
  return true;
}

void
InfoBoxContentTeamBearing::Update(InfoBoxWindow &infobox)
{
  if (XCSoarInterface::SettingsComputer().TeamFlarmIdTarget.defined() ||
      XCSoarInterface::SettingsComputer().TeammateCodeValid == true){
    // Set Value
    TCHAR tmp[32];
    _stprintf(tmp, _T("%2.0f")_T(DEG)_T("T"), (double)
              XCSoarInterface::Calculated().TeammateBearing.value_degrees());
    infobox.SetValue(tmp);
  }
  else
    infobox.SetValueInvalid();

  // Set Comment
  if (!XCSoarInterface::SettingsComputer().TeamFlarmIdTarget.defined())
    infobox.SetCommentInvalid();
  else if (!string_is_empty(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget))
    infobox.SetComment(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget);
  else
    infobox.SetComment(_T("???"));

  if (XCSoarInterface::Basic().flarm.FindTraffic(
      XCSoarInterface::SettingsComputer().TeamFlarmIdTarget) != NULL)
    infobox.SetColorBottom(2);
  else
    infobox.SetColorBottom(1);
}

void
InfoBoxContentTeamBearingDiff::Update(InfoBoxWindow &infobox)
{
#ifndef OLD_TASK
  infobox.SetInvalid();
  return;
#else
  if (!way_points.verify_index(XCSoarInterface::SettingsComputer().
      TeamCodeRefWaypoint)
      || !XCSoarInterface::SettingsComputer().TeammateCodeValid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  double Value = XCSoarInterface::Calculated().TeammateBearing -
                 XCSoarInterface::Basic().TrackBearing;

  if (Value < -180.0)
    Value += 360.0;
  else if (Value > 180.0)
    Value -= 360.0;

#ifndef __MINGW32__
  if (Value > 1)
    _stprintf(tmp, TEXT("%2.0f°»"), Value);
  else if (Value < -1)
    _stprintf(tmp, TEXT("«%2.0f°"), -Value);
  else
    _tcscpy(tmp, TEXT("«»"));
#else
  if (Value > 1)
    _stprintf(tmp, TEXT("%2.0fÂ°Â»"), Value);
  else if (Value < -1)
    _stprintf(tmp, TEXT("Â«%2.0fÂ°"), -Value);
  else
    _tcscpy(tmp, TEXT("Â«Â»"));
#endif
  infobox.SetValue(tmp);
#endif

  // Set Comment
  if (!XCSoarInterface::SettingsComputer().TeamFlarmIdTarget.defined())
    infobox.SetCommentInvalid();
  else if (!string_is_empty(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget))
    infobox.SetComment(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget);
  else
    infobox.SetComment(_T("???"));

  if (XCSoarInterface::Basic().flarm.FindTraffic(
      XCSoarInterface::SettingsComputer().TeamFlarmIdTarget) != NULL)
    infobox.SetColorBottom(2);
  else
    infobox.SetColorBottom(1);
}

void
InfoBoxContentTeamDistance::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::SettingsComputer().TeammateCodeValid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserDistance(XCSoarInterface::Calculated().TeammateRange,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::DistanceUnit);

  // Set Comment
  if (!XCSoarInterface::SettingsComputer().TeamFlarmIdTarget.defined())
    infobox.SetCommentInvalid();
  else if (!string_is_empty(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget))
    infobox.SetComment(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget);
  else
    infobox.SetComment(_T("???"));

  if (XCSoarInterface::Basic().flarm.FindTraffic(
      XCSoarInterface::SettingsComputer().TeamFlarmIdTarget) != NULL)
    infobox.SetColorBottom(2);
  else
    infobox.SetColorBottom(1);
}
