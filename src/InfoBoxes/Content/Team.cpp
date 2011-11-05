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
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Units/UnitsFormatter.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentTeamCode::Update(InfoBoxData &data)
{
  const TeamCodeSettings &settings = CommonInterface::SettingsComputer();
  const TeamInfo &teamcode_info = XCSoarInterface::Calculated();

  if (!settings.team_code_reference_waypoint) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(XCSoarInterface::Calculated().own_teammate_code.GetCode());

  // Set Comment
  if (teamcode_info.flarm_teammate_code_available) {
    data.SetComment(teamcode_info.flarm_teammate_code.GetCode());
    data.SetCommentColor(teamcode_info.flarm_teammate_code_current ? 2 : 1);
  } else if (settings.team_code_valid) {
    data.SetComment(settings.team_code.GetCode());
    data.SetCommentColor(0);
  }
  else
    data.SetCommentInvalid();
}

bool
InfoBoxContentTeamCode::HandleKey(const InfoBoxKeyCodes keycode)
{
  TeamCodeSettings &settings = CommonInterface::SetSettingsComputer();
  const FlarmState &flarm = XCSoarInterface::Basic().flarm;
  const FlarmTraffic *traffic =
    settings.team_flarm_id.IsDefined()
    ? flarm.FindTraffic(settings.team_flarm_id)
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
    settings.team_flarm_id = traffic->id;

    if (traffic->HasName()) {
      // copy the 3 first chars from the name to TeamFlarmCNTarget
      settings.team_flarm_callsign = traffic->name;
    } else {
      settings.team_flarm_callsign.clear();
    }
  } else {
    // no flarm traffic to select!
    settings.team_flarm_id.Clear();
    settings.team_flarm_callsign.clear();
  }
  return true;
}

void
InfoBoxContentTeamBearing::Update(InfoBoxData &data)
{
  const TeamCodeSettings &settings = CommonInterface::SettingsComputer();
  const FlarmState &flarm = XCSoarInterface::Basic().flarm;
  const TeamInfo &teamcode_info = CommonInterface::Calculated();

  if (teamcode_info.teammate_available) {
    // Set Value
    data.SetValue(teamcode_info.teammate_vector.bearing, _T("T"));
  }
  else
    data.SetValueInvalid();

  // Set Comment
  if (!settings.team_flarm_id.IsDefined())
    data.SetCommentInvalid();
  else if (!settings.team_flarm_callsign.empty())
    data.SetComment(settings.team_flarm_callsign.c_str());
  else
    data.SetComment(_T("???"));

  if (flarm.FindTraffic(settings.team_flarm_id) != NULL)
    data.SetCommentColor(2);
  else
    data.SetCommentColor(1);
}

void
InfoBoxContentTeamBearingDiff::Update(InfoBoxData &data)
{
  const TeamCodeSettings &settings = CommonInterface::SettingsComputer();
  const NMEAInfo &basic = XCSoarInterface::Basic();
  const FlarmState &flarm = basic.flarm;
  const TeamInfo &teamcode_info = CommonInterface::Calculated();

  if (teamcode_info.teammate_available && basic.track_available) {
    // Set Value
    Angle Value = teamcode_info.teammate_vector.bearing - basic.track;
    SetValueBearingDifference(data, Value);
  } else
    data.SetValueInvalid();

  // Set Comment
  if (!settings.team_flarm_id.IsDefined())
    data.SetCommentInvalid();
  else if (!string_is_empty(settings.team_flarm_callsign))
    data.SetComment(settings.team_flarm_callsign);
  else
    data.SetComment(_T("???"));

  if (flarm.FindTraffic(settings.team_flarm_id) != NULL)
    data.SetCommentColor(2);
  else
    data.SetCommentColor(1);
}

void
InfoBoxContentTeamDistance::Update(InfoBoxData &data)
{
  const TeamCodeSettings &settings = CommonInterface::SettingsComputer();
  const TeamInfo &teamcode_info = CommonInterface::Calculated();

  // Set Value
  if (teamcode_info.teammate_available) {
    TCHAR tmp[32];
    Units::FormatUserDistance(teamcode_info.teammate_vector.distance,
                              tmp, 32, false);
    data.SetValue(tmp);

    // Set Unit
    data.SetValueUnit(Units::current.distance_unit);
  } else
    data.SetValueInvalid();

  // Set Comment
  if (!settings.team_flarm_id.IsDefined())
    data.SetCommentInvalid();
  else if (!string_is_empty(settings.team_flarm_callsign))
    data.SetComment(settings.team_flarm_callsign);
  else
    data.SetComment(_T("???"));

  data.SetCommentColor(teamcode_info.flarm_teammate_code_current ? 2 : 1);
}
