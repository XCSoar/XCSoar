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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Button.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Dialogs/Dialogs.h"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Appearance.hpp"
#include "InfoBoxesConfigPanel.hpp"
#include "LogFile.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Dialogs/dlgConfigInfoboxes.hpp"

static WndForm* wf = NULL;

static void
dlgConfigInfoboxesShowModal(SingleWindow &main_window, unsigned panel)
{
  InfoBoxPanelConfig &data = infoBoxManagerConfig.panel[panel];

  bool changed = dlgConfigInfoboxesShowModal(main_window,
                                             InfoBoxLayout::InfoBoxGeometry,
                                             data);
  if (changed) {
    data.modified = true;
    Profile::SetInfoBoxManagerConfig(infoBoxManagerConfig);
    Profile::Save();
    LogDebug(_T("InfoBox configuration: Changes saved"));
  }
}

static void
OnInfoBoxesCircling(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_CIRCLING);
}

static void
OnInfoBoxesCruise(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_CRUISE);
}

static void
OnInfoBoxesFinalGlide(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_FINAL_GLIDE);
}

static void
OnInfoBoxesAux1(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_AUXILIARY);
}

static void
OnInfoBoxesAux2(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_AUXILIARY+1);
}

static void
OnInfoBoxesAux3(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_AUXILIARY+2);
}

static void
OnInfoBoxesAux4(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_AUXILIARY+3);
}

static void
OnInfoBoxesAux5(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_AUXILIARY+4);
}

void
InfoBoxesConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesCircling")))->
      SetOnClickNotify(OnInfoBoxesCircling);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesCruise")))->
      SetOnClickNotify(OnInfoBoxesCruise);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesFinalGlide")))->
      SetOnClickNotify(OnInfoBoxesFinalGlide);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesAux1")))->
      SetOnClickNotify(OnInfoBoxesAux1);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesAux2")))->
      SetOnClickNotify(OnInfoBoxesAux2);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesAux3")))->
      SetOnClickNotify(OnInfoBoxesAux3);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesAux4")))->
      SetOnClickNotify(OnInfoBoxesAux4);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesAux5")))->
      SetOnClickNotify(OnInfoBoxesAux5);

}


bool
InfoBoxesConfigPanel::Save(bool &requirerestart)
{
  return false;
}
