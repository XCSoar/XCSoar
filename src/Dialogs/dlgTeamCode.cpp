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

#include "Dialogs/Traffic.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Base.hpp"
#include "Form/DataField/Float.hpp"
#include "UIGlobals.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "ComputerSettings.hpp"
#include "Screen/Layout.hpp"
#include "Util/StringUtil.hpp"
#include "TeamCode.hpp"
#include "Compiler.h"
#include "Profile/Profile.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Interface.hpp"
#include "Blackboard/ScopeCalculatedListener.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

static WndForm *wf = NULL;

static void
Update(const MoreData &basic, const DerivedInfo &calculated)
{
  const TeamInfo &teamcode_info = calculated;
  const TeamCodeSettings &settings =
    CommonInterface::GetComputerSettings().team_code;
  StaticString<100> buffer;

  if (teamcode_info.teammate_available && basic.track_available) {
    FormatAngleDelta(buffer.buffer(), buffer.MAX_SIZE,
                     teamcode_info.teammate_vector.bearing - basic.track);
  } else {
    buffer = _T("---");
  }

  SetFormValue(*wf, _T("prpRelBearing"), buffer);

  if (teamcode_info.teammate_available) {
    LoadFormProperty(*wf, _T("prpBearing"),
                     teamcode_info.teammate_vector.bearing.Degrees());
    LoadFormProperty(*wf, _T("prpRange"), UnitGroup::DISTANCE,
                     teamcode_info.teammate_vector.distance);
  }

  SetFormValue(*wf, _T("prpOwnCode"),
               teamcode_info.own_teammate_code.GetCode());
  SetFormValue(*wf, _T("prpMateCode"), settings.team_code.GetCode());

  SetFormValue(*wf, _T("prpFlarmLock"),
               settings.team_flarm_tracking
               ? settings.team_flarm_callsign.c_str()
               : _T(""));
}

static void
OnSetWaypointClicked(gcc_unused WndButton &button)
{
  const Waypoint* wp = ShowWaypointListDialog(UIGlobals::GetMainWindow(),
                                         XCSoarInterface::Basic().location);
  if (wp != NULL) {
    CommonInterface::SetComputerSettings().team_code.team_code_reference_waypoint = wp->id;
    Profile::Set(ProfileKeys::TeamcodeRefWaypoint, wp->id);
  }
}

static void
OnCodeClicked(gcc_unused WndButton &button)
{
  TCHAR newTeammateCode[10];

  CopyString(newTeammateCode,
             CommonInterface::GetComputerSettings().team_code.team_code.GetCode(), 10);

  if (!dlgTextEntryShowModal(newTeammateCode, 7))
    return;

  TrimRight(newTeammateCode);

  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;
  settings.team_code.Update(newTeammateCode);
  if (!StringIsEmpty(settings.team_code.GetCode())) {
    settings.team_code_valid = true;
    settings.team_flarm_tracking = false;
  }
  else
    settings.team_code_valid = false;
}

static void
OnFlarmLockClicked(gcc_unused WndButton &button)
{
  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;
  TCHAR newTeamFlarmCNTarget[settings.team_flarm_callsign.MAX_SIZE];
  _tcscpy(newTeamFlarmCNTarget, settings.team_flarm_callsign.c_str());

  if (!dlgTextEntryShowModal(newTeamFlarmCNTarget, 4))
    return;

  settings.team_flarm_callsign = newTeamFlarmCNTarget;
  settings.team_code_valid = false;

  if (StringIsEmpty(settings.team_flarm_callsign)) {
    settings.team_flarm_tracking = false;
    settings.team_flarm_id.Clear();
    return;
  }

  FlarmId ids[30];
  unsigned count =
    FlarmDetails::FindIdsByCallSign(settings.team_flarm_callsign, ids, 30);

  if (count > 0) {
    const FlarmId id =
      dlgFlarmDetailsListShowModal(UIGlobals::GetMainWindow(),
                                   _("Set new teammate:"), ids, count);

    if (id.IsDefined()) {
      settings.team_flarm_id = id;
      settings.team_flarm_tracking = true;
      return;
    }
  } else {
    ShowMessageBox(_("Unknown Competition Number"),
                _("Not Found"), MB_OK | MB_ICONINFORMATION);
  }

  settings.team_flarm_tracking = false;
  settings.team_flarm_id.Clear();
  settings.team_flarm_callsign.clear();
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnFlarmLockClicked),
  DeclareCallBackEntry(OnCodeClicked),
  DeclareCallBackEntry(OnSetWaypointClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgTeamCodeShowModal()
{
  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_TEAMCODE_L") :
                                      _T("IDR_XML_TEAMCODE"));

  if (!wf)
    return;

  Update(CommonInterface::Basic(), CommonInterface::Calculated());

  const ScopeCalculatedListener l(CommonInterface::GetLiveBlackboard(),
                                  Update);

  wf->ShowModal();

  delete wf;
}
