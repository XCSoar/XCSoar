/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "TeamActions.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Widget/CallbackWidget.hpp"
#include "Language/Language.hpp"
#include "Util/StringCompare.hxx"

#include <tchar.h>
#include <stdio.h>

static void
ShowTeamCodeDialog()
{
  dlgTeamCodeShowModal();
}

static Widget *
LoadTeamCodeDialog(unsigned id)
{
  return new CallbackWidget(ShowTeamCodeDialog);
}

static constexpr InfoBoxPanel team_code_infobox_panels[] = {
  { N_("Team Code"), LoadTeamCodeDialog },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentTeamCode::GetDialogContent()
{
  return team_code_infobox_panels;
}

void
InfoBoxContentTeamCode::Update(InfoBoxData &data)
{
  const TeamCodeSettings &settings =
    CommonInterface::GetComputerSettings().team_code;
  const TeamInfo &teamcode_info = CommonInterface::Calculated();

  if (!settings.team_code_reference_waypoint) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(CommonInterface::Calculated().own_teammate_code.GetCode());

  // Set Comment
  if (teamcode_info.flarm_teammate_code.IsDefined()) {
    data.SetComment(teamcode_info.flarm_teammate_code.GetCode());
    data.SetCommentColor(teamcode_info.flarm_teammate_code_current ? 2 : 1);
  } else if (settings.team_code.IsDefined()) {
    data.SetComment(settings.team_code.GetCode());
    data.SetCommentColor(0);
  }
  else
    data.SetCommentInvalid();
}

bool
InfoBoxContentTeamCode::HandleKey(const InfoBoxKeyCodes keycode)
{
  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;
  const TrafficList &flarm = CommonInterface::Basic().flarm.traffic;
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
    TeamActions::TrackFlarm(traffic->id, traffic->name);
  } else {
    // no flarm traffic to select!
    settings.team_flarm_id.Clear();
    settings.team_flarm_callsign.clear();
  }
  return true;
}

void
UpdateInfoBoxTeamBearing(InfoBoxData &data)
{
  const TeamCodeSettings &settings =
    CommonInterface::GetComputerSettings().team_code;
  const TrafficList &flarm = CommonInterface::Basic().flarm.traffic;
  const TeamInfo &teamcode_info = CommonInterface::Calculated();

  if (teamcode_info.teammate_available) {
    // Set Value
    data.SetValue(teamcode_info.teammate_vector.bearing);
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
UpdateInfoBoxTeamBearingDiff(InfoBoxData &data)
{
  const TeamCodeSettings &settings =
    CommonInterface::GetComputerSettings().team_code;
  const NMEAInfo &basic = CommonInterface::Basic();
  const TrafficList &flarm = basic.flarm.traffic;
  const TeamInfo &teamcode_info = CommonInterface::Calculated();

  if (teamcode_info.teammate_available && basic.track_available) {
    // Set Value
    Angle Value = teamcode_info.teammate_vector.bearing - basic.track;
    data.SetValueFromBearingDifference(Value);
  } else
    data.SetValueInvalid();

  // Set Comment
  if (!settings.team_flarm_id.IsDefined())
    data.SetCommentInvalid();
  else if (!StringIsEmpty(settings.team_flarm_callsign))
    data.SetComment(settings.team_flarm_callsign);
  else
    data.SetComment(_T("???"));

  if (flarm.FindTraffic(settings.team_flarm_id) != NULL)
    data.SetCommentColor(2);
  else
    data.SetCommentColor(1);
}

void
UpdateInfoBoxTeamDistance(InfoBoxData &data)
{
  const TeamCodeSettings &settings =
    CommonInterface::GetComputerSettings().team_code;
  const TeamInfo &teamcode_info = CommonInterface::Calculated();

  // Set Value
  if (teamcode_info.teammate_available)
    data.SetValueFromDistance(teamcode_info.teammate_vector.distance);
  else
    data.SetValueInvalid();

  // Set Comment
  if (!settings.team_flarm_id.IsDefined())
    data.SetCommentInvalid();
  else if (!StringIsEmpty(settings.team_flarm_callsign))
    data.SetComment(settings.team_flarm_callsign);
  else
    data.SetComment(_T("???"));

  data.SetCommentColor(teamcode_info.flarm_teammate_code_current ? 2 : 1);
}
