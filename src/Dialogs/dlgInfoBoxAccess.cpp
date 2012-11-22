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
#include "InfoBoxes/Content/Factory.hpp"
#include "Form/TabBar.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/ActionWidget.hpp"
#include "Form/WindowWidget.hpp"
#include "Form/TwoWidgets.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <stdio.h>

/**
 * This modal result will trigger the "Switch InfoBox" dialog.
 */
static constexpr int SWITCH_INFO_BOX = 100;

void
dlgInfoBoxAccessShowModeless(const int id,
                             const InfoBoxContent::DialogContent *dlgContent)
{
  assert (id > -1);

  const InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  const unsigned panel_index = InfoBoxManager::GetCurrentPanel();
  const InfoBoxSettings::Panel &panel = settings.panels[panel_index];
  const InfoBoxFactory::Type old_type = panel.contents[id];

  const DialogLook &look = UIGlobals::GetDialogLook();

  PixelRect form_rc = InfoBoxManager::layout.remaining;

  WndForm form(UIGlobals::GetMainWindow(), look, form_rc,
               gettext(InfoBoxFactory::GetName(old_type)));

  WindowStyle tab_style;
  tab_style.ControlParent();
  ContainerWindow &client_area = form.GetClientAreaWindow();
  PixelRect client_rc = client_area.GetClientRect();
  PixelRect tab_rc = client_rc;
  tab_rc.bottom = tab_rc.top + Layout::Scale(45);

  TabBarControl tab_bar(client_area, look, tab_rc, tab_style, false);

  if (dlgContent != NULL) {
    for (int i = 0; i < dlgContent->PANELSIZE; i++) {
      assert(dlgContent->Panels[i].load != NULL);

      Widget *widget = dlgContent->Panels[i].load(id);

      if (widget == NULL)
        continue;

      if (i == dlgContent->PANELSIZE - 1) {
        /* add a "Switch InfoBox" button to the last tab, which we
           expect to be the "Setup" tab - kludge! */

        PixelRect button_rc;
        button_rc.left = 0;
        button_rc.top = 0;
        button_rc.right = Layout::Scale(60);
        button_rc.bottom = std::max(UPixelScalar(2 * Layout::GetMinimumControlHeight()),
                                    Layout::GetMaximumControlHeight());

        ButtonWindowStyle button_style;
        button_style.Hide();
        button_style.TabStop();
        button_style.multiline();

        WndButton *button =
          new WndButton(tab_bar, look, _("Switch InfoBox"),
                        button_rc, button_style,
                        form, SWITCH_INFO_BOX);

        widget = new TwoWidgets(widget, new WindowWidget(button), false);
      }

      tab_bar.AddTab(widget, gettext(dlgContent->Panels[i].name));
    }
  }

  if (tab_bar.GetTabCount() == 0) {
    Widget *wSwitch = new ActionWidget(form, SWITCH_INFO_BOX);
    tab_bar.AddTab(wSwitch, _("Switch InfoBox"));
  }

  Widget *wClose = new ActionWidget(form, mrOK);
  tab_bar.AddTab(wClose, _("Close"));

  const PixelSize max_size = tab_bar.GetMaximumSize();
  if (unsigned(max_size.cy) < unsigned(client_rc.bottom - client_rc.top)) {
    form_rc.top += client_rc.bottom - client_rc.top - max_size.cy;
    form.Move(form_rc);
    tab_bar.Move(client_area.GetClientRect());
  }

  int result = form.ShowModeless();

  if (form.IsDefined()) {
    bool changed = false, require_restart = false;
    tab_bar.Save(changed, require_restart);
  }

  if (result == SWITCH_INFO_BOX)
    InfoBoxManager::ShowInfoBoxPicker(id);
}
