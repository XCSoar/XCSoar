/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "UIState.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ActionWidget.hpp"
#include "Widget/ButtonWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <stdio.h>

/**
 * This modal result will trigger the "Switch InfoBox" dialog.
 */
static constexpr int SWITCH_INFO_BOX = 100;

void
dlgInfoBoxAccessShowModeless(const int id, const InfoBoxPanel *panels)
{
  assert (id > -1);

  const InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  const unsigned panel_index = CommonInterface::GetUIState().panel_index;
  const InfoBoxSettings::Panel &panel = settings.panels[panel_index];
  const InfoBoxFactory::Type old_type = panel.contents[id];

  const DialogLook &look = UIGlobals::GetDialogLook();

  PixelRect form_rc = InfoBoxManager::layout.remaining;

  WidgetDialog dialog(look);

  TabWidget tab_widget(TabWidget::Orientation::HORIZONTAL);
  dialog.Create(UIGlobals::GetMainWindow(),
                gettext(InfoBoxFactory::GetName(old_type)),
                form_rc, &tab_widget);
  dialog.PrepareWidget();

  bool found_setup = false;

  if (panels != nullptr) {
    for (; panels->load != nullptr; ++panels) {
      assert(panels->name != nullptr);

      Widget *widget = panels->load(id);

      if (widget == NULL)
        continue;

      if (!found_setup && StringIsEqual(panels->name, _T("Setup"))) {
        /* add a "Switch InfoBox" button to the "Setup" tab -
           kludge! */
        found_setup = true;

        PixelRect button_rc;
        button_rc.left = 0;
        button_rc.top = 0;
        button_rc.right = Layout::Scale(60);
        button_rc.bottom = std::max(2u * Layout::GetMinimumControlHeight(),
                                    Layout::GetMaximumControlHeight());

        auto *button = new ButtonWidget(look.button, _("Switch InfoBox"),
                                        dialog, SWITCH_INFO_BOX);

        widget = new TwoWidgets(widget, button, false);
      }

      tab_widget.AddTab(widget, gettext(panels->name));
    }
  }

  if (!found_setup) {
    /* the InfoBox did not provide a "Setup" tab - create a default
       one that allows switching the contents */
    Widget *wSwitch = new ActionWidget(dialog, SWITCH_INFO_BOX);
    tab_widget.AddTab(wSwitch, _("Switch InfoBox"));
  }

  Widget *wClose = new ActionWidget(dialog, mrOK);
  tab_widget.AddTab(wClose, _("Close"));

  const PixelRect client_rc = dialog.GetClientAreaWindow().GetClientRect();
  const PixelSize max_size = tab_widget.GetMaximumSize();
  if (unsigned(max_size.cy) < client_rc.GetHeight()) {
    form_rc.top += client_rc.GetHeight() - max_size.cy;
    dialog.Move(form_rc);
  }

  dialog.SetModeless();
  int result = dialog.ShowModal();

  dialog.StealWidget();

  if (result == SWITCH_INFO_BOX)
    InfoBoxManager::ShowInfoBoxPicker(id);
}
