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

#include "MacCreadySetup.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/TabBar.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"

class WndButton;

static int InfoBoxID;

static void
PnlSetupOnSetup(gcc_unused WndButton &Sender) {
  InfoBoxManager::SetupFocused(InfoBoxID);
  dlgInfoBoxAccess::OnClose();
}

static void
PnlSetupOnMode(gcc_unused WndButton &Sender)
{
  if (XCSoarInterface::SettingsComputer().task.auto_mc)
    Sender.SetCaption(_("AUTO"));
  else
    Sender.SetCaption(_("MANUAL"));

  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("mode"));
}

static gcc_constexpr_data CallBackTableEntry call_back_table[] = {
  DeclareCallBackEntry(PnlSetupOnSetup),
  DeclareCallBackEntry(PnlSetupOnMode),
  DeclareCallBackEntry(NULL)
};

Window *
LoadMacCreadySetupPanel(SingleWindow &parent, TabBarControl *wTabBar,
                        WndForm *wf, const int id)
{
  assert(wTabBar);
  assert(wf);

  InfoBoxID = id;

  Window *wInfoBoxAccessSetup =
    LoadWindow(call_back_table, wf, *wTabBar, _T("IDR_XML_INFOBOXMACCREADYSETUP"));
  assert(wInfoBoxAccessSetup);

  return wInfoBoxAccessSetup;
}

bool
MacCreadySetupPanelPreShow(TabBarControl::EventType EventType)
{
  if (XCSoarInterface::SettingsComputer().task.auto_mc)
    ((WndButton *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("cmdMode")))->SetCaption(_("MANUAL"));
  else
    ((WndButton *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("cmdMode")))->SetCaption(_("AUTO"));

  return true;
}
