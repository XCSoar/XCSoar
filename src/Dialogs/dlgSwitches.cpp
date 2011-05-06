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
  const SWITCH_INFO &switches = XCSoarInterface::Basic().SwitchState;

  Update(_T("prpFlapLanding"), switches.FlapLanding);
  Update(_T("prpAirbrakeExtended"), switches.AirbrakeLocked);
  Update(_T("prpFlapPositive"), switches.FlapPositive);
  Update(_T("prpFlapNeutral"), switches.FlapNeutral);
  Update(_T("prpFlapNegative"), switches.FlapNegative);
  Update(_T("prpGearExtended"), switches.GearExtended);
  Update(_T("prpAcknowledge"), switches.Acknowledge);
  Update(_T("prpRepeat"), switches.Repeat);
  Update(_T("prpSpeedCommand"), switches.SpeedCommand);
  Update(_T("prpUserSwitchUp"), switches.UserSwitchUp);
  Update(_T("prpUserSwitchMiddle"), switches.UserSwitchMiddle);
  Update(_T("prpUserSwitchDown"), switches.UserSwitchDown);
  Update(_T("prpVarioCircling"),
         switches.FlightMode == SWITCH_INFO::MODE_CIRCLING);
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
