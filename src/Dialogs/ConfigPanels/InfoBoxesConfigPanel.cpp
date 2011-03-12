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
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Appearance.hpp"
#include "InfoBoxesConfigPanel.hpp"
#include "LogFile.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"

static WndForm* wf = NULL;

static void
dlgConfigInfoboxesShowModal(SingleWindow &main_window, unsigned panel)
{
  InfoBoxPanelConfig &data = infoBoxManagerConfig.panel[panel];

  bool changed = dlgConfigInfoboxesShowModal(main_window,
                                             InfoBoxManager::GetPanelName(panel),
                                             data);
  if (changed) {
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
OnInfoBoxesAuxiliary(gcc_unused WndButton &button)
{
  dlgConfigInfoboxesShowModal(wf->GetMainWindow(),
                              InfoBoxManager::PANEL_AUXILIARY);
}

void
InfoBoxesConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesCircling")))->
      SetOnClickNotify(OnInfoBoxesCircling);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesCruise")))->
      SetOnClickNotify(OnInfoBoxesCruise);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesFinalGlide")))->
      SetOnClickNotify(OnInfoBoxesFinalGlide);
  ((WndButton *)wf->FindByName(_T("cmdInfoBoxesAuxiliary")))->
      SetOnClickNotify(OnInfoBoxesAuxiliary);

  LoadFormProperty(*wf, _T("prpAppInverseInfoBox"),
                   Appearance.InverseInfoBox);

  LoadFormProperty(*wf, _T("prpAppInfoBoxColors"), Appearance.InfoBoxColors);

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxBorder"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Box"));
    dfe->addEnumText(_("Tab"));
    dfe->Set(Appearance.InfoBoxBorder);
    wp->RefreshDisplay();
  }
}


bool
InfoBoxesConfigPanel::Save(bool &requirerestart)
{
  bool changed = false;
  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxBorder"));
  if (wp) {
    if (Appearance.InfoBoxBorder != (InfoBoxBorderAppearance_t)
        (wp->GetDataField()->GetAsInteger())) {
      Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)
        (wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppInfoBoxBorder,
                    Appearance.InfoBoxBorder);
      changed = true;
      requirerestart = true;
    }
  }

  changed |= requirerestart |=
    SaveFormProperty(*wf, _T("prpAppInverseInfoBox"),
                     szProfileAppInverseInfoBox, Appearance.InverseInfoBox);

  changed |= requirerestart |=
    SaveFormProperty(*wf, _T("prpAppInfoBoxColors"),
                     szProfileAppInfoBoxColors, Appearance.InfoBoxColors);


  return changed;
}
