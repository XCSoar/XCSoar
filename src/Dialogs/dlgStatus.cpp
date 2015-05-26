/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Dialogs/WidgetDialog.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ButtonWidget.hpp"
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

static int status_page = 0;

static void
SetTitle(WndForm &form, const TabWidget &pager)
{
  StaticString<128> title;
  title.Format(_T("%s: %s"), _("Status"),
               pager.GetButtonCaption(pager.GetCurrentIndex()));
  form.SetCaption(title);
}

void
dlgStatusShowModal(int start_page)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);

  WindowStyle button_style;
  button_style.Hide();
  button_style.TabStop();

  auto *close_button = new ButtonWidget(look.button, _("Close"),
                                        dialog, mrOK);

  TabWidget widget(TabWidget::Orientation::AUTO, close_button);
  widget.SetPageFlippedCallback([&dialog, &widget]() {
      SetTitle(dialog, widget);
    });

  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Status"), &widget);
  dialog.PrepareWidget();

  const NMEAInfo &basic = CommonInterface::Basic();
  const Waypoint *nearest_waypoint = basic.location_available
    ? way_points.GetNearest(CommonInterface::Basic().location, fixed(100000))
    : nullptr;

  /* setup tabs */

  const bool enable_icons =
    CommonInterface::GetUISettings().dialog.tab_style
    == DialogSettings::TabStyle::Icon;

  const IconLook &icons = UIGlobals::GetIconLook();
  const Bitmap *FlightIcon = enable_icons ? &icons.hBmpTabFlight : nullptr;
  const Bitmap *SystemIcon = enable_icons ? &icons.hBmpTabSystem : nullptr;
  const Bitmap *TaskIcon = enable_icons ? &icons.hBmpTabTask : nullptr;
  const Bitmap *RulesIcon = enable_icons ? &icons.hBmpTabRules : nullptr;
  const Bitmap *TimesIcon = enable_icons ? &icons.hBmpTabTimes : nullptr;

  Widget *flight_panel = new FlightStatusPanel(look, nearest_waypoint);
  widget.AddTab(flight_panel, _("Flight"), FlightIcon);

  Widget *system_panel = new SystemStatusPanel(look);
  widget.AddTab(system_panel, _("System"), SystemIcon);

  Widget *task_panel = new TaskStatusPanel(look);
  widget.AddTab(task_panel, _("Task"), TaskIcon);

  Widget *rules_panel = new RulesStatusPanel(look);
  widget.AddTab(rules_panel, _("Rules"), RulesIcon);

  Widget *times_panel = new TimesStatusPanel(look);
  widget.AddTab(times_panel, _("Times"), TimesIcon);

  /* restore previous page */

  if (start_page != -1) {
    status_page = start_page;
  }

  widget.SetCurrent(status_page);

  SetTitle(dialog, widget);

  dialog.ShowModal();
  dialog.StealWidget();

  /* save page number for next time this dialog is opened */
  status_page = widget.GetCurrentIndex();
}
