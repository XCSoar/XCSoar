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

#include "InfoBoxes/Content/Team.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Util/StringUtil.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentTeamCode::Update(InfoBoxWindow &infobox)
{
  const SETTINGS_TEAMCODE &settings = CommonInterface::SettingsComputer();
  const FLARM_STATE &flarm = XCSoarInterface::Basic().flarm;

  if (!settings.TeamCodeRefWaypoint) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  infobox.SetValue(XCSoarInterface::Calculated().OwnTeamCode.GetCode());

  // Set Comment
  if (settings.TeammateCodeValid) {
    infobox.SetComment(settings.TeammateCode.GetCode());
    if (!settings.TeamFlarmTracking)
      infobox.SetColorBottom(0);
    else if (flarm.FindTraffic(settings.TeamFlarmIdTarget) != NULL)
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
  SETTINGS_TEAMCODE &settings = CommonInterface::SetSettingsComputer();
  const FLARM_STATE &flarm = XCSoarInterface::Basic().flarm;
  const FLARM_TRAFFIC *traffic =
    settings.TeamFlarmIdTarget.defined()
    ? flarm.FindTraffic(settings.TeamFlarmIdTarget)
    : NULL;

  if (keycode == ibkUp)
    traffic = (traffic == NULL ?
               flarm.FirstTraffic() : flarm.NextTraffic(traffic));
  else if (keycode == ibkDown)
    traffic = (traffic == NULL ?
               flarm.LastTraffic() : flarm.PreviousTraffic(traffic));
  else
    return false;

  if (traffic != NULL) {
    settings.TeamFlarmIdTarget = traffic->id;

    if (traffic->HasName()) {
      // copy the 3 first chars from the name to TeamFlarmCNTarget
      settings.TeamFlarmCNTarget = traffic->name;
    } else {
      settings.TeamFlarmCNTarget.clear();
    }
  } else {
    // no flarm traffic to select!
    settings.TeamFlarmIdTarget.clear();
    settings.TeamFlarmCNTarget.clear();
  }
  return true;
}

void
InfoBoxContentTeamBearing::Update(InfoBoxWindow &infobox)
{
  const SETTINGS_TEAMCODE &settings = CommonInterface::SettingsComputer();
  const FLARM_STATE &flarm = XCSoarInterface::Basic().flarm;

  if (settings.TeamFlarmIdTarget.defined() || settings.TeammateCodeValid) {
    // Set Value
    infobox.SetValue(XCSoarInterface::Calculated().TeammateBearing,
                     _T("T"));
  }
  else
    infobox.SetValueInvalid();

  // Set Comment
  if (!settings.TeamFlarmIdTarget.defined())
    infobox.SetCommentInvalid();
  else if (!settings.TeamFlarmCNTarget.empty())
    infobox.SetComment(settings.TeamFlarmCNTarget.c_str());
  else
    infobox.SetComment(_T("???"));

  if (flarm.FindTraffic(settings.TeamFlarmIdTarget) != NULL)
    infobox.SetColorBottom(2);
  else
    infobox.SetColorBottom(1);
}

void
InfoBoxContentTeamBearingDiff::Update(InfoBoxWindow &infobox)
{
  const SETTINGS_TEAMCODE &settings = CommonInterface::SettingsComputer();
  const FLARM_STATE &flarm = XCSoarInterface::Basic().flarm;

  if (settings.TeamFlarmIdTarget.defined() || settings.TeammateCodeValid) {
    // Set Value
    Angle Value = XCSoarInterface::Calculated().TeammateBearing -
                  XCSoarInterface::Basic().track;

    SetValueBearingDifference(infobox, Value);
  } else
    infobox.SetValueInvalid();

  // Set Comment
  if (!settings.TeamFlarmIdTarget.defined())
    infobox.SetCommentInvalid();
  else if (!string_is_empty(settings.TeamFlarmCNTarget))
    infobox.SetComment(settings.TeamFlarmCNTarget);
  else
    infobox.SetComment(_T("???"));

  if (flarm.FindTraffic(settings.TeamFlarmIdTarget) != NULL)
    infobox.SetColorBottom(2);
  else
    infobox.SetColorBottom(1);
}

void
InfoBoxContentTeamDistance::Update(InfoBoxWindow &infobox)
{
  const SETTINGS_TEAMCODE &settings = CommonInterface::SettingsComputer();
  const FLARM_STATE &flarm = XCSoarInterface::Basic().flarm;

  if (!settings.TeammateCodeValid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserDistance(XCSoarInterface::Calculated().TeammateRange,
                            tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.DistanceUnit);

  // Set Comment
  if (!settings.TeamFlarmIdTarget.defined())
    infobox.SetCommentInvalid();
  else if (!string_is_empty(settings.TeamFlarmCNTarget))
    infobox.SetComment(settings.TeamFlarmCNTarget);
  else
    infobox.SetComment(_T("???"));

  if (flarm.FindTraffic(settings.TeamFlarmIdTarget) != NULL)
    infobox.SetColorBottom(2);
  else
    infobox.SetColorBottom(1);
}
