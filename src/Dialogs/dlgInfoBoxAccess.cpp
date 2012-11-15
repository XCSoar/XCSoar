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

#include "Dialogs/Dialogs.h"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "UIGlobals.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Components.hpp"
#include "Look/Look.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Form/TabBar.hpp"
#include "Form/Form.hpp"
#include "Form/ActionWidget.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <stdio.h>

/**
 * This modal result will trigger the "Switch InfoBox" dialog.
 */
static constexpr int SWITCH_INFO_BOX = 100;

static WndForm *wf = NULL;

static TabBarControl* wTabBar = NULL;

void
dlgInfoBoxAccessShowModeless(const int id,
                             const InfoBoxContent::DialogContent *dlgContent)
{
  // check for another instance of this window
  if (wf != NULL) return;
  assert (id > -1);

  const InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  const unsigned panel_index = InfoBoxManager::GetCurrentPanel();
  const InfoBoxSettings::Panel &panel = settings.panels[panel_index];
  const InfoBoxFactory::Type old_type = panel.contents[id];

  const DialogLook &look = UIGlobals::GetDialogLook();

  PixelRect form_rc = InfoBoxManager::layout.remaining;
  form_rc.top = form_rc.bottom - Layout::Scale(107);

  wf = new WndForm(UIGlobals::GetMainWindow(), look, form_rc,
                   gettext(InfoBoxFactory::GetName(old_type)));

  WindowStyle tab_style;
  tab_style.ControlParent();
  ContainerWindow &client_area = wf->GetClientAreaWindow();
  const PixelRect rc = client_area.GetClientRect();
  wTabBar = new TabBarControl(client_area, look, rc.left, rc.top,
                              rc.right - rc.left, Layout::Scale(45),
                              tab_style, Layout::landscape);

  if (dlgContent != NULL) {
    for (int i = 0; i < dlgContent->PANELSIZE; i++) {
      assert(dlgContent->Panels[i].load != NULL);

      Widget *widget = dlgContent->Panels[i].load(id);

      if (widget == NULL)
        continue;

      wTabBar->AddTab(widget, gettext(dlgContent->Panels[i].name));
    }
  }

  if (!wTabBar->GetTabCount()) {
    form_rc.top = form_rc.bottom - Layout::Scale(58);
    wf->Move(form_rc);

    Widget *wSwitch = new ActionWidget(*wf, SWITCH_INFO_BOX);
    wTabBar->AddTab(wSwitch, _("Switch InfoBox"));
  }

  Widget *wClose = new ActionWidget(*wf, mrOK);
  wTabBar->AddTab(wClose, _("Close"));

  int result = wf->ShowModeless();

  if (wf->IsDefined()) {
    bool changed = false, require_restart = false;
    wTabBar->Save(changed, require_restart);
  }

  delete wTabBar;
  delete wf;
  // unset wf because wf is still static and public
  wf = NULL;

  if (result == SWITCH_INFO_BOX)
    InfoBoxManager::ShowInfoBoxPicker(id);
}

bool
dlgInfoBoxAccess::OnClose()
{
  wf->SetModalResult(mrOK);
  return true;
}
