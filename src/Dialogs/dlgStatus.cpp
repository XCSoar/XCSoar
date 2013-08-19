/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/TabBar.hpp"
#include "UIGlobals.hpp"
#include "Look/IconLook.hpp"
#include "Look/DialogLook.hpp"
#include "StatusPanels/FlightStatusPanel.hpp"
#include "StatusPanels/TaskStatusPanel.hpp"
#include "StatusPanels/RulesStatusPanel.hpp"
#include "StatusPanels/SystemStatusPanel.hpp"
#include "StatusPanels/TimesStatusPanel.hpp"
#include "Screen/Layout.hpp"
#include "Components.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Compiler.h"

#include <assert.h>

static int status_page = 0;

static void
SetTitle(WndForm &form, const TabBarControl &tab_bar)
{
  StaticString<128> title;
  title.Format(_T("%s: %s"), _("Status"),
               tab_bar.GetButtonCaption(tab_bar.GetCurrentPage()));
  form.SetCaption(title);
}

void
dlgStatusShowModal(int start_page)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WndForm form(look);
  form.Create(UIGlobals::GetMainWindow(), _("Status"));

  const PixelRect rc = form.GetClientAreaWindow().GetClientRect();

  PixelRect close_rc, tab_rc;

  close_rc.left = 0;
  close_rc.top = 0;

  if (Layout::landscape) {
    const unsigned tab_width = Layout::Scale(80);

    close_rc.right = tab_width;
    close_rc.bottom = Layout::Scale(28);

    tab_rc.left = 0;
    tab_rc.top = close_rc.bottom;
    tab_rc.right = tab_width;
    tab_rc.bottom = rc.bottom;
  } else {
    close_rc.right = Layout::Scale(55);
    close_rc.bottom = Layout::Scale(76);

    tab_rc.left = close_rc.right;
    tab_rc.top = 0;
    tab_rc.right = rc.right;
    tab_rc.bottom = close_rc.bottom;
  }

  ButtonWindowStyle button_style;
  button_style.TabStop();

  WndButton close_button(form.GetClientAreaWindow(), look.button,
                         _("Close"), close_rc, button_style,
                         form, mrOK);

  ButtonWindowStyle tab_style;
  tab_style.ControlParent();

  TabBarControl tab_bar(form.GetClientAreaWindow(), look, tab_rc,
                        tab_style, Layout::landscape);
  tab_bar.SetPageFlippedCallback([&form, &tab_bar]() {
      SetTitle(form, tab_bar);
    });

  const NMEAInfo &basic = CommonInterface::Basic();
  const Waypoint *nearest_waypoint = basic.location_available
    ? way_points.GetNearest(CommonInterface::Basic().location, fixed(100000))
    : NULL;

  /* setup tabs */

  const bool enable_icons =
    CommonInterface::GetUISettings().dialog.tab_style
    == DialogSettings::TabStyle::Icon;

  const IconLook &icons = UIGlobals::GetIconLook();
  const Bitmap *FlightIcon = enable_icons ? &icons.hBmpTabFlight : NULL;
  const Bitmap *SystemIcon = enable_icons ? &icons.hBmpTabSystem : NULL;
  const Bitmap *TaskIcon = enable_icons ? &icons.hBmpTabTask : NULL;
  const Bitmap *RulesIcon = enable_icons ? &icons.hBmpTabRules : NULL;
  const Bitmap *TimesIcon = enable_icons ? &icons.hBmpTabTimes : NULL;

  Widget *flight_panel = new FlightStatusPanel(look, nearest_waypoint);
  tab_bar.AddTab(flight_panel, _("Flight"), FlightIcon);

  Widget *system_panel = new SystemStatusPanel(look);
  tab_bar.AddTab(system_panel, _("System"), SystemIcon);

  Widget *task_panel = new TaskStatusPanel(look);
  tab_bar.AddTab(task_panel, _("Task"), TaskIcon);

  Widget *rules_panel = new RulesStatusPanel(look);
  tab_bar.AddTab(rules_panel, _("Rules"), RulesIcon);

  Widget *times_panel = new TimesStatusPanel(look);
  tab_bar.AddTab(times_panel, _("Times"), TimesIcon);

  /* restore previous page */

  if (start_page != -1) {
    status_page = start_page;
  }

  tab_bar.SetCurrentPage(status_page);

  SetTitle(form, tab_bar);

  form.ShowModal();

  /* save page number for next time this dialog is opened */
  status_page = tab_bar.GetCurrentPage();
}
