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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Blackboard.hpp"
#include "Units/Units.hpp"
#include "InputEvents.hpp"
#include "DataField/Boolean.hpp"
#include "MainWindow.hpp"

static WndForm *wf = NULL;

static void
Update(const TCHAR *name, bool value)
{
  LoadFormProperty(*wf, name, value);
}

static void
UpdateValues()
{
  const SwitchInfo &switches = XCSoarInterface::Basic().switch_state;

  Update(_T("prpFlapLanding"), switches.flap_landing);
  Update(_T("prpAirbrakeExtended"), switches.airbrake_locked);
  Update(_T("prpFlapPositive"), switches.flap_positive);
  Update(_T("prpFlapNeutral"), switches.flap_neutral);
  Update(_T("prpFlapNegative"), switches.flap_negative);
  Update(_T("prpGearExtended"), switches.gear_extended);
  Update(_T("prpAcknowledge"), switches.acknowledge);
  Update(_T("prpRepeat"), switches.repeat);
  Update(_T("prpSpeedCommand"), switches.speed_command);
  Update(_T("prpUserSwitchUp"), switches.user_switch_up);
  Update(_T("prpUserSwitchMiddle"), switches.user_switch_middle);
  Update(_T("prpUserSwitchDown"), switches.user_switch_down);
  Update(_T("prpVarioCircling"),
         switches.flight_mode == SwitchInfo::MODE_CIRCLING);
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  UpdateValues();
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgSwitchesShowModal(void)
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
		              _T("IDR_XML_SWITCHES"));
  if (wf == NULL)
    return;

  wf->SetTimerNotify(OnTimerNotify);

  UpdateValues();
  wf->ShowModal();

  delete wf;
}
