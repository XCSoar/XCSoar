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

#include "Dialogs/Traffic.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/Waypoint.hpp"
#include "DataField/Float.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "SettingsComputer.hpp"
#include "Blackboard.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"
#include "StringUtil.hpp"
#include "TeamCodeCalculation.hpp"
#include "Compiler.h"
#include "Profile/Profile.hpp"
#include "Engine/Waypoint/Waypoint.hpp"

#include <stdio.h>

static WndForm *wf = NULL;

static void
Update()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const TeamInfo &teamcode_info = CommonInterface::Calculated();
  TCHAR Text[100];

  if (teamcode_info.teammate_available && basic.track_available) {
    double Value = (teamcode_info.teammate_vector.bearing - basic.track)
      .as_delta().value_degrees();

    if (Value > 1)
      _stprintf(Text, _T("%2.0f")_T(DEG)_T(">"), Value);
    else if (Value < -1)
      _stprintf(Text, _T("<%2.0f")_T(DEG), -Value);
    else
      _tcscpy(Text, _T("<>"));
  } else {
    _tcscpy(Text, _T("---"));
  }

  SetFormValue(*wf, _T("prpRelBearing"), Text);

  if (teamcode_info.teammate_available) {
    LoadFormProperty(*wf, _T("prpBearing"),
                     teamcode_info.teammate_vector.bearing.value_degrees());
    LoadFormProperty(*wf, _T("prpRange"), ugDistance,
                     teamcode_info.teammate_vector.distance);
  }

  SetFormValue(*wf, _T("prpOwnCode"),
               teamcode_info.own_teammate_code.GetCode());
  SetFormValue(*wf, _T("prpMateCode"),
               CommonInterface::SettingsComputer().TeammateCode.GetCode());

  const SETTINGS_TEAMCODE &settings = CommonInterface::SettingsComputer();
  SetFormValue(*wf, _T("prpFlarmLock"),
               settings.TeamFlarmTracking
               ? settings.TeamFlarmCNTarget.c_str()
               : _T(""));
}

static void
OnSetWaypointClicked(gcc_unused WndButton &button)
{
  const Waypoint* wp = dlgWaypointSelect(XCSoarInterface::main_window,
                                         XCSoarInterface::Basic().location);
  if (wp != NULL) {
    XCSoarInterface::SetSettingsComputer().TeamCodeRefWaypoint = wp->id;
    Profile::Set(szProfileTeamcodeRefWaypoint, wp->id);
  }
}

static void
OnCodeClicked(gcc_unused WndButton &button)
{
  TCHAR newTeammateCode[10];

  CopyString(newTeammateCode,
             XCSoarInterface::SettingsComputer().TeammateCode.GetCode(), 10);

  if (!dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                             newTeammateCode, 7))
    return;

  TrimRight(newTeammateCode);

  XCSoarInterface::SetSettingsComputer().TeammateCode.Update(newTeammateCode);
  if (!string_is_empty(XCSoarInterface::SettingsComputer().TeammateCode.GetCode())) {
    XCSoarInterface::SetSettingsComputer().TeammateCodeValid = true;
    XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = false;
  }
  else
    XCSoarInterface::SetSettingsComputer().TeammateCodeValid = false;
}

static void
OnFlarmLockClicked(gcc_unused WndButton &button)
{
  SETTINGS_TEAMCODE &settings = CommonInterface::SetSettingsComputer();
  TCHAR newTeamFlarmCNTarget[settings.TeamFlarmCNTarget.MAX_SIZE];
  _tcscpy(newTeamFlarmCNTarget, settings.TeamFlarmCNTarget.c_str());

  if (!dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                             newTeamFlarmCNTarget, 4))
    return;

  settings.TeamFlarmCNTarget = newTeamFlarmCNTarget;
  settings.TeammateCodeValid = false;

  if (string_is_empty(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget)) {
    settings.TeamFlarmTracking = false;
    settings.TeamFlarmIdTarget.clear();
    return;
  }

  const FlarmId *ids[30];
  unsigned count = FlarmDetails::FindIdsByCallSign(
      XCSoarInterface::SettingsComputer().TeamFlarmCNTarget, ids, 30);

  if (count > 0) {
    const FlarmId *id = dlgFlarmDetailsListShowModal(
        XCSoarInterface::main_window, _("Set new teammate:"), ids, count);

    if (id != NULL && id->defined()) {
      settings.TeamFlarmIdTarget = *id;
      settings.TeamFlarmTracking = true;
      return;
    }
  } else {
    MessageBoxX(_("Unknown Competition Number"),
                _("Not Found"), MB_OK | MB_ICONINFORMATION);
  }

  settings.TeamFlarmTracking = false;
  settings.TeamFlarmIdTarget.clear();
  settings.TeamFlarmCNTarget.clear();
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  Update();
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(OnFlarmLockClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgTeamCodeShowModal(void)
{
  WndButton *buttonCode = NULL;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_TEAMCODE_L") :
                                      _T("IDR_XML_TEAMCODE"));

  if (!wf)
    return;

  // set event for buttons
  buttonCode = ((WndButton *)wf->FindByName(_T("cmdSetCode")));
  if (buttonCode)
    buttonCode->SetOnClickNotify(OnCodeClicked);

  WndButton* cmdSetWaypoint = ((WndButton *)wf->FindByName(_T("cmdSetWaypoint")));
  assert(cmdSetWaypoint != NULL);
  cmdSetWaypoint->SetOnClickNotify(OnSetWaypointClicked);

  Update();

  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal();

  delete wf;
}
