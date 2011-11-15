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

#include "WindSetup.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/TabBar.hpp"
#include "DataField/Boolean.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Profile/ProfileKeys.hpp"

class WndButton;

static int InfoBoxID;

static void
PnlSetupOnSetup(gcc_unused WndButton &Sender)
{
  InfoBoxManager::SetupFocused(InfoBoxID);
  dlgInfoBoxAccess::OnClose();
}

static gcc_constexpr_data
CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(PnlSetupOnSetup),
  DeclareCallBackEntry(NULL)
};

Window *
LoadWindSetupPanel(SingleWindow &parent, TabBarControl *wTabBar,
                        WndForm *wf, const int id)
{
  assert(wTabBar);
  assert(wf);

  InfoBoxID = id;

  Window *wInfoBoxAccessSetup =
    LoadWindow(CallBackTable, wf, wTabBar->GetClientAreaWindow(),
               _T("IDR_XML_INFOBOXWINDSETUP"));
  assert(wInfoBoxAccessSetup);

  const NMEAInfo &basic = XCSoarInterface::Basic();
  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.use_external_wind;

  if (external_wind) {
    static gcc_constexpr_data StaticEnumChoice external_wind_list[] = {
      { 0, N_("External") },
      { 0 }
    };

    SetFormControlEnabled(*wf, _T("prpAutoWind"), false);
    LoadFormProperty(*wf, _T("prpAutoWind"), external_wind_list, 0);
  } else {
    static gcc_constexpr_data StaticEnumChoice auto_wind_list[] = {
      { AUTOWIND_NONE, N_("Manual") },
      { AUTOWIND_CIRCLING, N_("Circling") },
      { AUTOWIND_ZIGZAG, N_("ZigZag") },
      { AUTOWIND_CIRCLING | AUTOWIND_ZIGZAG, N_("Both") },
      { 0 }
    };

    LoadFormProperty(*wf, _T("prpAutoWind"), auto_wind_list,
                     settings_computer.auto_wind_mode);
  }

  LoadFormProperty(*wf, _T("prpTrailDrift"),
                   XCSoarInterface::SettingsMap().trail_drift_enabled);

  return wInfoBoxAccessSetup;
}

bool
WindSetupPanelPreHide()
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.use_external_wind;

  if (!external_wind) {
    SaveFormProperty(*dlgInfoBoxAccess::GetWindowForm(), _T("prpAutoWind"), szProfileAutoWind,
                     settings_computer.auto_wind_mode);
  }

  SaveFormProperty(*dlgInfoBoxAccess::GetWindowForm(), _T("prpTrailDrift"),
                   XCSoarInterface::SetSettingsMap().trail_drift_enabled);
  ActionInterface::SendSettingsMap();

  return true;
}
