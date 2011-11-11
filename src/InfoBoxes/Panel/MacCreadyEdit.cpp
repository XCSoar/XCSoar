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

#include "MacCreadyEdit.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Simulator.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/TabBar.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"

class WndButton;

int InfoBoxID;

static void
PnlEditOnPlusSmall(gcc_unused WndButton &Sender)
{
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("+0.1"));
}

static void
PnlEditOnPlusBig(gcc_unused WndButton &Sender)
{
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("+0.5"));
}

static void
PnlEditOnMinusSmall(gcc_unused WndButton &Sender)
{
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("-0.1"));
}

static void
PnlEditOnMinusBig(gcc_unused WndButton &Sender)
{
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("-0.5"));
}

static gcc_constexpr_data CallBackTableEntry call_back_table[] = {
  DeclareCallBackEntry(PnlEditOnPlusSmall),
  DeclareCallBackEntry(PnlEditOnPlusBig),
  DeclareCallBackEntry(PnlEditOnMinusSmall),
  DeclareCallBackEntry(PnlEditOnMinusBig),
  DeclareCallBackEntry(NULL)
};

Window *
LoadMacCreadyEditPanel(SingleWindow &parent, TabBarControl *wTabBar,
                       WndForm *wf, int id)
{
  assert(wTabBar);
  assert(wf);

  InfoBoxID = id;

  Window *wInfoBoxAccessEdit =
    LoadWindow(call_back_table, wf, wTabBar->GetClientAreaWindow(),
               _T("IDR_XML_INFOBOXMACCREADYEDIT"));
  assert(wInfoBoxAccessEdit);

  return wInfoBoxAccessEdit;
}
