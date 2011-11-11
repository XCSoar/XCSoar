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

#include "AltitudeSetup.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "DeviceBlackboard.hpp"
#include "Units/Units.hpp"
#include "Simulator.hpp"
#include "DataField/Float.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/TabBar.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"

class WndButton;

static int InfoBoxID;

static void
PnlSetupOnQNH(DataField *_Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat *Sender = (DataFieldFloat *)_Sender;
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();

  switch (Mode) {
  case DataField::daChange:
    settings_computer.pressure = AtmosphericPressure::HectoPascal(Sender->GetAsFixed());
    settings_computer.pressure_available.Update(CommonInterface::Basic().clock);
    device_blackboard->SetQNH(Sender->GetAsFixed());
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
PnlSetupOnSetup(gcc_unused WndButton &Sender)
{
  InfoBoxManager::SetupFocused(InfoBoxID);
  dlgInfoBoxAccess::OnClose();
}

static gcc_constexpr_data
CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(PnlSetupOnQNH),
  DeclareCallBackEntry(PnlSetupOnSetup),
  DeclareCallBackEntry(NULL)
};

Window *
LoadAltitudeSetupPanel(SingleWindow &parent, TabBarControl *wTabBar,
                       WndForm *wf, const int id)
{
  assert(wTabBar);
  assert(wf);

  InfoBoxID = id;

  Window *wInfoBoxAccessSetup =
    LoadWindow(CallBackTable, wf, wTabBar->GetClientAreaWindow(),
               _T("IDR_XML_INFOBOXALTITUDESETUP"));
  assert(wInfoBoxAccessSetup);

  LoadFormProperty(*wf, _T("prpQNH"),
                   CommonInterface::SettingsComputer().pressure.GetHectoPascal());

  return wInfoBoxAccessSetup;
}
